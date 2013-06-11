#include "../defs.h"
#include "uts.h"

#ifndef TIMES
#define TIMES 1
#endif

#ifndef PARALLEL
  #define COMPILER_TYPE 0
  #define PARALLEL 0
#else
  #define COMPILER_TYPE 5
#endif

#ifdef CUTOFF_DEPTH
  #define DO_CUTOFF 1
#else
  #define DO_CUTOFF 0
  #define CUTOFF_DEPTH 10000
#endif

#ifndef MAX_THREADS 
#define MAX_THREADS 256
#endif

int pthread_num_threads = -1;

void impl_abort(int err) {
  exit(err);
}

const char * impl_getName() {
  switch(COMPILER_TYPE) {
  case 0: return "Sequential C";
  case 5: return "TBB Version";
  default: return "Unsupported Compiler";
  }
}

// construct string with all parameter settings 
int impl_paramsToStr(char *strBuf, int ind) {
  ind += sprintf(strBuf+ind, "Execution strategy:  ");
  if (PARALLEL) {
    ind += sprintf(strBuf+ind, "Parallel search using %d threads\n", pthread_num_threads);
    //if (doSteal) {
      //ind += sprintf(strBuf+ind, "   Load balance by work stealing, chunk size = %d nodes\n",chunkSize);
      //ind += sprintf(strBuf+ind, "  CBarrier Interval: %d\n", cbint);
      //ind += sprintf(strBuf+ind, "   Polling Interval: %d\n", pollint);
    //}
    //else
    //  ind += sprintf(strBuf+ind, "   No load balancing.\n");
  }
  else
    ind += sprintf(strBuf+ind, "Iterative sequential search\n");

  return ind;
}

void impl_helpMessage() {
  if (PARALLEL) {
    printf("   -s  int   zero/nonzero to disable/enable work stealing\n");
    printf("   -c  int   chunksize for work stealing\n");
    printf("   -i  int   set cancellable barrier polling interval\n");
    printf("   -T  int   set number of threads\n");
  } else {
#ifdef UTS_STAT
    printf("   -u  int   unbalance measure (-1: none; 0: min/size; 1: min/n; 2: max/n)\n");
#else
    printf("   none.\n");
#endif
  }
}

int impl_parseParam(char *param, char *value) {
  int err = 0;  // Return 0 on a match, nonzero on an error

  switch (param[1]) {
#if (PARALLEL == 1)
    //case 'c':
    //  chunkSize = atoi(value); break;
    //case 's':
    //  doSteal = atoi(value);
    //  if (doSteal != 1 && doSteal != 0)
    //err = 1;
    //  break;
    //case 'i':
    //  cbint = atoi(value); break;
    case 'T':
      pthread_num_threads = atoi(value);
      if (pthread_num_threads > MAX_THREADS) {
        printf("Warning: Requested threads > MAX_THREADS.  Truncated to %d threads\n", MAX_THREADS);
        pthread_num_threads = MAX_THREADS;
      }
      break;
#else /* !PARALLEL */
#ifdef UTS_STAT
    case 'u':
      unbType = atoi(value);
      if (unbType > 2) {
        err = 1;
        break;
      }
      if (unbType < 0)
        stats = 0;
      else
        stats = 1;
      break;
#endif
#endif /* PARALLEL */
    default:
      err = 1;
      break;
  }

  return err;
}


Node *genChildren(Node *parent) {
  int parentHeight = parent->height;
  int numChildren, childType;

  numChildren = uts_numChildren(parent);
  childType   = uts_childType(parent);

  // record number of children in parent
  parent->numChildren = numChildren;

  // construct children and push onto stack
  if (numChildren > 0) {
    int i, j;
    //Node *kids = new Node[numChildren]();
    Node *kids = (Node*) scalable_malloc(numChildren * sizeof(Node));
    //Node *kids = (Node*) malloc(numChildren*sizeof(Node));
    int childHeight = parentHeight + 1;

    for (i = 0; i < numChildren; i++) {
      for (j = 0; j < computeGranularity; j++) {
        kids[i].type = childType;
        kids[i].height = childHeight;
        // TBD:  add parent height to spawn
        // computeGranularity controls number of rng_spawn calls per node
        rng_spawn(parent->state.state, kids[i].state.state, i); // AT: reads parent state and i and computes child state
      }
    }
    return kids;
  } else
    return 0;
}

long parTreeSearch(Node *n, int depth);

// The reduce operation updates local state that is then combined 
// with parent task
class ReduceBody {
public:
    long value;
    int depth;
    Node *kids;

    // Constructor
    ReduceBody(int depth, Node* kids): value(0), depth(depth), kids(kids) {}

    // Split Constructor
    ReduceBody(ReduceBody &b, tbb::split) { value = 0; depth = b.depth; kids = b.kids; }

    // Reduce operation
    void operator () (const tbb::blocked_range<int> &range) {
        int i;
        int tmp = 0;
        for(i=range.begin(); i<range.end(); i++) {
            tmp += parTreeSearch(&kids[i], depth+1);
        }
        value += tmp;
    }

    // Join Operation
    void join (ReduceBody &rhs) { value += rhs.value;}

};

long serTreeSearch(Node *node) {
  long nNodes = 1;
  Node *kids = genChildren(node);
  int i;
  //printf("DEBUG:: genChildren::numKids = %d\n", node->numChildren);
  for(i=0; i < node->numChildren; ++i) {
     //printf("DEBUG:: visiting kid#%d (%p)\n", i, &kids[i]);
     nNodes += serTreeSearch(&kids[i]);
  }
  //delete kids;
  scalable_free(kids);
  return nNodes;
}

long parTreeSearch(Node *node, int depth) {
  if (!DO_CUTOFF || depth < CUTOFF_DEPTH) {
    // Parallel
    long nNodes = 1;
    Node *kids = genChildren(node);
    if (node->numChildren > 1) {
      PARTITIONER partitioner;
      ReduceBody reduce(depth, kids);
      tbb::parallel_reduce(tbb::blocked_range<int>(0, node->numChildren, 1), reduce, partitioner);
      nNodes += reduce.value;
    } else if (node->numChildren == 1) {
      nNodes += parTreeSearch(kids, depth+1);
    }
    //delete kids;
    scalable_free(kids);
    return nNodes;
  } else {
    // Sequential
    return serTreeSearch(node);
  }
}


int main(int argc, char *argv[]) {
  Node   root;
  int    err;
  void  *rval;

  // parse args
  uts_parseParams(argc, argv);

  // initialize root node 
  uts_initRoot(&root, type);
 
  int defth = tbb::task_scheduler_init::default_num_threads();
  if (pthread_num_threads < 0) 
    pthread_num_threads = defth;
  uts_printParams();
  //printf("Default #Threads=%d. Using %d threads\n", defth, nth);
  tbb::task_scheduler_init init(pthread_num_threads);

  // start timing 
  tbb::tick_count t0 = tbb::tick_count::now();
  //printf("DEBUG:: ready to actually search the tree\n");
  int i;
  int result = 0;
  for (i=0; i<TIMES; i++) {
#if PARALLEL
    result = parTreeSearch(&root, 0);
#else
    result = serTreeSearch(&root);
#endif
  }
  
  tbb::tick_count t1 = tbb::tick_count::now();
  printf("Ticks = %f\n", (t1-t0).seconds());
  printf("Number of Nodes = %d\n", result);

  //showStats((t1-t0).seconds());

  // Dump result
  hexdump_var("result.hex", (int*)&result, 1);

  return 0;
}
