#!/usr/bin/make -f
# Waf Makefile wrapper
WAF_HOME=/home/fujisawa/work/matrix

all:
#@/home/fujisawa/work/matrix/waf build

all-debug:
	@/home/fujisawa/work/matrix/waf -v build

all-progress:
	@/home/fujisawa/work/matrix/waf -p build

install:
	/home/fujisawa/work/matrix/waf install --yes;

uninstall:
	/home/fujisawa/work/matrix/waf uninstall

clean:
	@/home/fujisawa/work/matrix/waf clean

distclean:
	@/home/fujisawa/work/matrix/waf distclean
	@-rm -rf build
	@-rm -f Makefile

check:
	@/home/fujisawa/work/matrix/waf check

dist:
	@/home/fujisawa/work/matrix/waf dist

.PHONY: clean dist distclean check uninstall install all

