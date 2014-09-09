
target = tpick
prefix ?= /usr/local

build: $(target)
	@true

install: $(target)
	install -m 0555 $(target) $(prefix)/bin/$(target)
	install -m 0555 $(target).1 $(prefix)/man/man1/$(target).1

%: %.c
	cc -Wall -o $@ $< -lcurses
