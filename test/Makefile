JOBS:=2
PATH:=../bin:$(PATH)

tags:
	cat tags/*.jpl | xargs -d '\n' -P $(JOBS) test-tags | pv -ls `cat tags/*.jpl | wc -l` >/dev/null

.PHONY: tags
