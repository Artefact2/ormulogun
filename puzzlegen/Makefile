SOURCES:=$(shell find . -name "*.c")
HEADERS:=$(shell find . -name "*.h")
TMP:=$(shell mktemp -u -p .)
JOBS:=16

gen-puzzles: $(HEADERS) $(SOURCES) ../gumble/build/src/libenginecore.a
	gcc --std=c11 -g -Og -Wall -D_POSIX_C_SOURCE=200809L -o $@ -I../gumble/include $(SOURCES) ../gumble/build/src/libenginecore.a

clean:
	rm gen-puzzles

pgnlist.jz:
	pv $(INPUT) | ./chunk-pgn | xargs -d '\n' -P $(JOBS) ./preprocess-pgn | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $@

master.jz:
	echo -n | zstd -4 - -o $@

merge-to-master: master.jz
	rm -f tmp.master.fifo tmp.input.fifo
	mkfifo tmp.master.fifo
	mkfifo tmp.input.fifo
	zstdcat master.jz > tmp.master.fifo &
	zstdcat $(INPUT) > tmp.input.fifo &
	./merge-jl tmp.master.fifo tmp.input.fifo | pv -l | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) master.jz
	rm -f tmp.master.fifo tmp.input.fifo

puzzles-all:
	pv pgnlist.jz | zstdcat | xargs -d '\n' -P $(JOBS) ./gen-puzzles | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) puzzles-all.jz
	make merge-to-master INPUT=puzzles-all.jz

puzzles-partial:
	zstdcat pgnlist.jz | tail -n +$(OFFSET) | head -n $(COUNT) | pv -ls $(COUNT) | xargs -d '\n' -P $(JOBS) -n $$(( $(COUNT) / $(JOBS) )) ./gen-puzzles | zstd -4 -o - $(TMP)
	sync $(TMP)
	mv $(TMP) puzzles-$(OFFSET)-$(COUNT).jz
	make merge-to-master INPUT=puzzles-$(OFFSET)-$(COUNT).jz

.PHONY: clean merge-to-master puzzles-all puzzles-partial
