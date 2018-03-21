default: build
	@+cd build && make --no-print-directory

build:
	mkdir -p $@
	cd $@ && CFLAGS="-g -Og -Wall" cmake .. || (rm -R ../$@; exit 1)

clean:
	find build -delete

.PHONY: default clean
