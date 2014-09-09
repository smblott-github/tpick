
target = tpick
prefix ?= /usr/local

build: $(target)
	@true

install: $(target)
	install -m 0555 $(target) $(prefix)/bin/$(target)

%: %.c
	cc -Wall -o $@ $< -lcurses
