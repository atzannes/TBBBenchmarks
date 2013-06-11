/* BFS Serial implementation 
 * Author: George Caragea
 * Description: Serially going thru the edges in one level, by vertex
 * Version: $Id: bfs_serial.c 1430 2006-08-04 20:02:15Z george $
 */
#include "tbb/tick_count.h"
#include "tbb/atomic.h"
#include "../xboutils/xboutils.h"
#include <stdio.h>

#ifndef TIMES
#define TIMES 1
#endif

tbb::atomic<int> gatekeeper[VSIZE];
tbb::atomic<int> newLevelIndex;


int queue[VSIZE];
int previous_vertex[VSIZE];

int current_level;
int q_head,q_tail, old_tail;
int i,j,v,next_vertex;

int level[VSIZE] = {0};

/* vars to be initialized from .xbo file or input */
int vertices[VSIZE];
int degrees[VSIZE];
#ifdef XBOFILE
int edges[ESIZE][2];
#else
int edges[ESIZE];
#endif

#include "read_input.c"

int main(void)
{
    readBFSInputs();

    tbb::tick_count t0 = tbb::tick_count::now();
    int ii;
    for (ii=0; ii<TIMES; ii++) {
        current_level = 0;
        // initialize the queue
        q_tail=0;
        q_head=0;
        // START is the initial NODE
        // enqueue that node, set its level and mark it
        queue[q_tail] = START;
        q_tail = q_tail + 1;
        // the previous 2 lines were queue[q_tail++] = START; and this 
        // would increment q_tail by 2 !!
        level[START] = current_level;
        previous_vertex[START]=-1;
        gatekeeper[START] = 1;

        // while all nodes have not been visited
        while(q_head!=q_tail && q_head<VSIZE)
        {
            current_level++;
            // mark the end of the current queue (old_tail)
            // this is needed to keep track of the levels
            old_tail = q_tail;

            for(q_head;q_head<old_tail;q_head++)
            {   // for all vertices currently in the queue
                v=queue[q_head];
                for(j=vertices[v];j<vertices[v]+degrees[v];j++)
                {   // for every outgoing edge from 'v'
#ifdef XBOFILE
                    next_vertex=edges[j][1];
  #ifdef ANTIPARALLEL
                    if (next_vertex != -1) {
  #endif
#else // in this format we only store the target node of edges in the edge list
                    next_vertex=edges[j];
#endif
                    if(gatekeeper[next_vertex] == 0)
                    {   // if the vertex u visited by the edge has not been visited yet..
                        // enqueue u, set its level and mark it as visited
                        queue[q_tail] = next_vertex;
                        q_tail = q_tail + 1;
                        level[next_vertex] = current_level;
                        previous_vertex[next_vertex] = v;
                        gatekeeper[next_vertex] = 1;
#ifdef XBOFILE
#ifdef ANTIPARALLEL
                        // now mark antiparallel edge as deleted
                        int antiParEdge = antiParallel[j]; // 0 RTM, value was prefetched
                        edges[antiParEdge][1] = -1;
                        //edges[antiParEdge][0] = -1;
                    }
#endif
#endif
                    }
                }
            }    
        } //end while 
    } // end for ii<TIMES
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

    // Dump result 
    hexdump_var("result.hex", level, VSIZE);

    return 0;
}

