CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic
LDLIBS ?= -pthread

LABS := $(addprefix lab,$(shell seq 1 19))
HEADERS := $(wildcard labs/common/*.h)
LIBURING_CFLAGS ?= $(shell pkg-config --cflags liburing 2>/dev/null)
LIBURING_LIBS ?= $(shell pkg-config --libs liburing 2>/dev/null)

.PHONY: all comparisons optional antipatterns test clean
all: $(addprefix build/,$(LABS)) build/lab18_plugin.so

build:
	mkdir -p $@

build/lab%: labs/lab%/main.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)

comparisons: build/fork_exec_shell build/atomic_publication

build/atomic_publication: labs/lab17/comparisons/publication.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)

build/lab18: labs/lab18/main.c labs/lab18/plugin_api.h $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS) -ldl

build/lab18_plugin.so: labs/lab18/plugin.c labs/lab18/plugin_api.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -shared -fPIC $< -o $@

optional:
	@if test -n "$(LIBURING_LIBS)"; then $(MAKE) build/lab20; else \
	  echo 'SKIP: lab20 requires liburing development files (pkg-config liburing)'; fi

build/lab20: labs/lab20/main.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIBURING_CFLAGS) $< -o $@ $(LDLIBS) $(LIBURING_LIBS)

antipatterns: build/leak_antipattern

build/leak_antipattern: labs/lab16/antipatterns/leak.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

build/fork_exec_shell: labs/lab3/comparisons/fork_exec.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)

test:
	python3 -m unittest discover -s tests -v

clean:
	rm -rf build
