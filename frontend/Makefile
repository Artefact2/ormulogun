PATH:=../tools:$(PATH)
FRONTEND:=index.html ormulogun.css ormulogun.js

all: ext-all deps $(FRONTEND)

index.html: index.xhtml
	echo '<!DOCTYPE html>' > $@
	tail -n +2 $< >> $@

ormulogun.js: ../src/frontend/js/main.js $(shell find ../src/frontend/js -name "*.js" -not -name "main.js")
	cat $^ > $@

ormulogun.css: ../src/frontend/scss/ormulogun.scss $(shell find ../src/frontend/scss -name "*.scss" -not -name "ormulogun.scss")
	sassc -t compressed ../src/frontend/scss/ormulogun.scss $@

deps:
	mkdir $@ || exit 1
	(cd $@; wget "https://github.com/twbs/bootstrap/releases/download/v4.0.0/bootstrap-4.0.0-dist.zip" && unzip bootstrap-4.0.0-dist.zip css/bootstrap.min.css js/bootstrap.bundle.min.js && rm bootstrap-4.0.0-dist.zip) || (rm -R $@; exit 1)
	(cd $@/js; wget "https://code.jquery.com/jquery-3.3.1.min.js") || (rm -R $@; exit 1)
# https://github.com/FezVrasta/popper.js/issues/575
	sed -i -e s/\.nodeName/\.nodeName\.toUpperCase\(\)/g deps/js/bootstrap.bundle.min.js

ext-all:
	cp -a ../ext/frontend/* .
	cp -a ../ext/puzzles .
	make -C puzzles js-all

host:
	@echo "Browse to http://[::1]:8080/index.xhtml in your browser..."
	php -S '[::]:8080' -t .

clean:
	rm -f $(FRONTEND)

dist-clean: clean
	rm -Rf deps

optisvg:
	optisvg < ormulogun.css > min.css
	while ! cmp -s min.css ormulogun.css; do mv min.css ormulogun.css; optisvg < ormulogun.css > min.css; done
	rm min.css

.PHONY: all ext-all host clean dist-clean optisvg
