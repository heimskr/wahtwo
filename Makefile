COMPILER        ?= clang++
CFLAGS          := -Iinclude
SOURCES         := $(shell find -L src -name '*.cpp' | sed -nE '/test|fsevents|inotify/!p')
OBJECTS         := $(SOURCES:.cpp=.o)

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

wahtwo.a: $(OBJECTS) src/$(IMPLEMENTATION).o
	$(AR) rcs $@ $^

wahtwo: $(OBJECTS) src/$(IMPLEMENTATION).o src/test.o
	$(COMPILER) $^ -o $@ $(LDFLAGS)

%.o: %.cpp include/wahtwo/Watcher.h
	$(COMPILER) $(CFLAGS) -c $< -o $@

test: wahtwo
	./$<

clean:
	rm -f $(shell find -L src -name '*.o')
