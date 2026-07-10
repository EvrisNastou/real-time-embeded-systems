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
    
        // Αν είναι το τελευταίο thread, το end_q πάει αναγκαστικά μέχρι το ΤΕΛΟΣ του πίνακα
        if (i == num_threads - 1) {
            args[i].end_q = num_Q; 
        } else {
            // Αλλιώς πάει μέχρι το επόμενο κανονικό "σκαλοπάτι"
            args[i].end_q = (i + 1) * block_per_thread; 
        }

        // Γέμισμα των υπόλοιπων πεδίων
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
        
        // Υπολογίζουμε πόσα σημεία Q έχει αυτό το μπλοκ 
        // (Συνήθως 2000, εκτός από το τελευταίο μπλοκ που μπορεί να έχει λιγότερα)
        q_chunk = (q_base + BLOCK_SIZE > arg->end_q) ? (arg->end_q - q_base) : BLOCK_SIZE;

        // Αρχικοποιούμε τους τοπικούς γείτονες με "Άπειρο" (πολύ μεγάλες τιμές)
        for (i = 0; i < q_chunk * arg->k; i++) {
            local_dst[i] = DBL_MAX;
            local_idx[i] = -1;
        }

        // 3. ΕΣΩΤΕΡΙΚΗ ΛΟΥΠΑ: Διατρέχει ΟΛΟΚΛΗΡΟ τον C σε κομμάτια
        for (c_base = 0; c_base < arg->num_c; c_base += BLOCK_SIZE) {
            
            c_chunk = (c_base + BLOCK_SIZE > arg->num_c) ? (arg->num_c - c_base) : BLOCK_SIZE;

            // --- ΒΗΜΑ Α: OPENBLAS ---
            // Υπολογίζει D = -2 * Q * C^T
            // Εδώ πρέπει να βάλεις τις παραμέτρους της cblas_dgemm
            // Χοντρικά η λογική είναι:
            cblas_dgemm(CblasRowMajor,   // 1. Διάταξη μνήμης (Η C/C++ αποθηκεύει ανά γραμμή)
                CblasNoTrans,    // 2. Ο πίνακας Q ΔΕΝ αντιστρέφεται (Q)
                CblasTrans,      // 3. Ο πίνακας C ΑΝΤΙΣΤΡΕΦΕΤΑΙ (C^T)
                q_chunk,         // 4. Αριθμός γραμμών του Q (και του τελικού πίνακα)
                c_chunk,         // 5. Αριθμός στηλών του τελικού πίνακα (ή γραμμών του C)
                arg->d,          // 6. Εσωτερική διάσταση (Οι διαστάσεις d των σημείων)
                -2.0,            // 7. Το άλφα (α) του τύπου
                &arg->Q[q_base * arg->d], // 8. Δείκτης: Από πού ξεκινάει να διαβάζει τον Q
                arg->d,          // 9. lda: Leading dimension του Q
                &arg->C[c_base * arg->d], // 10. Δείκτης: Από πού ξεκινάει να διαβάζει τον C
                arg->d,          // 11. ldb: Leading dimension του C
                0.0,             // 12. Το βήτα (β) του τύπου
                dist_matrix,     // 13. Ο πίνακας που θα αποθηκευτούν τα αποτελέσματα
                c_chunk);
            
            
            // --- ΒΗΜΑ Β: ΕΥΚΛΕΙΔΕΙΑ ΑΠΟΣΤΑΣΗ ---
            // Ο dist_matrix τώρα έχει το -2*(εσωτερικό γινόμενο).
            // Πρέπει να προσθέσουμε τα τετράγωνα (Norms) για να βγει η απόσταση.
            for (i = 0; i < q_chunk; i++) {
                for (j = 0; j < c_chunk; j++) {
                    // Εδώ θέλει τους υπολογισμούς των τετραγώνων (normQ + normC)
                    // Έστω ότι τους έχεις προϋπολογίσει, ο τύπος είναι:
                    dist_sq = arg->normQ[q_base + i] + arg->normC[c_base + j] + dist_matrix[i * c_chunk + j];

                    if (dist_sq < 0.0) dist_sq = 0.0;

                    dist_matrix[i * c_chunk + j] = sqrt(fabs(dist_sq)); 
                }
            }

            
            // --- ΒΗΜΑ Γ: QUICK-SELECT / ΕΝΗΜΕΡΩΣΗ ΤΩΝ K ΚΑΛΥΤΕΡΩΝ ---
            // Για κάθε ερώτημα (i) μέσα σε αυτό το μπλοκ...
            for (i = 0; i < q_chunk; i++) {
                // ...συγκρίνουμε τις `c_chunk` νέες αποστάσεις που βρήκαμε,
                // με τους k καλύτερους που έχουμε ήδη στον `local_dst[i * k]`.
                // (Εδώ καλείς μια δική σου συνάρτηση που κρατάει τα top-k, π.χ. με max-heap)
                
                update_top_k(&local_dst[i * arg->k], &local_idx[i * arg->k], 
                            &dist_matrix[i * c_chunk], c_base, c_chunk, arg->k);
            }

        } // Τέλος εσωτερικής λούπας (Τελειώσαμε με όλα τα C για το τρέχον Q_block)

        // 4. ΕΓΓΡΑΦΗ ΣΤΗΝ ΚΕΝΤΡΙΚΗ ΜΝΗΜΗ
        // Αφού είδαμε όλο τον C, οι γείτονες στο local_dst είναι οι ΟΡΙΣΤΙΚΟΙ.
        // Τους περνάμε στον global πίνακα που μας έδωσε η main.
        for (i = 0; i < q_chunk; i++) {
            for (n = 0; n < arg->k; n++) {
                global_pos = (q_base + i) * arg->k + n; // Η ακριβής θέση στο global array
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
    
    // Δεσμεύουμε μνήμη: Ένας αριθμός για κάθε σημείο
    double *norms = (double *)malloc(num_points * sizeof(double));
    
    for (int i = 0; i < num_points; i++) {
        double sum = 0.0;
        // Προσθέτουμε τα τετράγωνα όλων των διαστάσεων του σημείου i
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
    // Δημιουργούμε έναν προσωρινό πίνακα για όλους τους υποψήφιους (k + c_chunk)
    int total_candidates = k + c_chunk;
    Element *candidates = (Element *)malloc(total_candidates * sizeof(Element));

    // 1. Προσθέτουμε τους ήδη υπάρχοντες k καλύτερους
    for (int i = 0; i < k; i++) {
        candidates[i].dist = best_dst[i];
        candidates[i].index = best_idx[i];
    }

    // 2. Προσθέτουμε τους νέους c_chunk υποψήφιους από το τρέχον μπλοκ
    for (int i = 0; i < c_chunk; i++) {
        candidates[k + i].dist = new_dsts[i];
        candidates[k + i].index = c_base + i; // Ο πραγματικός δείκτης στον πίνακα C
    }

    // 3. Χρήση quick-select ή qsort για να βρούμε τα k μικρότερα
    // Εδώ χρησιμοποιούμε qsort για ευκολία, αλλά η λογική του quick-select 
    // (π.χ. std::nth_element σε C++) είναι ταχύτερη για μεγάλα k.
    qsort(candidates, total_candidates, sizeof(Element), compare_elements);

    // 4. Ενημερώνουμε τους τοπικούς πίνακες με τα νέα k καλύτερα
    for (int i = 0; i < k; i++) {
        best_dst[i] = candidates[i].dist;
        best_idx[i] = candidates[i].index;
    }

    free(candidates);
}