/* BFS Serial implementation 
 * Author: George Caragea
 * Description: Serially going thru the edges in one level, by vertex
 * Version: $Id: bfs_serial.c 1430 2006-08-04 20:02:15Z george $
 */

#include "../defs.h"

#ifndef TIMES
#define TIMES 1
#endif

#ifndef OUTER_GRAINSIZE
#define OUTER_GRAINSIZE 1
#endif

#ifndef INNER_GRAINSIZE
#define INNER_GRAINSIZE 1
#endif

tbb::atomic<int> newLevelIndex;
tbb::atomic<int> gatekeeper[VSIZE];


#ifndef START
#define START 0
#endif

int mytempB[VSIZE];
int mytempC[VSIZE];

int * currentLevelSet, * newLevelSet, *tmpSet;

int currentLevel;
int currentLevelSize,newLevelSize;

int level[VSIZE] = {0};

/* vars to be initialized from .xbo file or input*/
int vertices[VSIZE];
int degrees[VSIZE];
#ifdef XBOFILE
int edges[ESIZE][2];
#else
int edges[ESIZE];
#endif


#include "read_input.c"

class innerLoopBody {
public:
    int i;  // value of i from outer loop

    void operator () (const tbb::blocked_range<int> &range) const {
        int j;
        for(j=range.begin(); j<range.end(); j++) {
            //printf("Inner Loop Body (x,y)=(%d,%d) where y in [%d,%d)\n", i,j,range.begin(), range.end() );
	    int oldGatek;
            int freshNode,currentEdge;

            currentEdge = vertices[currentLevelSet[i]]+j;
            // let's handle one edge
#ifdef XBOFILE
            freshNode = edges[currentEdge][1]; // 0 RTM, value was prefetched
#else
            freshNode = edges[currentEdge];    // 0 RTM, value was prefetched
#endif
            oldGatek = -1;
            // test gatekeeper 
            oldGatek = gatekeeper[freshNode].fetch_and_increment();
	    if (oldGatek == 0) { // destination vertex unvisited!
                // increment newLevelIndex atomically
                int myIndex = newLevelIndex.fetch_and_increment();
	        // store fresh node in new level
	        newLevelSet[myIndex] = freshNode;

	        level[freshNode] = currentLevel + 1;
	    } // end if freshNode
        } // end for j
    }

    // Constructor
    innerLoopBody (int i) {
        this->i = i;
    }
};


class outerLoopBody {
public:
    void operator () (const tbb::blocked_range<int> &range) const {
        int i; 
        PARTITIONER partitioner;
        for (i=range.begin(); i<range.end(); i++) {
            //printf("Outer Loop Body[%d<=%d<%d]=[0,%d]\n", range.begin(), i, range.end(), degrees[currentLevelSet[i]]);
#ifdef SEQUENTIAL_INNER
            for(int j=0; j<degrees[currentLevelSet[i]]; j++) {
                int oldGatek;
                int freshNode,currentEdge;

                currentEdge = vertices[currentLevelSet[i]]+j;
                // let's handle one edge
#ifdef XBOFILE
                freshNode = edges[currentEdge][1]; // 0 RTM, value was prefetched
#else
                freshNode = edges[currentEdge];    // 0 RTM, value was prefetched
#endif
                oldGatek = -1;
                // test gatekeeper 
                oldGatek = gatekeeper[freshNode].fetch_and_increment();
                if (oldGatek == 0) { // destination vertex unvisited!
                    // increment newLevelIndex atomically
                    int myIndex = newLevelIndex.fetch_and_increment();
                    // store fresh node in new level
                    newLevelSet[myIndex] = freshNode;
 
                    level[freshNode] = currentLevel + 1;
                } // end if freshNode
            }
#else
            tbb::parallel_for (tbb::blocked_range<int>(0,degrees[currentLevelSet[i]],INNER_GRAINSIZE), innerLoopBody(i), partitioner);
#endif
        }
    }
    // Constructor
};

int main(int argc, char* argv[])
{
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
    readBFSInputs();

    tbb::task_scheduler_init init(nth);

    tbb::tick_count t0 = tbb::tick_count::now();
    int ii = 0;
    for (ii=0; ii<TIMES; ii++) {
        int nodesLeft = VSIZE;
        currentLevel = 0; 
        currentLevelSize = 1; 
        currentLevelSet = mytempC;
        newLevelSet = mytempB; 

        currentLevelSet[0] = START; // store the vertex# this thread will handle
        level[START] = 0;           // the level of the START node is 0
        gatekeeper[START] = 1;      // set the gatekeeper of the START node
        PARTITIONER partitioner;
        while (currentLevelSize>0) {
            //printf("New level=%d, levelSize=%d, remaining Nodes=%d\n", currentLevel, currentLevelSize, nodesLeft);
            newLevelIndex = 0;
            //printf("Lvl:%d, lvlSize=%d\n", currentLevel, currentLevelSize);
            tbb::parallel_for (tbb::blocked_range<int>(0, currentLevelSize, OUTER_GRAINSIZE), outerLoopBody(), partitioner);

            // move to next layer
            currentLevel++;
            currentLevelSize = newLevelIndex; // from the prefix-sums
            nodesLeft -= currentLevelSize;
            // "swap" currentLevelSet with newLevelSet
            tmpSet = newLevelSet;
            newLevelSet = currentLevelSet;
            currentLevelSet = tmpSet;
            //if (currentLevel >= 5) break;    

        } // end while
    } // end for ii<TIMES
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

    printf("current level = %d\n", currentLevel);
    // Dump result 
    hexdump_var("result.hex", level, VSIZE);

#ifdef PRINT_RESULT
    printf("Levels:\n");
    for (ii=0;ii<vertices_dim0_size;ii++) {
        printf("Node %d at level %d \n",ii,level[ii]);
    }
#endif
}
