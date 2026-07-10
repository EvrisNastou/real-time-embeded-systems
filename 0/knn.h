#ifndef KNN_H
#define KNN_H

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <openblas/cblas.h>

#define BLOCK_SIZE 2000

typedef struct {
    int start_q;       // Από ποιο ερώτημα του Q ξεκινάει αυτό το thread
    int end_q;         // Σε ποιο ερώτημα του Q τελειώνει (αποκλειστικά)
    int num_c;         // Συνολικός αριθμός σημείων στο C (το χρειάζεται για τη λούπα)
    int d;             // Διαστάσεις των σημείων
    int k;             // Πόσους γείτονες ψάχνουμε
    double *C;         // Δείκτης στον πίνακα C (όλος ο πίνακας)
    double *Q;         // Δείκτης στον πίνακα Q (όλος ο πίνακας)
    int *global_idx;   // Δείκτης στον ΤΕΛΙΚΟ πίνακα δεικτών (malloced στη main)
    double *global_dst;// Δείκτης στον ΤΕΛΙΚΟ πίνακα αποστάσεων (malloced στη main)
    double *normQ; // Πίνακας με τα τετράγωνα του Q
    double *normC; // Πίνακας με τα τετράγωνα του C
} ThreadArgs;

typedef struct
{
    int *idx;
    double *dst;
}knnresult;

typedef struct {
    double dist;
    int index;
} Element;


knnresult knnsearch (double *Q, double *C, int num_Q, int num_C, int d, int k, int num_threads);
void *knn_block(void *args);
double* compute_squared_norms(double *P, int num_points, int d);
int compare_elements(const void *a, const void *b);
void update_top_k(double *best_dst, int *best_idx, double *new_dsts, int c_base, int c_chunk, int k);

#endif /* KNN_H */