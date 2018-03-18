SHELL:=/bin/bash
TMP:=$(shell mktemp -u -p .)
PATH:=../tools:../build/src:$(PATH)

import-pgn:
	pv -cN input $(INPUT) | chunk-pgn | parallel -j100% --lb --pipe --round-robin filter-pgn | parallel -j100% --lb --pipe --round-robin pgnml_to_json | pv -lcN games | zstd -19 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)

puzzles:
	zstdcat $(INPUT) | tail -n +$$(( $(START) + 1 )) | head -n $$(( $(END) - $(START) + 1 )) | parallel -q -j100% -N1 --lb --pipe --round-robin gen-puzzles --verbose $(PFLAGS) 2> >(tee >(grep --line-buffered move | pv -lcN moves >/dev/null) | grep --line-buffered game | pv -s $$(( $(END) - $(START) + 1 )) -lcN games >/dev/null) | pv -lcN puzzles | zstd -19 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)-$(START)-$(END).jpz

puzzles-all:
	make puzzles START=0 END=$$(zstdcat $(INPUT) | wc -l)

out/manifest.json: $(shell find out/ -name "*.json" -not -name "manifest.json")
	update-manifest $@

out/%.json: %.jpz
	zstdcat $< | jl_to_json > $@

out/%.js: out/%.json
	cp $< $@

retag:
	zstdcat $(INPUT) | zstdcat | parallel -j100% -N100 --keep-order --pipe retag-puzzles | pv -ls $$(zstdcat $(INPUT) | wc -l) | zstd -19 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(INPUT)

retag-all:
	find . -maxdepth 1 -type f -name "*.jpz" -print0 | parallel -0 -N1 -j2 make -j1 -s retag INPUT={}

json-all: $(patsubst %.jpz, out/%.json, $(shell find . -name "*.jpz"))
	make out/manifest.json

.PHONY: clean import-pgn puzzles puzzles-all retag retag-all json-all
