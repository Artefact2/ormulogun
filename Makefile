DEFAULTS=frontend/index.html frontend/ormulogun.js frontend/gumble.js

default: frontend/deps $(DEFAULTS)

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
	emcc -Oz --memory-init-file 0 -s EXPORTED_FUNCTIONS="['_cch_init_board', '_cch_load_fen', '_cch_save_fen', '_cch_play_legal_move', '_cch_parse_lan_move', '_cch_format_lan_move', '_cch_format_san_move', '_cch_generate_moves', '_cch_is_move_legal']" gumble/build-js/src/libenginecore.a -o $@

frontend/ormulogun.js: frontend/js/main.js frontend/js/gumble.js frontend/js/load.js frontend/js/fen.js frontend/js/move.js frontend/js/movehist.js frontend/js/puzzle.js
	cat $^ > $@

clean:
	rm -f $(DEFAULTS)
	rm -Rf gumble/build gumble/build-js

dist-clean: clean
	rm -Rf frontend/deps

host:
	@echo "Browse to http://127.0.0.1:8080/index.xhtml in your browser..."
	php -S 0.0.0.0:8080 -t frontend

optisvg:
	./tools/optisvg < frontend/ormulogun.css > min.css
	while ! cmp -s min.css frontend/ormulogun.css; do mv min.css frontend/ormulogun.css; ./tools/optisvg < frontend/ormulogun.css > min.css; done
	rm min.css

.PHONY: clean dist-clean host optisvg
