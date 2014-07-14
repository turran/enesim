#!/bin/sh

rm -rf autom4te.cache
rm -f aclocal.m4 ltmain.sh

# Make sure we have common
if [ ! -d common/m4 ]; then
	echo "+ Setting up common submodule"
	git submodule init
fi
git submodule update --rebase

autoreconf -f -i -v --warnings=all || exit 1

if [ -z "$NOCONFIGURE" ]; then
	./configure "$@"
fi
