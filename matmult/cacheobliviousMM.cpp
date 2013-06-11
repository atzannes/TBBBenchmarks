/* 
 * Rectangular matrix multiplication.
 *
 * See the paper ``Cache-Oblivious Algorithms'', by
 * Matteo Frigo, Charles E. Leiserson, Harald Prokop, and 
 * Sridhar Ramachandran, FOCS 1999, for an explanation of
 * why this algorithm is good for caches.
 *
 * Author: Matteo Frigo
 */
static const char *ident __attribute__((__unused__))
     = "$HeadURL: https://bradley.csail.mit.edu/svn/repos/cilk/5.4.3/examples/matmul.cilk $ $LastChangedBy: sukhaj $ $Rev: 517 $ $Date: 2003-10-27 10:05:37 -0500 (Mon, 27 Oct 2003) $";

/*
 * Copyright (c) 2003 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/// Code to set thread affinity (on Linux)
#include <unistd.h>
#include <sys/types.h>
#include <linux/unistd.h>

pid_t gettid() { return (pid_t)syscall(__NR_gettid); }

#define GET_MASK(cpu_set) (*(unsigned*)(void*)&cpu_set)
#define RES_STAT(res) (res != 0 ? "failed" : "ok")

    class AffinityHelper {
        static cpu_set_t m_processMask;

        class Initializer {
        public:
            Initializer () {
                CPU_ZERO (&m_processMask);
                int res = sched_getaffinity( getpid(), sizeof(cpu_set_t), &m_processMask );
                ASSERT ( res == 0, "sched_getaffinity failed" );
            }
        }; // class AffinityHelper::Initializer

        static Initializer m_initializer;

    public:
        static const cpu_set_t& ProcessMask () { return m_processMask; }
    }; // class AffinityHelper

    cpu_set_t AffinityHelper::m_processMask;
    AffinityHelper::Initializer AffinityHelper::m_initializer;

    bool PinTheThread ( int cpu_idx, tbb::atomic<int>& nThreads ) {
        cpu_set_t orig_mask, target_mask;
        CPU_ZERO( &target_mask );
        CPU_SET( cpu_idx, &target_mask );
        ASSERT ( CPU_ISSET(cpu_idx, &target_mask), "CPU_SET failed" );
        CPU_ZERO( &orig_mask );
        int res = sched_getaffinity( gettid(), sizeof(cpu_set_t), &orig_mask );
        ASSERT ( res == 0, "sched_getaffinity failed" );
        res = sched_setaffinity( gettid(), sizeof(cpu_set_t), &target_mask );
        ASSERT ( res == 0, "sched_setaffinity failed" );
        --nThreads;
        while ( nThreads )
            __TBB_Yield();
        return true;
    }

    class AffinitySetterTask : tbb::task {
        static bool m_result;
        static tbb::atomic<int> m_nThreads;
        int m_idx;

        tbb::task* execute () {
            //TestAffinityOps();
            m_result = PinTheThread( m_idx, m_nThreads );
            return NULL;
        }

    public:
        AffinitySetterTask ( int idx ) : m_idx(idx) {}

        friend bool AffinitizeTBB ( int, int /*mode*/ );
    }; // end class AffinitySetterTask

    bool AffinitySetterTask::m_result = true;
    tbb::atomic<int> AffinitySetterTask::m_nThreads;

    bool AffinitizeTBB ( int p, int affMode ) {
        AffinitySetterTask::m_result = true;
        AffinitySetterTask::m_nThreads = p;
        tbb::task_list  tl;
        for ( int i = 0; i < p; ++i ) {
            tbb::task &t = *new( tbb::task::allocate_root() ) AffinitySetterTask( affMode == amSparse ? i * NumCpus / p : i );
            t.set_affinity( tbb::task::affinity_id(i + 1) );
            tl.push_back( t );
        }
        tbb::task::spawn_root_and_wait(tl);
        return AffinitySetterTask::m_result;
    }

    inline
    void Affinitize ( int p, int affMode ) {
        if ( !AffinitizeTBB (p, affMode) )
            REPORT("Warning: Failed to set affinity for %d TBB threads\n", p);
    }



