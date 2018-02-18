JOBS:=2

test:
	sort -R tags.jl | xargs -d '\n' -n 1 -P $(JOBS) ./test-tags | pv -ls `wc -l < tags.jl` >/dev/null

.PHONY: test
