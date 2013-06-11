#include "../xboutils/readMMformat.c"

void readBFSInputs() {

#ifdef XBOFILE
    read_int_data_var (XBOFILE, vertices, "vertices");
    read_int_data_var (XBOFILE, degrees,  "degrees");
    read_int_data_var (XBOFILE, edges,    "edges");
#else
#ifdef FILENAME
    int v_num = VSIZE, e_num = ESIZE;
    IdxT *vs = &(vertices[0]);
    IdxT *ds = &(degrees[0]);
    IdxT *es = &(edges[0]);
    //readMMFormat(FILENAME, NSEPS, &v_num, &vs, &ds, &e_num, &es);
    readBinFormat(FILENAME, &v_num, &vs, &ds, &e_num, &es);
    assert(v_num==VSIZE);
    assert(e_num==ESIZE);
    printf("Done reading input from file: %s\n", FILENAME);
#ifdef DUMP_INPUTS
    hexdump_var("degrees.hex", degrees, VSIZE);
    hexdump_var("vertices.hex", vertices, VSIZE);
    hexdump_var("edges.hex", es, 2*ESIZE);
#endif
#else
    printf("Either FILENAME or XBOUTILS must be defined to read input\n");
    exit(1);
#endif
#endif
             
}