#define TBB_PREVIEW_LOCAL_OBSERVER 1
#define TBB_PREVIEW_TASK_ARENA 1
#include "../defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef REAL
  //#define REAL int
  #define REAL double
  //#define REAL float
#endif

#define spawn 

#ifndef SIZE
#define SIZE 1024
#endif

#ifndef SEED
#define SEED 0
#endif

#ifndef CUTOFF
#define CUTOFF 64
#endif

void zero(REAL *A, int n)
{
     int i, j;
     
     for (i = 0; i < n; i++) {
	   for (j = 0; j < n; j++) {
	       A[i * n + j] = 0.0;
	   }
     }
}

void initSquare(REAL *A, int n)
{
    int i, j;
     
    for (i = 0; i < n; i++) {
	  for (j = 0; j < n; j++) {
	      A[i * n + j] = (double) rand();
	  }
    }
}

double maxerror(REAL *A, REAL *B, int n)
{
     int i, j;
     double error = 0.0;
     
     for (i = 0; i < n; i++) {
	  for (j = 0; j < n; j++) {
	       double diff = (A[i * n + j] - B[i * n + j]) / A[i * n + j];
	       if (diff < 0)
		    diff = -diff;
	       if (diff > error)
		    error = diff;
	  }
     }
     return error;
}

void iter_matmul(REAL *A, REAL *B, REAL *C, int n)
{
     int i, j, k;
     
     for (i = 0; i < n; i++)
	  for (k = 0; k < n; k++) {
	       REAL c = 0.0;
	       for (j = 0; j < n; j++)
		    c += A[i * n + j] * B[j * n + k];
	       C[i * n + k] = c;
	  }
}

/*
 * A \in M(m, n)
 * B \in M(n, p)
 * C \in M(m, p)
 * @param ld: row size of the original arrays
 * @param add = 1 if we are adding to C[i][j] and 0 if we are simply assigning
 */
void ser_rec_matmul(REAL *A, REAL *B, REAL *C, int m, int n, int p, int ld,
		     int add)
{
     if ((m + n + p) <= CUTOFF) {
	  int i, j, k;
	  /* base case */
	  if (add) {
	       for (i = 0; i < m; i++)
		    for (k = 0; k < p; k++) {
			 REAL c = 0.0;
			 for (j = 0; j < n; j++)
			      c += A[i * ld + j] * B[j * ld + k];
			 C[i * ld + k] += c;
		    }
	  } else {
	       for (i = 0; i < m; i++)
		    for (k = 0; k < p; k++) {
			 REAL c = 0.0;
			 for (j = 0; j < n; j++)
			      c += A[i * ld + j] * B[j * ld + k];
			 C[i * ld + k] = c;
		    }
	  }
     } else if (m >= n && n >= p) {
      ///   / A1 \         / A1 B \
      ///  |      | B  =  |        |
      ///   \ A2 /         \ A2 B /
	  int m1 = m >> 1;
	  //spawn rec_matmul(A, B, C, m1, n, p, ld, add);
	  //spawn rec_matmul(A + m1 * ld, B, C + m1 * ld, m - m1,
	  //		   n, p, ld, add);
      ser_rec_matmul(A, B, C, m1, n, p, ld, add);
      ser_rec_matmul(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld, add); 
     } else if (n >= m && n >= p) {
      ///           / B1 \
      ///  (A1 A2) |      |  =  A1 B1 + A2 B2
      ///           \ B2 /
	  int n1 = n >> 1;
      /// FIXME synchronization may be too aggressive here...
	  ///spawn rec_matmul(A, B, C, m, n1, p, ld, add);
	  ///sync;
	  ///spawn rec_matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld, 1);
	  ser_rec_matmul(A, B, C, m, n1, p, ld, add);
	  ser_rec_matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld, 1);
     } else {
      ///
      ///  A (B1 B2)  = (A B1   A B2)
      ///
	  int p1 = p >> 1;
	  //spawn rec_matmul(A, B, C, m, n, p1, ld, add);
	  //spawn rec_matmul(A, B + p1, C + p1, m, n, p - p1, ld, add);
      ser_rec_matmul(A, B, C, m, n, p1, ld, add);
      ser_rec_matmul(A, B + p1, C + p1, m, n, p - p1, ld, add);
     }
}

