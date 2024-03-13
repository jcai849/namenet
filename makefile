CFLAGS = -Wall -Wextra -g -O2 -ansi -Wpedantic -D_POSIX_C_SOURCE=200112L
LDLIBS = `pkg-config libgvc --libs` -l:librec.so.1
PROG = namenet
.PHONY: test clean

${PROG}: ${PROG}.c
	$(CC) $(CFLAGS) ${PROG}.c $(LDLIBS) -o ${PROG}

test: ${PROG}
	@cd test && ./test
clean:
	rm namenet
	rm tags
