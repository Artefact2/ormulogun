SOURCES:=$(shell find src -name "*.c")
HEADERS:=$(shell find src -name "*.h")
TMP:=$(shell mktemp -u -p .)
JOBS:=16

tools/gen-puzzles: $(HEADERS) $(SOURCES) ../gumble/build/src/libenginecore.a
	gcc --std=c11 -g -Og -Wall -D_POSIX_C_SOURCE=200809L -o $@ -I../gumble/include $(SOURCES) ../gumble/build/src/libenginecore.a

clean:
	rm -f tools/gen-puzzles

pgnlist.jz:
	pv $(INPUT) | ./tools/chunk-pgn | xargs -d '\n' -P $(JOBS) ./tools/preprocess-pgn | zstd -4 - -o $(TMP)
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
	./tools/merge-jl tmp.master.fifo tmp.input.fifo | pv -l | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) master.jz
	rm -f tmp.master.fifo tmp.input.fifo

puzzles-all: tools/gen-puzzles
	pv pgnlist.jz | zstdcat | xargs -d '\n' -P $(JOBS) ./tools/gen-puzzles | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) puzzles-all.jz
	make merge-to-master INPUT=puzzles-all.jz

puzzles-partial: tools/gen-puzzles
	zstdcat pgnlist.jz | tail -n +$(OFFSET) | head -n $(COUNT) | pv -ls $(COUNT) | xargs -d '\n' -P $(JOBS) -n $$(( $(COUNT) / $(JOBS) )) ./tools/gen-puzzles | zstd -4 -o - $(TMP)
	sync $(TMP)
	mv $(TMP) puzzles-$(OFFSET)-$(COUNT).jz
	make merge-to-master INPUT=puzzles-$(OFFSET)-$(COUNT).jz

master-json: master.jz
	zstdcat master.jz | ./tools/jl_to_json > ./out/master.json
	./tools/update-manifest ./out/manifest.json
	cd .. && make

.PHONY: clean merge-to-master puzzles-all puzzles-partial master-json
