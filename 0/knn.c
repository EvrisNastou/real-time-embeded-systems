#include "knn.h"

knnresult knnsearch (double *Q, double *C, int num_Q, int num_C, int d, int k, int num_threads){
    knnresult result;
    pthread_t th[num_threads];
    int i;
    int block_per_thread;
    ThreadArgs args[num_threads];
    double *normQ, *normC;

    result.idx = (int *)malloc(num_Q * k * sizeof(int));
    result.dst = (double *)malloc(num_Q * k * sizeof(double));

    normQ  = compute_squared_norms(Q, num_Q, d);
    normC = compute_squared_norms(C, num_C, d);

    block_per_thread=num_Q/num_threads;
    for (i=0; i<num_threads; i++){
        args[i].start_q = i * block_per_thread;
    
        if (i == num_threads - 1) {
            args[i].end_q = num_Q; 
        } else {
            args[i].end_q = (i + 1) * block_per_thread; 
        }

        args[i].normC = normC;
        args[i].normQ = normQ;
        args[i].num_c = num_C;
        args[i].d = d;
        args[i].k = k;
        args[i].C = C;
        args[i].Q = Q;
        args[i].global_idx = result.idx;
        args[i].global_dst = result.dst;
        
        if(pthread_create(&th[i], NULL, knn_block, (void *) &args[i])!=0) exit(1);
    }


    for (i = 0; i < num_threads; i++) {
        if (pthread_join(th[i], NULL)!=0) exit(1);
    }

    free(normC);
    free(normQ);

    return result;
}

void *knn_block(void *args){
    ThreadArgs *arg;
    int i, j, n, global_pos;
    int q_base, c_base, q_chunk, c_chunk;
    double *dist_matrix;
    double *local_dst;
    double dist_sq;
    int *local_idx;



    arg=(ThreadArgs *) args;
    dist_matrix = (double *)malloc(BLOCK_SIZE * BLOCK_SIZE * sizeof(double));
    local_idx = (int *)malloc(BLOCK_SIZE * arg->k * sizeof(int));
    local_dst =  (double *)malloc(BLOCK_SIZE * arg->k * sizeof(double));

    for ( q_base = arg->start_q; q_base < arg->end_q; q_base += BLOCK_SIZE) {
        
        q_chunk = (q_base + BLOCK_SIZE > arg->end_q) ? (arg->end_q - q_base) : BLOCK_SIZE;

        for (i = 0; i < q_chunk * arg->k; i++) {
            local_dst[i] = DBL_MAX;
            local_idx[i] = -1;
        }

        for (c_base = 0; c_base < arg->num_c; c_base += BLOCK_SIZE) {
            
            c_chunk = (c_base + BLOCK_SIZE > arg->num_c) ? (arg->num_c - c_base) : BLOCK_SIZE;

            cblas_dgemm(CblasRowMajor,
                CblasNoTrans,    
                CblasTrans,      
                q_chunk,         
                c_chunk,         
                arg->d,          
                -2.0,            
                &arg->Q[q_base * arg->d],
                arg->d,         
                &arg->C[c_base * arg->d],
                arg->d,          
                0.0,             
                dist_matrix,     
                c_chunk);
            
            
            for (i = 0; i < q_chunk; i++) {
                for (j = 0; j < c_chunk; j++) {
                    dist_sq = arg->normQ[q_base + i] + arg->normC[c_base + j] + dist_matrix[i * c_chunk + j];

                    if (dist_sq < 0.0) dist_sq = 0.0;

                    dist_matrix[i * c_chunk + j] = sqrt(fabs(dist_sq)); 
                }
            }

            for (i = 0; i < q_chunk; i++) {
                update_top_k(&local_dst[i * arg->k], &local_idx[i * arg->k], 
                            &dist_matrix[i * c_chunk], c_base, c_chunk, arg->k);
            }

        }

        for (i = 0; i < q_chunk; i++) {
            for (n = 0; n < arg->k; n++) {
                global_pos = (q_base + i) * arg->k + n;
                arg->global_dst[global_pos] = local_dst[i * arg->k + n];
                arg->global_idx[global_pos] = local_idx[i * arg->k + n];
            }
        }

    }

    free(dist_matrix);
    free(local_dst);
    free(local_idx);
}


double* compute_squared_norms(double *P, int num_points, int d) {
    
    double *norms = (double *)malloc(num_points * sizeof(double));
    
    for (int i = 0; i < num_points; i++) {
        double sum = 0.0;
        for (int j = 0; j < d; j++) {
            double val = P[i * d + j]; 
            sum += val * val;
        }
        norms[i] = sum;
    }
    
    return norms;
}


int compare_elements(const void *a, const void *b) {
    Element *e1 = (Element *)a;
    Element *e2 = (Element *)b;
    if (e1->dist < e2->dist) return -1;
    if (e1->dist > e2->dist) return 1;
    return 0;
}


void update_top_k(double *best_dst, int *best_idx, double *new_dsts, int c_base, int c_chunk, int k) {
    int total_candidates = k + c_chunk;
    Element *candidates = (Element *)malloc(total_candidates * sizeof(Element));

    for (int i = 0; i < k; i++) {
        candidates[i].dist = best_dst[i];
        candidates[i].index = best_idx[i];
    }

    for (int i = 0; i < c_chunk; i++) {
        candidates[k + i].dist = new_dsts[i];
        candidates[k + i].index = c_base + i;
    }
.
    qsort(candidates, total_candidates, sizeof(Element), compare_elements);

    for (int i = 0; i < k; i++) {
        best_dst[i] = candidates[i].dist;
        best_idx[i] = candidates[i].index;
    }

    free(candidates);
}
