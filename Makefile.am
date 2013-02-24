AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = \
Makefile.in \
aclocal.m4 \
compile \
config.guess \
config.h.in \
config.h.in~ \
config.sub \
configure \
depcomp \
install-sh \
ltmain.sh \
missing \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.bz2 \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.gz \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-doc.tar.bz2 \
m4/libtool.m4 \
m4/lt~obsolete.m4 \
m4/ltoptions.m4 \
m4/ltsugar.m4 \
m4/ltversion.m4

EXTRA_DIST =

### Library

include src/lib/Makefile.mk
include src/lib/argb/Makefile.mk
include src/lib/compositor/Makefile.mk
include src/lib/converter/Makefile.mk
include src/lib/image/Makefile.mk
include src/lib/pool/Makefile.mk
include src/lib/rasterizer/Makefile.mk
include src/lib/renderer/Makefile.mk
include src/lib/text/Makefile.mk
include src/lib/util/Makefile.mk

### Modules

include src/modules/Makefile.mk

### Tests

include src/tests/Makefile_Tests.mk

### Examples

include src/examples/Makefile.mk

### Documentation

include doc/Makefile.mk

EXTRA_DIST += \
AUTHORS \
COPYING \
README \
enesim.pc.in \
enesim_opencl.pc.in \
doc/Doxyfile.in \
doc/img/enesim_images.svg \
doc/img/enesim.png \
doc/img/enesim_text.svg \
doc/img/renderer_checker.png \
doc/img/renderer_template.svg \
m4/ens_attribute.m4 \
m4/ens_benchmark.m4 \
m4/ens_compiler.m4 \
m4/ens_coverage.m4 \
m4/ens_cpu.m4 \
m4/ens_doxygen.m4 \
m4/ens_format.m4 \
m4/ens_modules.m4 \
m4/ens_tests.m4

sed_process = \
$(SED) \
-e 's,@VERSION\@,$(VERSION),g' \
-e 's,@VMAJ\@,$(VMAJ),g' \
-e 's,@prefix\@,$(prefix),g' \
-e 's,@exec_prefix\@,$(exec_prefix),g' \
-e 's,@libdir\@,$(libdir),g' \
-e 's,@includedir\@,$(includedir),g' \
-e 's,@pkgincludedir\@,$(pkgincludedir),g' \
-e 's,@requirements_pc\@,$(requirements_pc),g' \
-e 's,@requirements_libs\@,$(requirements_libs),g' \
-e 's,@requirements_cflags\@,$(requirements_cflags),g' \
-e 's,@build_opencl\@,$(build_opencl),g' \
-e 's,@build_opengl\@,$(build_opengl),g' \
< $< > $@ || rm $@

pc_verbose = $(pc_verbose_@AM_V@)
pc_verbose_ = $(pc_verbose_@AM_DEFAULT_V@)
pc_verbose_0 = @echo "  PC      " $@;

enesim.pc: $(top_srcdir)/enesim.pc.in Makefile
	@rm -f $@
	$(pc_verbose)$(sed_process)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = enesim.pc

if HAVE_OPENCL
pkgconfig_DATA += enesim_opencl.pc
endif

if HAVE_OPENGL
pkgconfig_DATA += enesim_opengl.pc
endif

.PHONY: coverage benchmark

# Unit tests

check-local:
	@echo "reconfigure with --enable-tests"

if ENS_ENABLE_COVERAGE
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
