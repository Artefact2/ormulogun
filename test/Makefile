PATH:=../bin:$(PATH)

tags:
	cat tags/*.jpl | parallel -j100% -N10 --lb --pipe --round-robin test-tags | pv -ls $$(cat tags/*.jpl | wc -l) | grep '^Test failed:'

.PHONY: tags
