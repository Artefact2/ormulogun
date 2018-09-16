SHELL:=/bin/bash
TMP:=$(shell mktemp -u -p .)
PATH:=../tools:../tools/puzzlegen:../build/src:../build/src/puzzlegen:$(PATH)

default:
	@echo "See README.md for usage."

import-pgn:
	pv $(INPUT) | chunk-pgn | parallel --lb --pipe --round-robin filter-pgn | zstd -19 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)

puzzles:
	zstdcat $(INPUT) | tail -n +$$(( $(START) + 1 )) | head -n $$(( $(END) - $(START) + 1 )) | parallel -q -N1 --lb --pipe --round-robin gen-puzzles --verbose $(PFLAGS) 2> >(grep --line-buffered '^move' | pv -ls $$(zstdcat $(INPUT) | tail -n +$$(( $(START) + 1)) | head -n $$(( $(END) - $(START) + 1 )) | tr '\n' ',' | tr -dc ',' | wc -c) >/dev/null) | zstd -19 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(OUTPUT)-$(START)-$(END).jpz

puzzles-all:
	make puzzles START=0 END=$$(zstdcat $(INPUT) | wc -l)

out/manifest.json: $(shell find out/ -name "*.json" -not -name "manifest.json")
	update-manifest $@

out/%.json: %.jpz
	zstdcat $< | strip-jl | jl_to_json > $@

inplace-op:
	zstdcat $(INPUT) | parallel -N100 --keep-order --pipe $(COMMAND) | pv -ls $$(zstdcat $(INPUT) | wc -l) | zstd -19 - -o $(TMP)
	sync $(TMP)
	mv $(TMP) $(INPUT)

inplace-op-all:
	find . -maxdepth 1 -type f -name "*.jpz" -print0 | parallel -0 -N1 make -j1 -s inplace-op INPUT={}

retag:
	+make inplace-op COMMAND=retag-puzzles

retag-all:
	+make inplace-op-all COMMAND=retag-puzzles

regen:
	+make inplace-op COMMAND=regen-puzzles

regen-all:
	+make inplace-op-all COMMAND=regen-puzzles

json-all: $(patsubst %.jpz, out/%.json, $(shell find . -maxdepth 1 -name "*.jpz"))
	make out/manifest.json

test:
	parallel -N10 --lb --pipe --round-robin test-tags < tags.jpl | pv -ls $$(wc -l < tags.jpl) | (grep -v '^Test OK'; exit 0)

.PHONY: default clean import-pgn puzzles puzzles-all inplace-op inplace-op-all retag retag-all regen regen-all json-all test
