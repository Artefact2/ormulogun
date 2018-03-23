TMP:=$(shell mktemp -u -p .)
PATH:=../tools:../tools/bookgen:../build/src:../build/src/bookgen:$(PATH)

default:
	@echo "See README.md for usage."

book.tsv.zst:
	mkdir $(TMP)
	pv $(INPUT) | chunk-pgn | parallel --block-size 100M --pipe --files --compress --compress-program zstd --tmpdir $(TMP) gen-book >/dev/null
	batch-merge 100 2 $@ $(TMP)/par*.par
	sync $@

eco.tsv:
	mkdir $(TMP)
	pv $(INPUT) | chunk-pgn | parallel -j200% --block-size 100M --pipe --ungroup gen-eco $(TMP) >/dev/null
	find $(TMP) -mindepth 1 -maxdepth 1 -type d | parallel --bar batch-merge 10 2 {}.tsv.zst {}'/*.tsv.zst'
	find $(TMP) -mindepth 1 -maxdepth 1 -type f -name '*.tsv.zst' | parallel --bar -N10 --lb merge-ecos | prune-ecos | sort > $@
	sync $@

clean:
	find . -mindepth 1 -maxdepth 1 -type d -name 'tmp.*' -exec find {} -delete ';'

.PHONY: default clean
