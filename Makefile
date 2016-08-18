# Turn off builtin implicit rules
.SUFFIXES:

CC=clang
DB=NDEBUG

CC_FLAGS = -Wall -std=c11 -O2 -g -pthread -D${DB}
all: test

msg_pool.o : msg_pool.c msg_pool.h msg_pool.h dpf.h Makefile
	${CC} ${CC_FLAGS} -c $< -o $@

mpscringbuff.o : mpscringbuff.c mpscringbuff.h msg_pool.h dpf.h Makefile
	${CC} ${CC_FLAGS} -c $< -o $@

test.o : test.c mpscringbuff.h dpf.h msg_pool.h Makefile
	${CC} ${CC_FLAGS} -c $< -o $@

test : test.o mpscringbuff.o msg_pool.o
	${CC} ${CC_FLAGS} $^ -o $@
	objdump -d $@ > $@.txt

run : test
	@./test ${client_count} ${loops} ${msg_count}

clean :
	@rm -f test *.o test.txt
