SOURCES:=$(shell find src -name "*.c")
HEADERS:=$(shell find src -name "*.h")
TMP:=$(shell mktemp -u -p .)
JOBS:=2

tools/gen-puzzles: $(HEADERS) $(SOURCES) ../gumble/build/src/libenginecore.a
	gcc --std=c11 -g -Og -Wall -D_POSIX_C_SOURCE=200809L -o $@ -I../gumble/include $(SOURCES) ../gumble/build/src/libenginecore.a

clean:
	rm -f tools/gen-puzzles

pgnlist.jz:
	pv -cN input $(INPUT) | ./tools/chunk-pgn | xargs -d '\n' -P $(JOBS) ./tools/preprocess-pgn | pv -lcN games | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $@

master.jz:
	echo -n | zstd -4 - -o $@

puzzles-all: tools/gen-puzzles
	pv -cN input pgnlist.jz | zstdcat | xargs -d '\n' -P $(JOBS) ./tools/gen-puzzles | pv -lcN puzzles | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) puzzles-all.jz

puzzles-partial: tools/gen-puzzles
	zstdcat pgnlist.jz | tail -n +$(OFFSET) | head -n $(COUNT) | pv -lcN games -s $(COUNT) | xargs -d '\n' -P $(JOBS) -n $$(( $(COUNT) / $(JOBS) )) ./tools/gen-puzzles | pv -lcN puzzles | zstd -4 -o - $(TMP)
	sync $(TMP)
	mv $(TMP) puzzles-$(OFFSET)-$(COUNT).jz

merge-all: master.jz $(shell find . -name "*.jz" -not -name "pgnlist.jz" -not -name "master.jz")
	zstdcat $^ | pv -lcN in | ./tools/merge-jl | pv -lcN out | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) master.jz

out/manifest.json: $(shell find out/ -name "*.json" -not -name "manifest.json")
	./tools/update-manifest $@

out/%.json: %.jz
	zstdcat $< | ./tools/jl_to_json > $@

out/%.js: out/%.json
	cp $< $@

json-all: $(patsubst %.jz, out/%.json, $(shell find . -name "*.jz" -not -name "pgnlist.jz"))
	make out/manifest.json

js-all: $(patsubst %.json, %.js, $(shell find out/ -name "*.json"))

.PHONY: clean puzzles-all puzzles-partial merge-all json-all js-all
