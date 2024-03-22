CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c2x -g

LIBDESTDIR=build
LIBSRCS=$(wildcard *.c)
LIBOBJS=$(patsubst %.c,$(LIBDESTDIR)/%.o,$(LIBSRCS))

TESTDESTDIR=build/test
TESTSRCS=$(wildcard test/*.c)
TESTBINS=$(patsubst test/%.c,$(TESTDESTDIR)/%,$(TESTSRCS))

init:
	@mkdir -p build/test

lib: init $(LIBOBJS)

$(LIBDESTDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TESTBINS)
	@for test in $(TESTBINS); do \
		./$$test --verbose -j0 -f ; \
		if [ $$? -ne 0 ]; then \
			exit 1; \
		fi; \
	done
	@echo "All tests passed. Generating coverage report..."
$(TESTDESTDIR)/%: test/%.c lib
	$(CC) $(CFLAGS) -fprofile-arcs -ftest-coverage $< $(LIBOBJS) -o $@ -lcriterion
