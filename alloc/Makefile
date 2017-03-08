CC = gcc
CFLAGS = -O0 -g -Wall
GENCL = weave

OBJS = cl_params_aux.o cl_params.o alloc.o
all: alloc

alloc: $(OBJS)
	$(CC) $(CFLAGS) -o alloc $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

cl_params.o: cl_params.c
	$(CC) $(CFLAGS) -c $<

cl_params_aux.o: cl_params_aux.c
	$(CC) $(CFLAGS) -c $<

cl_params_aux.c: appl-cl.txt
	$(GENCL) -l C -d appl-cl.txt

clean:
	rm -f *.o core alloc

dist_clean:
	rm -f *.o core cl_params_aux.[ch] cl_params.[ch] alloc
