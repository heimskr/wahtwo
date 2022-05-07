COMPILER        ?= clang++
CFLAGS          := -Iinclude
LDFLAGS         := -framework CoreServices
SOURCES         := $(shell find -L src -name '*.cpp' | sed -nE '/test/!p')
OBJECTS         := $(SOURCES:.cpp=.o)

.PHONY: all test clean

all: wahtwo

wahtwo.a: $(OBJECTS)
	$(AR) rcs $@ $^

wahtwo: $(OBJECTS) src/test.o
	$(COMPILER) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(COMPILER) $(CFLAGS) -c $< -o $@

test: wahtwo
	./$<

clean:
	rm -f $(shell find -L src -name '*.o')
