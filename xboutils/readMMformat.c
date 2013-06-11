#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 1025
#endif


//typedef unsigned int IdxT;
typedef int IdxT;
const char *separator = "%-------------------------------------------------------------------------";

#define assert(e) { if (!(e)) { printf("Assertion failure: %s\n", #e); exit(1);}}
#define MIN(x,y) (x<y?x:y)

void writeBinFormat(const char *filename, IdxT v_num, IdxT verts[],
                    IdxT degs[], IdxT e_num, IdxT edgs[]) {
  FILE *f;
  if ((f = fopen(filename, "r")) != NULL) {
    printf ("Output file already exists <%s>. Remove or change to proceed.\n", filename);
    exit(1);
  }
  if ((f = fopen(filename, "w")) == NULL) {
    printf ("Cannot open output for writing <%s>.\n", filename);
    exit(1);
  }
  // 1. Write |V|
  size_t writeCount = fwrite(&v_num, sizeof(IdxT), 1, f);
  assert(writeCount==1);

  // 2. Write |E|
  writeCount = fwrite(&e_num, sizeof(IdxT), 1, f);
  assert(writeCount==1);

  // 3. Write vertices
  writeCount = fwrite(verts, sizeof(IdxT), v_num, f);
  assert(writeCount==v_num);

  // 4. Write degrees
  writeCount = fwrite(degs, sizeof(IdxT), v_num, f);
  assert(writeCount==v_num);

  // 5. Write edges
  writeCount = fwrite(edgs, sizeof(IdxT), e_num, f);
  assert(writeCount==e_num);

  fclose(f);
}

#ifdef __BIG_ENDIAN_XBO
inline int switchEndianness(int x) {
  return ((x>>24)& 0x000000FF) |
         ((x<<8) & 0x00FF0000) |
         ((x>>8) & 0x0000FF00) |
         (x<<24);
}

inline void switchEndiannessVector(int *v, int size) {
  assert(size>=0);
  for(int i=0; i<size; i++) {
    v[i] = switchEndianness(v[i]);
  }
}
#endif

void readBinFormat(const char *filename, IdxT *v_num, IdxT **verts,
                  IdxT **degs, IdxT *e_num, IdxT **edgs) {

  FILE *f;
  if ((f = fopen(filename, "r")) == NULL) {
    printf ("can't open file <%s> \n", filename);
    exit(1);
  }
  // 1. Read |V|
  size_t readCount = fread(v_num, sizeof(IdxT), 1, f);
  assert(readCount==1);
#ifdef __BIG_ENDIAN_XBO
  switchEndiannessVector(v_num, 1);
#endif
  printf("DEBUG: readCount = %d,  v_num=%d\n", readCount, *v_num);

  // 2. Read |E|
  readCount = fread(e_num, sizeof(IdxT), 1, f);
  assert(readCount==1);
#ifdef __BIG_ENDIAN_XBO
  switchEndiannessVector(e_num, 1);
#endif
  printf("DEBUG: readCounT = %d, e_num=%d\n", readCount, *e_num);

  // 3. Read vertices
  /*for(int c = 0; c < *v_num; c += 512) {
    int amt = MIN(512, *v_num-c);
    IdxT *vptr = *verts+c;
    readCount = fread(*verts, sizeof(IdxT), amt, f);
    printf("DEBUG: readCount = %d\n", readCount);
    assert(readCount==amt);
  }*/
  
  readCount = fread(*verts, sizeof(IdxT), *v_num, f);
  printf("DEBUG: readCount = %d\n", readCount);
  assert(readCount==*v_num);
#ifdef __BIG_ENDIAN_XBO
  switchEndiannessVector(*verts, *v_num);
#endif

  // 4. Read degrees
  readCount = fread(*degs, sizeof(IdxT), *v_num, f);
  printf("DEBUG: readCount = %d\n", readCount);
  assert(readCount==*v_num);
#ifdef __BIG_ENDIAN_XBO
  switchEndiannessVector(*degs, *v_num);
#endif

  // 5. Read edges
  readCount = fread(*edgs, sizeof(IdxT), *e_num, f);
  printf("DEBUG: readCount = %d\n", readCount);
  assert(readCount==*e_num);
#ifdef __BIG_ENDIAN_XBO
  switchEndiannessVector(*edgs, *e_num);
#endif

  fclose(f);
}