void rec_matmul(REAL *A, REAL *B, REAL *C, int m, int n, int p, int ld, int add);

class RecMatmulFunctor {
  REAL *A, *B, *C;
  int m, n, p, ld, add;
public:
  RecMatmulFunctor(REAL *A, REAL *B, REAL *C, 
                    int m, int n, int p, int ld, int add)
                   : A(A), B(B), C(C), m(m), n(n), p(p), ld(ld), add(add) {}
  void operator () () const { rec_matmul(A, B, C, m, n, p, ld, add);}
                    
};

class SplitVerticallyBody {
  REAL *A, *B, *C;
  int m, n, p, ld, add, m1;
public:
  SplitVerticallyBody(REAL *A, REAL *B, REAL *C, 
                    int m, int n, int p, int ld, int add)
                   : A(A), B(B), C(C), m(m), n(n), p(p), ld(ld), add(add) 
  { m1 = m >> 1; }

  void operator () (const tbb::blocked_range<int> &range) const {
    if (range.begin() == 0)
	  rec_matmul(A, B, C, m1, n, p, ld, add);
    else
	  rec_matmul(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld, add);
  }
};

class SplitHorizontallyBody {
  REAL *A, *B, *C;
  int m, n, p, ld, add, p1;
public:
  SplitHorizontallyBody(REAL *A, REAL *B, REAL *C, 
                    int m, int n, int p, int ld, int add)
                   : A(A), B(B), C(C), m(m), n(n), p(p), ld(ld), add(add) 
  { p1 = p >> 1; }

  void operator () (const tbb::blocked_range<int> &range) const {
    if (range.begin() == 0)
	  rec_matmul(A, B, C, m, n, p1, ld, add);
    else
      rec_matmul(A, B + p1, C + p1, m, n, p - p1, ld, add);
  }
};

/*
 * A \in M(m, n)
 * B \in M(n, p)
 * C \in M(m, p)
 * @param ld: row size of the original arrays
 * @param add = 1 if we are adding to C[i][j] and 0 if we are simply assigning
 */
void rec_matmul(REAL *A, REAL *B, REAL *C, int m, int n, int p, int ld, int add)
{
     if ((m + n + p) <= CUTOFF) {
	  int i, j, k;
	  /* base case */
	  if (add) {
	       for (i = 0; i < m; i++)
		    for (k = 0; k < p; k++) {
			 REAL c = 0.0;
			 for (j = 0; j < n; j++)
			      c += A[i * ld + j] * B[j * ld + k];
			 C[i * ld + k] += c;
		    }
	  } else {
	       for (i = 0; i < m; i++)
		    for (k = 0; k < p; k++) {
			 REAL c = 0.0;
			 for (j = 0; j < n; j++)
			      c += A[i * ld + j] * B[j * ld + k];
			 C[i * ld + k] = c;
		    }
	  }
     } else if (m >= n && n >= p) {
      ///   / A1 \         / A1 B \
      ///  |      | B  =  |        |
      ///   \ A2 /         \ A2 B /
      #ifdef TBB_INVOKE
	  int m1 = m >> 1;
	  //spawn rec_matmul(A, B, C, m1, n, p, ld, add);
	  //spawn rec_matmul(A + m1 * ld, B, C + m1 * ld, m - m1,
	  //		   n, p, ld, add);
      /////////////////////////////////////////////////
      /// TBB:INVOKE
      RecMatmulFunctor Top(A, B, C, m1, n, p, ld, add);
      RecMatmulFunctor Bottom(A + m1*ld, B, C + m1*ld, m-m1, n, p, ld, add);
      tbb::parallel_invoke( Top, Bottom);
      #else 
      /////////////////////////////////////////////////
      /// TBB:PARALLEL_FOR
      SplitVerticallyBody body(A, B, C, m, n, p, ld, add);
      tbb::simple_partitioner partitioner;
      tbb::parallel_for (tbb::blocked_range<int> (0, 2, 1), body, partitioner);
      #endif
     } else if (n >= m && n >= p) {
      ///           / B1 \
      ///  (A1 A2) |      |  =  A1 B1 + A2 B2
      ///           \ B2 /
	  int n1 = n >> 1;
      /// FIXME synchronization may be too aggressive here...
	  ///spawn rec_matmul(A, B, C, m, n1, p, ld, add);
	  ///sync;
	  ///spawn rec_matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld, 1);
	  rec_matmul(A, B, C, m, n1, p, ld, add);
	  rec_matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld, 1);
     } else {
      ///
      ///  A (B1 B2)  = (A B1   A B2)
      ///
      #ifdef TBB_INVOKE
	  int p1 = p >> 1;
	  //spawn rec_matmul(A, B, C, m, n, p1, ld, add);
	  //spawn rec_matmul(A, B + p1, C + p1, m, n, p - p1, ld, add);
      RecMatmulFunctor Left(A, B, C, m, n, p1, ld, add);
      RecMatmulFunctor Right(A, B + p1, C + p1, m, n, p - p1, ld, add);
      tbb::parallel_invoke( Left, Right);
      #else
      /////////////////////////////////////////////////
      /// TBB:PARALLEL_FOR
      SplitHorizontallyBody body(A, B, C, m, n, p, ld, add);
      tbb::simple_partitioner partitioner;
      tbb::parallel_for (tbb::blocked_range<int>(0, 2, 1), body, partitioner);
      #endif
     }
}

