%.js: %.json
	cp $< $@

js-all: $(patsubst %.json, %.js, $(shell find . -name "*.json"))

.PHONY: js-all
