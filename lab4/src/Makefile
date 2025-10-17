CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LDFLAGS = -lpthread

all: psum

psum: parallel_sum.o parallel_sum_lib.o utils.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

parallel_sum.o: parallel_sum.c utils.h parallel_sum_lib.h
	$(CC) $(CFLAGS) -c -o $@ $<

parallel_sum_lib.o: parallel_sum_lib.c parallel_sum_lib.h
	$(CC) $(CFLAGS) -c -o $@ $<

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o psum

test: psum
	./psum --threads_num 4 --array_size 1000 --seed 123

.PHONY: all clean test