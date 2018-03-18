default: build
	+cd build && make

build:
	mkdir -p $@
	cd $@ && CFLAGS="-g -Og -Wall" cmake .. || (rm -R ../$@; exit 1)

clean: build
	+cd build && make clean

dist-clean:
	rm -Rf build

.PHONY: default build clean dist-clean
