JOBS:=16
PATH:=../bin:$(PATH)

tags:
	cat tags/*.jpl | xargs -n 1 -d '\n' -P $(JOBS) test-tags | pv -ls `cat tags/*.jpl | wc -l` >/dev/null

.PHONY: tags
