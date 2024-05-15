CC=gcc
CDBGFLAGS=-Wall -Wextra -pedantic -std=c2x -g -fprofile-arcs -ftest-coverage

LIBDESTDIR=build
LIBSRCS=$(wildcard *.c)
LIBOBJS=$(patsubst %.c,$(LIBDESTDIR)/%.o,$(LIBSRCS))

TESTDESTDIR=build/test
TESTSRCS=$(wildcard test/*.c)
TESTBINS=$(patsubst test/%.c,$(TESTDESTDIR)/%,$(TESTSRCS))
TESTCOVS=$(patsubst test/%.c,$(LIBDESTDIR)/%.gcov,$(TESTSRCS))

.PHONY: clean init lib test covreport

init:
	@mkdir -p build/test
	@mkdir -p build/report

clean:
	@rm -rf build

lib: init $(LIBOBJS)

$(LIBDESTDIR)/%.o: %.c
	$(CC) $(CDBGFLAGS) -c $< -o $@

test: $(TESTBINS)
	@for test in $(TESTBINS); do \
		./$$test --verbose -f ; \
		if [ $$? -ne 0 ]; then \
			exit 1; \
		fi; \
	done
	@echo -e "\033[0;32mAll tests passed!\033[0m"

$(TESTDESTDIR)/%: test/%.c lib
	$(CC) $(CDBGFLAGS) $< $(LIBOBJS) -o $@ -lcriterion

covreport: test $(TESTCOVS)
	gcovr --html-details build/report/index.html \
		--html-theme github.dark-green \
		--gcov-exclude-directories build/test

$(LIBDESTDIR)/%.gcov:
	gcov -o $(TESTDESTDIR)/$*-$*.gcno $(patsubst %_test,%.c,$*).c -t > $@