void discardBanner(FILE *f, int NSeps) {
  // Read & Discard lines starting with %
  int count = 0;
  char line[MAX_LINE_LENGTH];
  while(count < NSeps) {
    if (fgets(line, MAX_LINE_LENGTH, f) == NULL) {
      printf("Failed while scanning MM banner.\n");
      exit(1);
    }
    if(strncmp(line, separator, strlen(separator))==0)
      count++;
  }
}

IdxT readMMEdges(FILE *f, IdxT base, IdxT v_num, 
                 IdxT e_num, IdxT *degrees, IdxT *edges) {
  IdxT k = 0;
  for (int i = 0; i < e_num; i++) {
    // Read line (v1, v2) -- sorted by v2, v1
    IdxT v1, v2;
    int readCount = fscanf(f, "%d %d", &v1, &v2);
    assert(readCount==2);
    //printf("DEBUG:: read (v1,v2)=(%d,%d)\n", v1, v2);
    v1 -= base;
    v2 -= base;
    assert(v1>=0 && v1<v_num);
    assert(v2>=0 && v2<v_num);
    edges[2*i] = v1;
    edges[2*i+1] = v2;
    degrees[v1]++;
    if (v1 != v2) {
      edges[2*e_num+2*k] = v2;
      edges[2*e_num+2*k+1] = v1;
      degrees[v2]++;
      k++;
    }
  }
  return k;
}

void computeVertices(IdxT v_num, IdxT *degrees, IdxT *vertices) {
  vertices[0] = 0;
  for (int i = 1; i < v_num; i++) {
     vertices[i] = vertices[i-1] + degrees[i-1];
  }
}

void buildEdgeList(IdxT v_num, IdxT e_num, IdxT *vertices, 
                   IdxT *degrees, IdxT *edges, IdxT *sorted_edges) {
  // 1. make a copy of vertices
  IdxT *curr_idx = (IdxT*) malloc (v_num*sizeof(IdxT));
  memcpy(curr_idx, vertices, v_num*sizeof(IdxT));  
  // 2. compute edge list
  for (int i = 0; i < e_num; i++) {
    IdxT v1 = edges[2*i], v2 = edges[2*i+1];
    sorted_edges[curr_idx[v1]] = v2;
    curr_idx[v1]++;
    //printf("DEBUG::curr_idx[v1]<= vertices[v1]+degrees[v1] ? (%d <= %d + %d), for (v1,v2)=(%d,%d)\n", curr_idx[v1], vertices[v1], degrees[v1], v1, v2);
    assert(curr_idx[v1]<= vertices[v1]+degrees[v1]);
  }
  free(curr_idx);
}

void buildEdgeList2(IdxT v_num, IdxT e_num, IdxT *vertices, 
                   IdxT *degrees, IdxT *edges, IdxT *sorted_edges) {
  // 1. make a copy of vertices
  IdxT *curr_idx = (IdxT*) malloc (v_num*sizeof(IdxT));
  memcpy(curr_idx, vertices, v_num*sizeof(IdxT));  

  for (int i = 0; i < e_num; i++) {
    IdxT v1 = edges[2*i], v2 = edges[2*i+1];
    sorted_edges[2*curr_idx[v1]] = v1;
    sorted_edges[2*curr_idx[v1]+1] = v2;
    curr_idx[v1]++;
    //printf("DEBUG::curr_idx[v1]<= vertices[v1]+degrees[v1] ? (%d <= %d + %d), for (v1,v2)=(%d,%d)\n", curr_idx[v1], vertices[v1], degrees[v1], v1, v2);
    assert(curr_idx[v1]<= vertices[v1]+degrees[v1]);
  }
  free(curr_idx);
}

