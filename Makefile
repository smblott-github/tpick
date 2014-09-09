
target = tpick
prefix ?= /usr/local

build: $(target)
	@true

install: $(target)
	install -m 0755 $(target) $(prefix)/bin/$(target)
	install -m 0644 $(target).1 $(prefix)/man/man1/$(target).1

uninstall:
	! [ -f $(prefix)/bin/$(target) ] || rm -v $(prefix)/bin/$(target)
	! [ -f $(prefix)/man/man1/$(target).1 ] || rm -v $(prefix)/man/man1/$(target).1
%: %.c
	cc -Wall -o $@ $< -lcurses
