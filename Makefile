FRONTEND:=frontend/index.html frontend/ormulogun.css frontend/ormulogun.js
PATH:=./bin:$(PATH)
SOURCES:=$(shell find src -name "*.c" -not -name "gen-puzzles.c" -not -name "retag-puzzles.c")
HEADERS:=$(shell find src -name "*.h")

all: all-frontend bin/gen-puzzles bin/retag-puzzles

all-frontend: js-all frontend-ext frontend/deps $(FRONTEND)

bin/%: src/c/%.c $(HEADERS) $(SOURCES) ext/gumble/build/src/libenginecore.a
	clang --std=c11 -g -Og -Wall -D_POSIX_C_SOURCE=200809L -o $@ -I./ext/gumble/include $< $(SOURCES) ./ext/gumble/build/src/libenginecore.a

ext/gumble/build/src/libenginecore.a:
	git submodule update --recursive --init
	make -C ext/gumble clean test/PerftSuite.cmake
	mkdir -p ext/gumble/build
	cd ext/gumble/build && cmake ..
	make -C ext/gumble/build enginecore

frontend-ext:
	git submodule update --recursive --init
	cp -a ext/frontend/* frontend

frontend/deps:
	mkdir $@ || exit 1
	(cd $@; wget "https://github.com/twbs/bootstrap/releases/download/v4.0.0/bootstrap-4.0.0-dist.zip" && unzip bootstrap-4.0.0-dist.zip css/bootstrap.min.css js/bootstrap.bundle.min.js && rm bootstrap-4.0.0-dist.zip) || (rm -R $@; exit 1)
	(cd $@/js; wget "https://code.jquery.com/jquery-3.3.1.min.js") || (rm -R $@; exit 1)
# https://github.com/FezVrasta/popper.js/issues/575
	sed -i -e s/\.nodeName/\.nodeName\.toUpperCase\(\)/g frontend/deps/js/bootstrap.bundle.min.js

frontend/index.html: frontend/index.xhtml
	echo '<!DOCTYPE html>' > $@
	tail -n +2 $< >> $@

frontend/ormulogun.js: src/js/main.js $(shell find src/js -name "*.js" -not -name "main.js")
	cat $^ > $@

frontend/ormulogun.css: $(shell find src/scss -name "*.scss")
	sassc -t compressed src/scss/ormulogun.scss $@

clean:
	make -C ext clean
	rm -f $(FRONTEND) bin/gen-puzzles bin/retag-puzzles

dist-clean: clean
	rm -Rf frontend/deps

host:
	@echo "Browse to http://[::1]:8080/index.xhtml in your browser..."
	php -S '[::]:8080' -t frontend

optisvg:
	optisvg < frontend/ormulogun.css > min.css
	while ! cmp -s min.css frontend/ormulogun.css; do mv min.css frontend/ormulogun.css; optisvg < frontend/ormulogun.css > min.css; done
	rm min.css

js-all:
	+make -C puzzles $@

.PHONY: all all-frontend clean dist-clean host optisvg js-all
