## Process this file with automake to produce Makefile.in

SUBDIRS = enesim

if HAVE_EMAGE
SUBDIRS += emage
endif

if HAVE_ETEX
SUBDIRS += etex
endif

MAINTAINERCLEANFILES = \
Makefile.in \
aclocal.m4 \
config.guess \
config.sub \
configure \
install-sh \
ltconfig \
ltmain.sh \
missing mkinstalldirs \
stamp-h.in \
build-stamp \
configure-stamp \
depcomp \
enesim.pc \
enesim_opencl.pc \
emage.pc \
etex.pc \
enesim_docs.tar.gz \
m4/ltsugar.m4 \
m4/libtool.m4 \
m4/ltversion.m4 \
m4/lt~obsolete.m4 \
m4/ltoptions.m4 \
config.h.in~ \
config.h.in \
config.h

EXTRA_DIST = \
AUTHORS \
COPYING \
gendoc \
Doxyfile \
README \
enesim.pc.in \
enesim_opencl.pc.in \
emage.pc.in \
etex.pc.in \
m4/efl_attribute.m4 \
m4/efl_benchmark.m4 \
m4/efl_compiler_flag.m4 \
m4/efl_coverage.m4 \
m4/efl_cpu.m4 \
m4/efl_tests.m4 \
m4/enesim_format.m4 \
external


ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = enesim.pc

if HAVE_OPENCL
pkgconfig_DATA += enesim_opencl.pc
endif

if HAVE_OPENGL
pkgconfig_DATA += enesim_opengl.pc
endif

if HAVE_EMAGE
pkgconfig_DATA += emage.pc
endif

if HAVE_ETEX
pkgconfig_DATA += etex.pc
endif

.PHONY: coverage benchmark

# Unit tests

check-local:
	@echo "reconfigure with --enable-tests"

if EFL_ENABLE_COVERAGE
lcov-reset:
	@rm -rf coverage
	@find . -name "*.gcda" -exec rm {} \;
	@lcov --directory . --zerocounters

lcov-report:
	@mkdir coverage
	@lcov --compat-libtool --directory . --capture --output-file coverage/coverage.info
	@lcov -l coverage/coverage.info | grep -v "`cd $(top_srcdir) && pwd`" | cut -d: -f1 > coverage/remove
	@lcov -r coverage/coverage.info `cat coverage/remove` > coverage/coverage.cleaned.info
	@rm coverage/remove
	@mv coverage/coverage.cleaned.info coverage/coverage.info
	@genhtml -t "$(PACKAGE_STRING)" -o coverage coverage/coverage.info

coverage:
	@make lcov-reset
	@make check
	@make lcov-report

clean-local:
	@rm -rf coverage
else
lcov-reset:
	@echo "reconfigure with --enable-coverage"

lcov-report:
	@echo "reconfigure with --enable-coverage"

coverage:
	@echo "reconfigure with --enable-tests --enable-coverage"
endif
