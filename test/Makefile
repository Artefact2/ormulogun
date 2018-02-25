JOBS:=2
PATH:=../bin:$(PATH)

tags:
	sort -R tags.jl | xargs -d '\n' -n 1 -P $(JOBS) test-tags | pv -ls `wc -l < tags.jl` >/dev/null

.PHONY: tags
