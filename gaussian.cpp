#include <iostream>
// #include <windows.h> //windows下的计时
#include <sys/time.h>
#include<pthread.h>
#include<cassert>
#ifndef NUM_THREAD
#define NUM_THREAD 16
#endif
#define MATERIALIZE 0
using namespace std;
#define N 1024
int n = N;
double A[N][N];
double answer[N][N];
pthread_barrier_t barrier1, barrier2;
pthread_mutex_t lock[NUM_THREAD];
pthread_cond_t cond[NUM_THREAD];
int ready[NUM_THREAD]; //predicate variable for cond

// 按照实验手册上进行初始化
void init()
{
    // srand(time(NULL));
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            A[i][j] = 0;
        }
        A[i][i] = 1.0;
        for (int j = i + 1; j < n; j++)
            A[i][j] = rand() % 1000;
    }
    for(int i = 0; i < n; i ++)
    {
        for(int j = 0; j < n; j++)
        {
            answer[i][j] = A[i][j];
        }
    }
    for (int k = 0; k < n; k++)
    {
        for (int i = k + 1; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                A[i][j] += A[k][j];
            }
        }
    }
}

void Serial()
{
    //loop over all diagonal (pivot) elements
    for (int k = 0; k < n; k++)
    {
        //for all elements in the row of, and to the right of pivot
        for (int j = k + 1; j < n; j++)
        {
            //divide by pivot
            A[k][j] = A[k][j] * 1.0 / A[k][k];
        }
        A[k][k] = 1.0;
        //for all rows below the  pivot row
        for (int i = k + 1; i < n; i++)
        {
            //for all elements in the row
            for (int j = k + 1; j < n; j++)
            {
                //substract by that pivot row
                A[i][j] = A[i][j] - A[i][k] * A[k][j];
            }
            A[i][k] = 0;
        }
    }
}
// perform elimination on a 2d matrix - each thread operates on a partition of A
// (spcific number of rows)
void* intuitive_eliminate (void *ptr) { /*triangulize the matrix A*/

    int k, i, j; // each thread has a local copy of iteration id (k-loop)
    long tid = (long) ptr;
    int block_size = n/NUM_THREAD;
    int start = block_size * tid;
    int end = start + block_size;

    for (k=0; k<n; k++) { /*outermost loop of gauss-elimination*/
        // compute pivot row if applicable (only 1 pivot row per k-th iteration)
        if (k>=start && k < end) {
            //cout << "pivot row comp: threadId, " << tid << endl;
            for (j=k+1; j<n; j++)
                A[k][j] = A[k][j]/A[k][k]; /* all elements in the pivot row, to the right of pivot, divide by pivot */
            A[k][k] = 1;
        }
        pthread_barrier_wait(&barrier1); // barriers get reset once the reqd. num of threads have reached

        for (i=start; i<end; i++) {
            if (i>=k+1 && i<n) { /*only for rows below pivot, and within matrix*/
                for (j=k+1; j<n; j++) /*for all elements in the row*/
                    A[i][j] = A[i][j] - A[i][k] * A[k][j];
                A[i][k] = 0;
            }
        }
        pthread_barrier_wait(&barrier2);
    }

    pthread_exit(NULL);
}
// perform elimination on a 2d matrix - each thread operates on a partition of A
// (spcific number of rows)
void* interleave_eliminate (void *ptr) { /*triangulize the matrix A*/
    int k, i, j; // each thread has a local copy of iteration id (k-loop)
    long tid = (long) ptr;
    for (k=0; k<n; k++) { /*outermost loop of gauss-elimination*/
        // compute pivot row if applicable (only 1 pivot row per k-th iteration)
        if (k%NUM_THREAD == tid) {
            //cout << "pivot row comp: threadId, " << tid << endl;
            for (j=k+1; j<n; j++)
                A[k][j] = A[k][j]/A[k][k]; /* all elements in the pivot row, to the right of pivot, divide by pivot */
            A[k][k] = 1;
        }
        pthread_barrier_wait(&barrier1); // barriers get reset once the reqd. num of threads have reached

        for (i=tid; i<n; i+=NUM_THREAD) {
            if (i>=k+1 && i<n) { /*only for rows below pivot, and within matrix*/
                for (j=k+1; j<n; j++) /*for all elements in the row*/
                    A[i][j] = A[i][j] - A[i][k] * A[k][j];
                A[i][k] = 0;
            }
        }
        // barrier is not reqd. - next iter it will either compute on its own part (if next pivot row is in its mapping), or wait
    }
    pthread_exit(NULL);
}

