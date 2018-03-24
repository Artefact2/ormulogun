TMP:=$(shell mktemp -u -p .)
PATH:=../tools:../tools/bookgen:../build/src:../build/src/bookgen:$(PATH)

default:
	@echo "See README.md for usage."

book.tsv.zst:
	mkdir $(TMP)
	pv $(INPUT) | chunk-pgn | parallel --block-size 100M --pipe --files --compress --compress-program pzstd --tmpdir $(TMP) gen-book >/dev/null
	zstdize merge-books-mt 2 -- $(TMP)/par*.par | prune-book | pv -l | pzstd -19 - -fqo $@
	sync $@

eco.tsv:
	mkdir $(TMP)
	pv $(INPUT) | chunk-pgn | parallel --block-size 100M --pipe --files --compress --compress-program pzstd --tmpdir $(TMP) gen-eco >/dev/null
	zstdize merge-books-mt 0 -- $(TMP)/par*.par | pv -l | prune-ecos | finalize-ecos > $@
	sync $@

clean:
	find . -mindepth 1 -maxdepth 1 -type d -name 'tmp.*' -exec find {} -delete ';'

.PHONY: default clean
