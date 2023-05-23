#include <iostream>
#include <sys/time.h>
#include <pthread.h>
#include <cassert>
#include <omp.h>
#ifndef NUM_THREAD
#define NUM_THREAD 4
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

void Parallel()
{
    //loop over all diagonal (pivot) elements
    #pragma omp parallel num_threads(NUM_THREAD)
    for (int k = 0; k < n; k++)
    {
        //for all elements in the row of, and to the right of pivot
        #pragma omp single
        {
        for (int j = k + 1; j < n; j++)
        {
            //divide by pivot
            A[k][j] = A[k][j] * 1.0 / A[k][k];
        }
        A[k][k] = 1.0;
        }
        //for all rows below the  pivot row
        #pragma omp for schedule(static)
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

    Parallel();

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

