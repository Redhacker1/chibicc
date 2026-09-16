CFLAGS=-std=c11 -g -fno-common -Wall -Wno-switch
INCLUDES=-Iwin_includes -I. -Isdk/include -Ilibc/libc/include -Iinclude -Icompiler_include
LIBC_FLAGS=$(if $(wildcard sdk/lib/libc.dll.a),sdk/lib/libc.dll.a -Wl,--allow-multiple-definition,$(if $(wildcard libc.dll),libc.dll -Wl,--allow-multiple-definition,))

SRCS=$(filter-out codegen.c, $(wildcard *.c)) $(wildcard codegen/*.c) $(wildcard codegen/*/*.c) $(wildcard abi/*.c) $(wildcard abi/*/*.c)
OBJS=$(SRCS:.c=.o)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

# Stage 1

chibicc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBC_FLAGS) $(LDFLAGS)

$(OBJS): chibicc.h

test/%.exe: chibicc test/%.c
	./chibicc $(INCLUDES) -Itest -c -o test/$*.o test/$*.c
	$(CC) -pthread -o $@ test/$*.o -xc test/common $(LIBC_FLAGS)

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh ./chibicc

test-all: test test-stage2

# Stage 2

stage2/chibicc: $(OBJS:%=stage2/%)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBC_FLAGS) $(LDFLAGS)

stage2/%.o: chibicc %.c
	mkdir -p stage2/test
	./chibicc $(INCLUDES) -c -o $(@D)/$*.o $*.c

stage2/test/%.exe: stage2/chibicc test/%.c
	mkdir -p stage2/test
	./stage2/chibicc $(INCLUDES) -Itest -c -o stage2/test/$*.o test/$*.c
	$(CC) -pthread -o $@ stage2/test/$*.o -xc test/common $(LIBC_FLAGS)

test-stage2: $(TESTS:test/%=stage2/test/%)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh ./stage2/chibicc

# Misc.

clean:
	rm -rf chibicc tmp* $(TESTS) test/*.s test/*.exe stage2
	find * -type f '(' -name '*~' -o -name '*.o' ')' -exec rm {} ';'

.PHONY: test clean test-stage2
