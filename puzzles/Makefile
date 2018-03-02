SHELL=/bin/bash
TMP:=$(shell mktemp -u -p .)
JOBS:=16
PATH:=../bin:$(PATH)

import-pgn:
	pv -cN input $(INPUT) | chunk-pgn | xargs -d '\n' -P $(JOBS) filter-pgn | xargs -d '\n' -P $(JOBS) pgnml_to_json | pv -lcN games | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)

master.jpz:
	echo -n | zstd -4 - -o $@

puzzles-all:
	make puzzles START=0 END=$$(zstdcat $(INPUT) | wc -l) OUTPUT=$(TMP).p
	make merge-all
	rm $(TMP).p*

puzzles:
	seq $(START) $(END) | xargs -n 1 -P $(JOBS) -I '{}' bash -c "zstdcat $(INPUT) | tail -n +{} | head -n 1 | xargs -d '\n' gen-puzzles --verbose $(PFLAGS) | tee >(zstd -4 -o $(TMP).{}); sync $(TMP).{}; mv $(TMP).{} $(OUTPUT)-{}.jpz" 2> >(tee >(grep --line-buffered move | pv -lcN moves >/dev/null) | grep --line-buffered game | pv -s $$(( $(END) - $(START) + 1 )) -lcN games >/dev/null) | pv -lcN puzzles >/dev/null

merge-all: master.jpz $(shell find . -name "*.jpz" -not -name "master.jpz")
	zstdcat $^ | pv -lcN in | merge-jl | pv -lcN out | zstd -4 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) master.jpz

out/manifest.json: $(shell find out/ -name "*.json" -not -name "manifest.json")
	update-manifest $@

out/%.json: %.jpz
	zstdcat $< | jl_to_json > $@

out/%.js: out/%.json
	cp $< $@

retag-all: $(shell find . -name "*.jpz")
	(for x in $^; do zstdcat $$x | xargs -P $(JOBS) -d '\n' retag-puzzles | tee >(zstd -4 - -o tmp.retag.$$x); done) | pv -ls `zstdcat $^ | wc -l` >/dev/null
	sync tmp.retag.*
	rename tmp.retag. "" tmp.retag.*

json-all: $(patsubst %.jpz, out/%.json, $(shell find . -name "*.jpz"))
	make out/manifest.json

js-all: $(patsubst %.json, %.js, $(shell find out/ -name "*.json"))

.PHONY: clean import-pgn puzzles-all puzzles-partial merge-all json-all js-all
