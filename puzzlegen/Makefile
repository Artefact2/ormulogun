SOURCES=$(shell find . -name "*.c")
HEADERS=$(shell find . -name "*.h")

gen-puzzles: $(HEADERS) $(SOURCES) ../gumble/build/src/libenginecore.a
	gcc --std=c11 -g -Og -Wall -D_POSIX_C_SOURCE=200809L -o $@ -I../gumble/include $(SOURCES) ../gumble/build/src/libenginecore.a

clean:
	rm gen-puzzles

.PHONY: clean
