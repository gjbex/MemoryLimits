#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cl_params.h"

#define EXIT_NO_ARG 1
#define EXIT_NO_MEM 2

void fill(char *c, long size);

int main(int argc, char *argv[]) {
    long mem;
    Params params;
    initCL(&params);
    parseCL(&params, &argc, &argv);
    dumpCL(stdout, "# ", &params);
    if (params.maxMem < 0)
        errx(EXIT_NO_ARG, "no -maxMem specified");
    if (params.incr < 0)
        params.incr = params.maxMem;
    for (mem = params.incr; mem <= params.maxMem; mem += params.incr) {
        char *c;
        if ((c = (char *) calloc(mem, sizeof(char))) == NULL)
            errx(EXIT_NO_MEM, "can't allocate %ld bytes", mem);
        printf("%ld bytes allocated succesfully\n", mem);
        fill(c, mem);
        printf("%ld bytes written succesfully\n", mem);
        sleep(params.sleep);
        free(c);
    }
    finalizeCL(&params);
    return EXIT_SUCCESS;
}

void fill(char *c, long size) {
    long i;
    for (i = 0; i < size; i++)
        c[i] = 'A' + rand() % 26;
}
