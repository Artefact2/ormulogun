SHELL=/bin/bash
TMP:=$(shell mktemp -u -p .)
JOBS:=16
PATH:=../bin:$(PATH)

import-pgn:
	pv -cN input $(INPUT) | chunk-pgn | xargs -rd '\n' -P $(JOBS) filter-pgn | xargs -rd '\n' -P $(JOBS) pgnml_to_json | pv -lcN games | zstd -19 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)

master.jpz:
	echo -n | zstd -19 - -o $@

puzzles-all:
	make puzzles START=0 END=$$(zstdcat $(INPUT) | wc -l) OUTPUT=all

puzzles:
	zstdcat $(INPUT) | tail -n +$$(( $(START) + 1 )) | head -n $$(( $(END) - $(START) + 1 )) | xargs -rd '\n' -n 1 -P $(JOBS) gen-puzzles --verbose $(PFLAGS) 2> >(tee >(grep --line-buffered move | pv -lcN moves >/dev/null) | grep --line-buffered game | pv -s $$(( $(END) - $(START) + 1 )) -lcN games >/dev/null) | pv -lcN puzzles | zstd -19 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)-$(START)-$(END).jpz

merge-all: master.jpz $(shell find . -name "*.jpz" -not -name "master.jpz")
	zstdcat $^ | pv -lcN in | merge-jl | pv -lcN out | zstd -19 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) master.jpz

out/manifest.json: $(shell find out/ -name "*.json" -not -name "manifest.json")
	update-manifest $@

out/%.json: %.jpz
	zstdcat $< | jl_to_json > $@

out/%.js: out/%.json
	cp $< $@

retag-all:
	find . -maxdepth 1 -type f -name "*.jpz" -printf "%P\0" | xargs -r0 -P $(JOBS) -n 1 -I '{}' bash -c "zstdcat {} | xargs -rd '\n' retag-puzzles | tee >(zstd -19 - -o tmp.retag.{})" | pv -ls `find . -maxdepth 1 -type f -name "*.jpz" -print0 | xargs -r0 zstdcat | wc -l` >/dev/null
	sync tmp.retag.*
	rename tmp.retag. "" tmp.retag.*

json-all: $(patsubst %.jpz, out/%.json, $(shell find . -name "*.jpz"))
	make out/manifest.json

js-all: $(patsubst %.json, %.js, $(shell find out/ -name "*.json"))

.PHONY: clean import-pgn puzzles-all puzzles-partial merge-all json-all js-all
