TMP:=$(shell mktemp -u -p .)
PATH:=../tools:./tools:./build:$(PATH)

build/%: src/%.c
	mkdir -p build
	clang -g -Og -Wall -o $@ $<

book.tsv.zst: build/merge-books
	mkdir $(TMP)
# Do not use --round-robin to keep memory usage reasonable
	pv $(INPUT) | chunk-pgn | parallel -j100% -N10000 --pipe --files --compress --compress-program pzstd --tmpdir $(TMP) gen-book >/dev/null
	parallel -q sh -c 'mkfifo {}.fifo; zstdcat {} > {}.fifo &' ::: $(TMP)/par*.par
	merge-books 0 $(TMP)/*.fifo | pv -l | pzstd -q - -o $@
	sync $@
	rm -Rf $(TMP)

clean:
	rm book.json
