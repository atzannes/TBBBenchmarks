#include <stdio.h>
#include <stdlib.h>
#include "tbb/blocked_range.h"
#include "tbb/blocked_range2d.h"
#include "tbb/tick_count.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_for_meld.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/scalable_allocator.h"
#include "tbb/parallel_invoke.h"
#include "tbb/parallel_reduce.h"
#include "tbb/mutex.h"
#include "tbb/atomic.h"
#include "tbb/combinable.h"
//#include "../xboutils/xboutils.h"
#include "xboutils/xboutils.h"

#ifndef PARTITIONER
  #if defined SIMPLE_PARTITIONER
    #define PARTITIONER tbb::simple_partitioner
  #else /* ndef SIMPLE_PARTITIONER */
    #if defined  AUTO_PARTITIONER
      #define PARTITIONER tbb::auto_partitioner
    #else /* ndef ( SIMPLE_PARTITIONER || AUTO_PARTITIONER )*/
      #if defined AFFINITY_PARTITIONER
        #define PARTITIONER tbb::affinity_partitioner
      #else /* ndef ( SIMPLE_PARTITIONER || AUTO_PARTITIONER || AFFINITY_PARTITIONER) */
        #define PARTITIONER tbb::auto_partitioner
      #endif /* def AFFINITY_PARTITIONER */
    #endif /* def AUTO_PARTITIONER */
  #endif /* def SIMPLE_PARTITIONER */
#endif /* ndef PARTITIONER */

#ifndef TIMES
#define TIMES 1
#endif

#ifdef DEQUE_THRESH
#define DEQUE_CHECK_CODE(task) { if ( task->task_pool_size() < DEQUE_THRESH) break; }
#else
#define DEQUE_CHECK_CODE(task) { if ( task->is_task_pool_empty() ) break; }
#endif