//#define __TBB_SCHEDULER_OBSERVER
//#include "tbb/task_scheduler_observer.h"
//#include "tbb/task_arena.h"
#define TBB_OBSERVER tbb::task_scheduler_observer 
#define TBB_ARENA tbb::task_arena
#define AFFINITY_MASK cpu_set_t

class pinning_observer: public TBB_OBSERVER {
public:
  AFFINITY_MASK m_mask; // HW affinity mask to be used with an arena
  pinning_observer( AFFINITY_MASK mask ) : m_mask(mask) { }
  pinning_observer( TBB_ARENA &a, AFFINITY_MASK mask )
    : TBB_OBSERVER(a), m_mask(mask) {
    observe(true); // activate the observer
  }
  /*override*/ void on_scheduler_entry( bool worker ) {
    sched_aff(TBB_ARENA::current_slot(), m_mask); 
  }
};




int main(int argc, char *argv[])
{
    int n;
    REAL *A, *B, *C1, *C2;
    double err = -1;
    int nth=-1; // number of (hardware) threads (-1 means undefined)
    if (argc > 1) { //command line with parameters
        if (argc > 2) {
            printf("ERROR: wrong use of command line arguments. Usage %s <#threads>\n", argv[0]);
            return 1;
        } else {
            nth = atoi( argv[1] );
        }
    }
    int defth = tbb::task_scheduler_init::default_num_threads();
    if (nth<0) nth=defth;
    printf("Default #Threads=%d. Using %d threads\n", defth, nth);
    pinning_observer po(
    tbb::task_scheduler_init init(nth);

    n = SIZE;

    A = (REAL*) malloc(n * n * sizeof(REAL));
    B = (REAL*) malloc(n * n * sizeof(REAL));
    C1 = (REAL*) malloc(n * n * sizeof(REAL));
    C2 = (REAL*) malloc(n * n * sizeof(REAL));
	  
     srand(SEED);
     initSquare(A, n);
     initSquare(B, n);
     zero(C1, n);
     zero(C2, n);

     /// Compute Reference Solution
     //iter_matmul(A, B, C1, n);

     /* Timing. "Start" timers */
     tbb::tick_count t0 = tbb::tick_count::now();

     #ifdef SERIAL
     ser_rec_matmul(A, B, C2, n, n, n, n, 0);
     #else
     rec_matmul(A, B, C2, n, n, n, n, 0); 
     #endif

     /* Timing. "Stop" timers */
     tbb::tick_count t1 = tbb::tick_count::now();
     double elapsed = (t1-t0).seconds();
     printf("Ticks = %f\n", elapsed);

     //err = maxerror(C1, C2, n);

     printf("Max error     = %g\n", err);
     printf("Options: size = %d\n", n);
     printf("``MFLOPS''    = %4f\n\n",
	    2.0 * n * n * n / ( elapsed ));

     free(C2);
     free(C1);
     free(B);
     free(A);
     return 0;
}
