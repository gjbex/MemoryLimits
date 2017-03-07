CC = gcc
CFLAGS = -O0 -g -Wall
CXX = mpic++
CXXFLAGS = -O0 -g -Wall -std=c++14
GENCL = weave

OBJS = cl_params_aux.o cl_params.o alloc.o
all: alloc mem_limit

mem_limit: mem_limit.o
	$(CXX) $(CXXFLAGS) -o $@ $^

mem_limit.o: mem_limit.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

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
	rm -f *.o core alloc mem_limit

dist_clean:
	rm -f *.o core cl_params_aux.[ch] cl_params.[ch] alloc mem_limit