void convertMMtoBinFormat(const char *filenameIn, int NSeps, const char *filenameOut) {
  FILE *fout;
  if ((fout = fopen(filenameOut, "r")) != NULL) {
    printf ("Output file already exists <%s>. Remove or change to proceed.\n", filenameOut);
    fclose(fout);
    exit(1);
  }
  FILE *f;
  if ((f = fopen(filenameIn, "r")) == NULL) {
    printf ("can't open file <%s> \n", filenameIn);
    exit(1);
  }
  IdxT base = 1;
  // Read & Discard lines starting with %
  discardBanner(f, NSeps);
  // Read 1st line |V| |V| |E|
  IdxT vertices_size = 0;
  IdxT v_size2 = 0;
  IdxT edges_size = 0;
  int readCount = fscanf(f, "%d %d %d", &vertices_size, &v_size2, &edges_size); 
  assert(readCount == 3);
  assert(vertices_size == v_size2);
  printf("Number of undirected nodes = %d\n", vertices_size);
  printf("Number of undirected edges = %d\n", edges_size);

  // Read |E| undirected edges, create 2|E| directed edges
  IdxT *vertices = (IdxT*) malloc (vertices_size*sizeof(IdxT));
  IdxT *degrees = (IdxT*) malloc (vertices_size*sizeof(IdxT));
  memset(degrees, 0, vertices_size*sizeof(IdxT));  
  IdxT *edges = (IdxT*) malloc (4*edges_size*sizeof(IdxT));
  IdxT k = readMMEdges(f, base, vertices_size, edges_size, degrees, edges);
  edges_size += k;
  fclose(f);
  printf("Number of   directed edges = %d\n", edges_size);
  
  printf("Done reading edges from input file\n");
  // prefix sum over degrees
  computeVertices(vertices_size, degrees, vertices);
  printf("Computed vertices and curr_idx from degrees\n");

  IdxT *sorted_edges = (IdxT*) malloc(edges_size*sizeof(IdxT));
  assert(sorted_edges!=0);
  //IdxT *sorted_edges = *edgs;
  buildEdgeList(vertices_size, edges_size, vertices, degrees, edges, sorted_edges);
  printf("Done building degrees and sorted_edges.\n");
  free(edges);
  // Now write to output file.
  printf("Gonna write to file now!\n");

  writeBinFormat(filenameOut, vertices_size, vertices, 
                 degrees, edges_size, sorted_edges);
  free(vertices);
  free(degrees);
  free(sorted_edges);
}

void readMMFormat(const char *filename, int NSeps, IdxT *v_num, 
                  IdxT **verts, IdxT **degs, IdxT *e_num, IdxT **edgs) {
  FILE *f;
  if ((f = fopen(filename, "r")) == NULL) {
    printf ("can't open file <%s> \n", filename);
    exit(1);
  }
  IdxT base = 1;
  // Read & Discard lines starting with %
  discardBanner(f, NSeps);
  // Read 1st line |V| |V| |E|
  IdxT vertices_size = 0;
  IdxT v_size2 = 0;
  IdxT edges_size = 0;
  int readCount = fscanf(f, "%d %d %d", &vertices_size, &v_size2, &edges_size); 
  assert(readCount==3);
  assert(vertices_size == v_size2);
  assert(vertices_size == *v_num);
  assert(2*edges_size == *e_num);
  // Read |E| undirected edges, create 2|E| directed edges
  IdxT *vertices = *verts;
  IdxT *degrees = *degs;
  memset(degrees, 0, vertices_size*sizeof(IdxT));  
  IdxT *edges = (IdxT*) malloc (2*(*e_num)*sizeof(IdxT));
  IdxT k = readMMEdges(f, base, vertices_size, edges_size, degrees, edges);

  //*e_num += k;
  assert(*e_num == edges_size+k);
  edges_size += k;
  
  // prefix sum over degrees
  computeVertices(vertices_size, degrees, vertices);
  // Compute Edge List
  IdxT *sorted_edges = *edgs;
  buildEdgeList(vertices_size, edges_size, vertices, degrees, edges, sorted_edges);

  free(edges);
  fclose(f);
}
