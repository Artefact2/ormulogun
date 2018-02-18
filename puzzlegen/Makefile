SHELL=/bin/bash
SOURCES:=$(shell find src -name "*.c")
HEADERS:=$(shell find src -name "*.h")
TMP:=$(shell mktemp -u -p .)
JOBS:=2

tools/gen-puzzles: $(HEADERS) $(SOURCES) ../gumble/build/src/libenginecore.a
	gcc --std=c11 -g -Og -Wall -D_POSIX_C_SOURCE=200809L -o $@ -I../gumble/include $(SOURCES) ../gumble/build/src/libenginecore.a

clean:
	rm -f tools/gen-puzzles

import-pgn:
	pv -cN input $(INPUT) | ./tools/chunk-pgn | xargs -d '\n' -P $(JOBS) ./tools/filter-pgn | xargs -d '\n' -P $(JOBS) ./tools/pgnml_to_json | pv -lcN games | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)

master.jpz:
	echo -n | zstd -4 - -o $@

puzzles-all: tools/gen-puzzles
	pv -cN input $(INPUT) | zstdcat | xargs -d '\n' -P $(JOBS) ./tools/gen-puzzles --verbose $(PFLAGS) 2> >(tee >(grep --line-buffered move | pv -lcN moves >/dev/null) | grep --line-buffered game | pv -lcN games >/dev/null) | pv -lcN puzzles | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)

puzzles-partial: tools/gen-puzzles
	zstdcat $(INPUT) | tail -n +$(OFFSET) | head -n $(COUNT) | xargs -d '\n' -P $(JOBS) ./tools/gen-puzzles --verbose $(PFLAGS) 2> >(tee >(grep --line-buffered move | pv -lcN moves >/dev/null) | grep --line-buffered game | pv -s $(COUNT) -lcN games >/dev/null) | pv -lcN puzzles | zstd -4 -o - $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)

merge-all: master.jpz $(shell find . -name "*.jpz" -not -name "master.jpz")
	zstdcat $^ | pv -lcN in | ./tools/merge-jl | pv -lcN out | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) master.jpz

out/manifest.json: $(shell find out/ -name "*.json" -not -name "manifest.json")
	./tools/update-manifest $@

out/%.json: %.jpz
	zstdcat $< | ./tools/jl_to_json > $@

out/%.js: out/%.json
	cp $< $@

json-all: $(patsubst %.jpz, out/%.json, $(shell find . -name "*.jpz"))
	make out/manifest.json

js-all: $(patsubst %.json, %.js, $(shell find out/ -name "*.json"))

.PHONY: clean import-pgn puzzles-all puzzles-partial merge-all json-all js-all
