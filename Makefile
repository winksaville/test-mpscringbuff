# Turn off builtin implicit rules
.SUFFIXES:

CC=clang
DB=NDEBUG

CC_FLAGS = -Wall -std=c11 -O2 -g -pthread -D${DB}
all: test

mpscringbuff.o : mpscringbuff.c
	${CC} ${CC_FLAGS} -c $< -o $@

test.o : test.c
	${CC} ${CC_FLAGS} -c $< -o $@

test : test.o mpscringbuff.o
	${CC} ${CC_FLAGS} $^ -o $@
	objdump -d $@ > $@.txt

run : test
	@./test ${client_count} ${loops} ${msg_count}

clean :
	@rm -f test *.o test.txt