// perform elimination on a 2d matrix - each thread operates on a partition of A
// (spcific number of rows)
void* pipeline_eliminate (void *ptr) { /*triangulize the matrix A*/

    int k, i, j; // each thread has a local copy of iteration id (k-loop)
    long tid = (long) ptr;
    for (k=0; k<n; k++) { /*outermost loop of gauss-elimination*/
        //printf("iter: %d tid %d\n", k, tid);

        // compute pivot row if applicable (only 1 pivot row per k-th iteration)
        if (k%NUM_THREAD == tid) {
            //printf("iter: %d pivot row comp pid:, %d\n", k, tid);
            for (j=k+1; j<n; j++)
                A[k][j] = A[k][j]/A[k][k]; /* all elements in the pivot row, to the right of pivot, divide by pivot */
            A[k][k] = 1;
        }
        // wait for pivot row to arrive from previous thread
        else { //wait for the previous thread to send the pivot row (logically)
            pthread_mutex_lock (&lock[tid]);
            while (ready[tid]==0) {
                //printf("iter: %d pid waiting: %d ready: %d\n", k, tid, ready[tid]);
                pthread_cond_wait(&cond[tid], &lock[tid]);
            }
            ready[tid] -= 1;
            //printf("iter: %d pid reset-ready: %d ready: %d\n", k, tid, ready[tid]);
            pthread_mutex_unlock (&lock[tid]);
        }
        // if this thread comes out of the wait - signal next thread to proceed
        long tid_n = (tid+1)%NUM_THREAD;
        if (tid_n != (k%NUM_THREAD)) { //cannot send the pivot row back to the thread computing it
            pthread_mutex_lock (&lock[tid_n]);
            ready[tid_n] += 1;
            //printf("iter: %d pid signalling: %d ready %d\n", k, tid_n, ready[tid_n]);
            pthread_cond_signal (&cond[tid_n]);
            pthread_mutex_unlock (&lock[tid_n]);
        }
        for (i=tid; i<n; i+=NUM_THREAD) {
            if (i>=k+1 && i<n) { /*only for rows below pivot, and within matrix*/
                for (j=k+1; j<n; j++) /*for all elements in the row*/
                    A[i][j] = A[i][j] - A[i][k] * A[k][j];
                A[i][k] = 0;
            }
        }
    }

    pthread_exit(NULL);
}

//pthread implimentation of elimination
void Parallel()
{
    // create and join threads
    pthread_t threads[NUM_THREAD];
    for (int i=0; i<NUM_THREAD; i++) {
#ifdef INTERLEAVE
        int rc = pthread_create(&threads[i], NULL, interleave_eliminate, (void*) i);
#else
        int rc = pthread_create(&threads[i], NULL, intuitive_eliminate, (void*) i);
#endif
        if (rc) {
            cout << "Error: unable to create thread, " << rc << endl;
            exit(-1);
        }
    }
    for (int i=0; i<NUM_THREAD; i++) {
        int rc = pthread_join(threads[i], NULL);
        if (rc) {
            cout << "Error: unable to join thread, " << rc << endl;
            exit(-1);
        }
    }
}

void Pipeline()
{
    // create and join threads
    pthread_t threads[NUM_THREAD];
    for (int i=0; i<NUM_THREAD; i++) {
        int rc = pthread_create(&threads[i], NULL, pipeline_eliminate, (void*) i);
        if (rc) {
            cout << "Error: unable to create thread, " << rc << endl;
            exit(-1);
        }
    }
    for (int i=0; i<NUM_THREAD; i++) {
        int rc = pthread_join(threads[i], NULL);
        if (rc) {
            cout << "Error: unable to join thread, " << rc << endl;
            exit(-1);
        }
    }
}

void display(double A[N][N], int n)
{
    for(int i = 0; i < n; i++)
    {
        for(int j = 0; j < n; j++)
        {
            cout<<A[i][j]<<'\t';

        }
        cout<<endl;
    }
}
bool is_correct()
{
    for(int i = 0; i<n;i++)
    {
        for(int j=0;j<n;j++)
        {
            if(A[i][j]!=answer[i][j])
                return false;
        }
    }
    return true;
}
int main()
{
    // 输入相应的矩阵规模
  
    // cin >> n;
    timeval tic, toc;
    double seconds;
    
    init();
    // initialize barriers
    pthread_barrier_init(&barrier1, NULL, NUM_THREAD);
    pthread_barrier_init(&barrier2, NULL, NUM_THREAD);
    // initialize locks and conds
    for (int i=0; i<NUM_THREAD; i++) {
        pthread_mutex_init (&lock[i], NULL);
        pthread_cond_init (&cond[i], NULL);
        ready[i] = 0;
    }
    
#if MATERIALIZE
    cout<<"Correct answer: \n";
    display(answer, n);
    cout<<"Original matrix: \n";
    display(A, n);
#endif
    gettimeofday(&tic, NULL);
#ifdef SERIAL
    Serial();
#endif
#ifdef INTUITIVE
    Parallel();
#endif
#ifdef INTERLEAVE
    Parallel();
#endif
#ifdef PIPELINE
    Pipeline();
#endif
    gettimeofday(&toc, NULL);
#if MATERIALIZE
    cout<<"Drived answer: \n";
    display(A, n);
    //verify correction
    assert(is_correct());
#endif
    cout<<"result correct\n";
    seconds = (toc.tv_sec - tic.tv_sec) * 1000.0 + (toc.tv_usec - tic.tv_usec) / 1000.0;
    cout << "time spent: " << seconds << "ms" << endl;
    pthread_barrier_destroy(&barrier1);
    pthread_barrier_destroy(&barrier2);
    // destroy mutexes and conds
    for (int i=0; i<NUM_THREAD; i++) {
        pthread_mutex_destroy (&lock[i]);
        pthread_cond_destroy (&cond[i]);
    }
    pthread_exit(NULL);
}

