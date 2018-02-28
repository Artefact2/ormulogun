FRONTEND:=frontend/index.html frontend/ormulogun.css frontend/ormulogun.js frontend/gumble.js
PATH:=./bin:$(PATH)
SOURCES:=$(shell find src -name "*.c" -not -name "gen-puzzles.c" -not -name "retag-puzzles.c")
HEADERS:=$(shell find src -name "*.h")

all: all-frontend bin/gen-puzzles bin/retag-puzzles

all-frontend: js-all frontend/deps $(FRONTEND)

bin/%: src/c/%.c $(HEADERS) $(SOURCES) ./gumble/build/src/libenginecore.a
	gcc --std=c11 -g -Og -Wall -D_POSIX_C_SOURCE=200809L -o $@ -I./gumble/include $< $(SOURCES) ./gumble/build/src/libenginecore.a

frontend/deps:
	mkdir $@ || exit 1
	(cd $@; wget "https://github.com/twbs/bootstrap/releases/download/v4.0.0/bootstrap-4.0.0-dist.zip" && unzip bootstrap-4.0.0-dist.zip css/bootstrap.min.css js/bootstrap.min.js && rm bootstrap-4.0.0-dist.zip) || (rm -R $@; exit 1)
	(cd $@/js; wget "https://code.jquery.com/jquery-3.3.1.min.js") || (rm -R $@; exit 1)

frontend/index.html: frontend/index.xhtml
	echo '<!DOCTYPE html>' > $@
	tail -n +2 $< >> $@

frontend/gumble.js:
	git submodule init
	git submodule update
	cd gumble && make clean test/PerftSuite.cmake
	mkdir gumble/build gumble/build-js
	cd gumble/build && cmake .. && make enginecore gumble
	cd gumble/build-js && CFLAGS="-Oz -DNDEBUG" emcmake cmake .. && emmake make enginecore
	emcc -Oz --memory-init-file 0 -s EXPORTED_FUNCTIONS="['_cch_init_board', '_cch_load_fen', '_cch_save_fen', '_cch_play_legal_move', '_cch_parse_lan_move', '_cch_format_lan_move', '_cch_format_san_move', '_cch_generate_moves', '_cch_is_move_legal', '_cch_is_square_checked']" gumble/build-js/src/libenginecore.a -o $@

frontend/ormulogun.js: src/js/main.js $(shell find src/js -name "*.js" -not -name "main.js")
	cat $^ > $@

frontend/ormulogun.css: $(shell find src/scss -name "*.scss")
	sassc -t compressed src/scss/ormulogun.scss $@

clean:
	rm -f $(FRONTEND) bin/gen-puzzles
	rm -Rf gumble/build gumble/build-js

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
