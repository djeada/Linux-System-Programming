CC ?= cc
CFLAGS ?= -std=c11 -O0 -g -Wall -Wextra -Wpedantic
ASANFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer

EPISODES := $(notdir $(patsubst %/,%,$(wildcard episodes/*/)))
PLAIN := $(foreach e,$(EPISODES),build/$(e)/vulnerable build/$(e)/fixed)
SANITIZED := $(foreach e,$(EPISODES),build-asan/$(e)/vulnerable build-asan/$(e)/fixed)

.PHONY: all asan test clean

all: $(PLAIN)
asan: $(SANITIZED)

build/%/vulnerable: episodes/%/vulnerable.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

build/%/fixed: episodes/%/fixed.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

build-asan/%/vulnerable: episodes/%/vulnerable.c
	@mkdir -p $(@D)
	$(CC) $(ASANFLAGS) $< -o $@

build-asan/%/fixed: episodes/%/fixed.c
	@mkdir -p $(@D)
	$(CC) $(ASANFLAGS) $< -o $@

test: all
	python3 -m unittest discover -s tests -v

clean:
	rm -rf build build-asan
