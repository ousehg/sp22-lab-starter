#include "omp_apps.h"

/* -------------------------------Utilties, Do Not Modify------------------------------*/
double* gen_array(int n) {
    double* array = (double*)malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) array[i] = drand48();
    return array;
}

int verify(double* x, double* y, void(*funct)(double *x, double *y, double *z)) {
    double *z_v_add = (double*) malloc(ARRAY_SIZE*sizeof(double));
    double *z_oracle = (double*) malloc(ARRAY_SIZE*sizeof(double));
    (*funct)(x, y, z_v_add);
    for(int i=0; i<ARRAY_SIZE; i++){
        z_oracle[i] = x[i] + y[i];
    }
    for(int i=0; i<ARRAY_SIZE; i++){
        if(z_oracle[i] != z_v_add[i])
            return 0;
    }
    return 1;
}

/* -------------------------------Vector Addition------------------------------*/
void v_add_naive(double* x, double* y, double* z) {
    #pragma omp parallel
    {
        for(int i=0; i<ARRAY_SIZE; i++)
            z[i] = x[i] + y[i];
    }
}

// Adjacent Method
void v_add_optimized_adjacent(double* x, double* y, double* z) {
    // TODO: Implement this function
    // Do NOT use the `for` directive here!
    #pragma omp parallel
    {
        int total_thread = omp_get_num_threads();
        int thread_id = omp_get_thread_num();
        for(int i=0; i<ARRAY_SIZE; i++)
        {
            if((i % total_thread) == thread_id)
            {
                z[i] = x[i] + y[i];
	//	printf("adjacent - index: %i, thread id: %i, total thread: %i \n", i, thread_id, total_thread);
            }
        }
    }
}

// Chunks Method
void v_add_optimized_chunks(double* x, double* y, double* z) {
    // TODO: Implement this function
    // Do NOT use the `for` directive here!
    #pragma omp parallel
    {
        int total_thread = omp_get_num_threads();
        int thread_id = omp_get_thread_num();
        for(int i=0; i<ARRAY_SIZE / total_thread * total_thread; i += total_thread)
        {
            for(int j=i; j<i+total_thread; j++)
            {
                z[j] = x[j] + y[j];
//                printf("z: %i, index: %i, thread id: %i \n", z[j], j, thread_id);
            }
            // tail case
       	    for(int i=ARRAY_SIZE / total_thread; i < ARRAY_SIZE; i++) 
            {
		z[i] = x[i] + y[i]; 
	    }
	}
    }
}

/* -------------------------------Dot Product------------------------------*/
double dotp_naive(double* x, double* y, int arr_size) {
    double global_sum = 0.0;
    #pragma omp parallel
    {
        #pragma omp for
        for (int i = 0; i < arr_size; i++)
            #pragma omp critical
            global_sum += x[i] * y[i];
    }
    return global_sum;
}

// Manual Reduction
double dotp_manual_optimized(double* x, double* y, int arr_size) {
    double global_sum = 0.0;
    // TODO: Implement this function
    // Do NOT use the `reduction` directive here!
    #pragma omp parallel
    {
        for (int i = 0; i < arr_size; i ++)
        {
            int total_thread = omp_get_num_threads();
            int thread_id = omp_get_thread_num();
            if(i % total_thread == thread_id)
            {
                #pragma omp critical
                global_sum += x[i] * y[i];
            }
        }
    }
    return global_sum;
}

// Reduction Keyword
double dotp_reduction_optimized(double* x, double* y, int arr_size) {
    double global_sum = 0.0;
    // TODO: Implement this function
    // Please DO use the `reduction` directive here!
    #pragma omp parallel
    {
        #pragma omp for reduction(+ : global_sum)
        for (int i = 0; i < arr_size; i++)
            global_sum += x[i] * y[i];
    }
    return global_sum;
}

char* compute_dotp(int arr_size) {
    // Generate input vectors
    char* report_buf = (char*)malloc(BUF_SIZE), *pos = report_buf;
    double start_time, run_time;

    double *x = gen_array(arr_size), *y = gen_array(arr_size);
    double serial_result = 0.0, result = 0.0;

    // calculate result serially
    for (int i = 0; i < arr_size; i++) {
        serial_result += x[i] * y[i];
    }

    int num_threads = omp_get_max_threads();
    for (int i = 1; i <= num_threads; i++) {
        omp_set_num_threads(i);
        start_time = omp_get_wtime();
        for (int j = 0; j < REPEAT; j++) result = dotp_manual_optimized(x, y, arr_size);
        run_time = omp_get_wtime() - start_time;
        pos += sprintf(pos, "Manual Optimized: %d thread(s) took %f seconds\n", i, run_time);

        // verify result is correct (within some threshold)
        if (fabs(serial_result - result) > 0.001) {
            pos += sprintf(pos, "Incorrect result!\n");
            *pos = '\0';
            return report_buf;
        }
    }

    for (int i = 1; i <= num_threads; i++) {
        omp_set_num_threads(i);
        start_time = omp_get_wtime();

        for (int j = 0; j < REPEAT; j++) {
            result = dotp_reduction_optimized(x, y, arr_size);
        }

        run_time = omp_get_wtime() - start_time;
        pos += sprintf(pos, "Reduction Optimized: %d thread(s) took %f seconds\n",
                       i, run_time);

        // verify result is correct (within some threshold)
        if (fabs(serial_result - result) > 0.001) {
            pos += sprintf(pos, "Incorrect result!\n");
            *pos = '\0';
            return report_buf;
        }
    }

    // Only run this once because it's too slow..
    omp_set_num_threads(1);
    start_time = omp_get_wtime();
    for (int j = 0; j < REPEAT; j++) result = dotp_naive(x, y, arr_size);
    run_time = omp_get_wtime() - start_time;

    pos += sprintf(pos, "Naive: %d thread(s) took %f seconds\n", 1, run_time);
    *pos = '\0';
    return report_buf;
}
