COMPILER        ?= clang++
CFLAGS          := -std=c++20 -g -Wall -Wextra -Iinclude

ifeq ($(shell uname -s), Darwin)
IMPLEMENTATION  := fsevents
LDFLAGS         := -framework CoreServices
else
ifeq ($(shell uname -s), Linux)
IMPLEMENTATION  := inotify
else
$(error Unsupported platform)
endif
endif

.PHONY: all test clean

all: wahtwo

libwahtwo.a: src/$(IMPLEMENTATION).o
	$(AR) rcs $@ $^

wahtwo: src/$(IMPLEMENTATION).o src/test.o
	$(COMPILER) $^ -o $@ $(LDFLAGS)

%.o: %.cpp include/wahtwo/Watcher.h
	$(COMPILER) $(CFLAGS) -c $< -o $@

test: wahtwo
	./$<

clean:
	rm -f $(shell find -L src -name '*.o')
