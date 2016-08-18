# Turn off builtin implicit rules
.SUFFIXES:

CC=clang
DB=NDEBUG

CC_FLAGS = -Wall -std=c11 -O2 -g -pthread -D${DB}
all: test simple

diff_timespec.o : diff_timespec.c diff_timespec.h Makefile
	${CC} ${CC_FLAGS} -c $< -o $@

msg_pool.o : msg_pool.c msg_pool.h msg_pool.h dpf.h Makefile
	${CC} ${CC_FLAGS} -c $< -o $@

mpscringbuff.o : mpscringbuff.c mpscringbuff.h msg_pool.h dpf.h Makefile
	${CC} ${CC_FLAGS} -c $< -o $@

test.o : test.c mpscringbuff.h dpf.h msg_pool.h Makefile
	${CC} ${CC_FLAGS} -c $< -o $@

test : test.o mpscringbuff.o msg_pool.o diff_timespec.o
	${CC} ${CC_FLAGS} $^ -o $@
	objdump -d $@ > $@.txt

run : test
	@./test ${client_count} ${loops} ${msg_count}

simple.o : simple.c mpscringbuff.h dpf.h msg_pool.h Makefile
	${CC} ${CC_FLAGS} -c $< -o $@

simple : simple.o mpscringbuff.o msg_pool.o diff_timespec.o
	${CC} ${CC_FLAGS} $^ -o $@
	objdump -d $@ > $@.txt

runs : simple
	@./simple ${loops}

clean :
	@rm -f *.o
	@rm -f test test.txt
	@rm -f simple simple.txt
