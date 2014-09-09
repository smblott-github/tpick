
.PHONY: build install

target = tpick
prefix ?= /usr/local

install = $(prefix)/bin/$(target) $(prefix)/man/man1/$(target).1

build: $(target)
	@true

install: $(install)
	@true

$(prefix)/bin/$(target): $(target)
	install -m 0755 $< $@

$(prefix)/man/man1/$(target).1: $(target).1
	install -m 0644 $< $@

uninstall:
	rm -vf $(install)

%: %.c
	cc -Wall -o $@ $< -lcurses
