DEFAULTS=frontend/index.html $(patsubst %.json, %.js, $(shell find frontend/puzzles -name "*.json"))

default: frontend/deps $(DEFAULTS)

frontend/deps:
	mkdir $@ || exit 1
	(cd $@; wget "https://github.com/twbs/bootstrap/releases/download/v4.0.0/bootstrap-4.0.0-dist.zip" && unzip bootstrap-4.0.0-dist.zip css/bootstrap.min.css js/bootstrap.min.js && rm bootstrap-4.0.0-dist.zip) || (rm -R $@; exit 1)
	(cd $@/js; wget "https://code.jquery.com/jquery-3.3.1.min.js") || (rm -R $@; exit 1)

frontend/index.html: frontend/index.xhtml
	echo '<!DOCTYPE html>' > $@
	tail -n +2 $< >> $@

frontend/puzzles/%.js: frontend/puzzles/%.json
	cp $< $@

clean:
	rm -f $(DEFAULTS)

dist-clean: clean
	rm -Rf frontend/deps

host:
	@echo "Browse to http://127.0.0.1:8080/index.xhtml in your browser..."
	php -S 0.0.0.0:8080 -t frontend

.PHONY: clean dist-clean host
