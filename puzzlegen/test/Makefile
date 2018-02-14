JOBS:=2

test:
	xargs -d '\n' -P $(JOBS) ./test-tags < tags.jl | pv -ls `wc -l < tags.jl` >/dev/null

.PHONY: test
