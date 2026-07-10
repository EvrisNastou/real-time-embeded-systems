#include "knn.h"
#include <sys/time.h> // Για τη σωστή μέτρηση χρόνου με gettimeofday

// Βοηθητική συνάρτηση για αποθήκευση πινάκων double σε αρχείο .bin
void save_double_matrix(const char *filename, double *matrix, int rows, int cols) {
    FILE *f = fopen(filename, "wb");
    if (f == NULL) { printf("Σφάλμα ανοίγματος αρχείου!\n"); exit(1); }
    fwrite(matrix, sizeof(double), rows * cols, f);
    fclose(f);
}

// Βοηθητική συνάρτηση για αποθήκευση του idx (με μετατροπή σε 1-based για MATLAB)
void save_idx_matrix(const char *filename, int *matrix, int rows, int cols) {
    FILE *f = fopen(filename, "wb");
    if (f == NULL) { printf("Σφάλμα ανοίγματος αρχείου!\n"); exit(1); }
    
    // Φτιάχνουμε έναν προσωρινό πίνακα για να προσθέσουμε το +1
    int *matlab_idx = (int *)malloc(rows * cols * sizeof(int));
    for(int i = 0; i < rows * cols; i++) {
        matlab_idx[i] = matrix[i] + 1; // C(0) -> MATLAB(1)
    }
    
    fwrite(matlab_idx, sizeof(int), rows * cols, f);
    free(matlab_idx);
    fclose(f);
}

int main() {
    struct timeval start, end;
    double time_used;
    double *C, *Q;
    int i;

    // ΜΕΓΕΘΗ ΔΕΔΟΜΕΝΩΝ (Αρκετά μεγάλα για να φανεί η διαφορά των threads)
    // 50.000 σημεία με 20 διαστάσεις θέλουν αρκετό χρόνο υπολογισμού.
    int N = 50000; 
    int d = 20;    
    int k = 4;     

    printf("Δημιουργία τυχαίων δεδομένων (N=%d, d=%d)...\n", N, d);
    srand(time(NULL));
    
    // Δέσμευση μνήμης για τον πίνακα C (και Q, αφού Q==C)
    C = (double *)malloc(N * d * sizeof(double));
    Q = (double *)malloc(N * d * sizeof(double));
    for (i = 0; i < N * d; i++) {
        C[i] = (double)rand() / RAND_MAX; // Τυχαίοι αριθμοί 0.0 έως 1.0
        Q[i] = (double)rand() / RAND_MAX; // Τυχαίοι αριθμοί 0.0 έως 1.0
    }
    //Q = C; // Στην άσκησή σου το Q και το C είναι το ίδιο σετ

    // Αποθήκευση του πίνακα C για να τον ανοίξεις μετά στο MATLAB
    save_double_matrix("C_data.bin", C, N, d);
    save_double_matrix("Q_data.bin", Q, N, d);
    printf("Τα δεδομένα αποθηκεύτηκαν στο 'C_data.bin' και 'Q_data.bin'.\n\n");

    // --- ΤΕΣΤ ΜΕ 8 THREADS ---
    printf("Εκτέλεση k-NN με 16 Threads...\n");
    gettimeofday(&start, NULL);
    
    knnresult res16 = knnsearch(Q, C, N, N, d, k, 16);
    
    gettimeofday(&end, NULL);
    time_used = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Χρόνος με 16 Threads: %.4f δευτερόλεπτα\n", time_used);

    // Αποθήκευση των αποτελεσμάτων του 8-thread run για το MATLAB
    save_idx_matrix("idx_results.bin", res16.idx, N, k);
    save_double_matrix("dst_results.bin", res16.dst, N, k);
    printf("Τα αποτελέσματα αποθηκεύτηκαν στα 'idx_results.bin' και 'dst_results.bin'.\n\n");

    free(res16.idx);
    free(res16.dst);
    free(C);
    free(Q);


    /*
    // --- ΤΕΣΤ ΜΕ 1 THREAD (Για σύγκριση) ---
    printf("Εκτέλεση k-NN με 1 Thread (Σειριακά). Παρακαλώ περιμένετε...\n");
    gettimeofday(&start, NULL);
    
    knnresult res1 = knnsearch(Q, C, N, N, d, k, 1);
    
    gettimeofday(&end, NULL);
    time_used = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Χρόνος με 1 Thread: %.4f δευτερόλεπτα\n", time_used);

    free(res1.idx);
    free(res1.dst);
    free(C); // Απελευθερώνουμε τον μεγάλο πίνακα στο τέλος
    */

    return 0;
}