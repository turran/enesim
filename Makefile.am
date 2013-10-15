AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4 -I common/m4

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
CLEAN_LOCAL = enesim.pc

### Library

include src/lib/Makefile.mk
include src/lib/argb/Makefile.mk
include src/lib/compositor/Makefile.mk
include src/lib/converter/Makefile.mk
include src/lib/image/Makefile.mk
include src/lib/object/Makefile.mk
include src/lib/path/Makefile.mk
include src/lib/pool/Makefile.mk
include src/lib/rasterizer/Makefile.mk
include src/lib/renderer/Makefile.mk
include src/lib/stream/Makefile.mk
include src/lib/text/Makefile.mk
include src/lib/text/buffer/Makefile.mk
include src/lib/util/Makefile.mk

### Modules

include src/modules/Makefile.mk

### Tests

include src/tests/Makefile.mk

### Examples

include src/examples/Makefile.mk

### Documentation

include doc/Makefile.mk

### Coverage

include common/Makefile_coverage.mk

EXTRA_DIST += \
AUTHORS \
COPYING \
README \
enesim.pc.in \
enesim_opencl.pc.in \
doc/Doxyfile.in \
doc/img/enesim_images.svg \
doc/img/enesim_text.svg \
doc/img/enesim.png \
doc/img/renderer_template.svg \
doc/img/enesim_renderer_blur01.png \
doc/img/enesim_renderer_circle01.png \
doc/img/enesim_renderer_circle02.png \
doc/img/enesim_renderer_hints01.png \
doc/img/enesim_renderer_path01.png \
doc/img/enesim_renderer_path02.png \
doc/img/enesim_renderer_path03.png \
doc/img/enesim_renderer_path04.png \
doc/img/enesim_renderer_path05.png \
doc/img/enesim_renderer_perlin01.png \
doc/img/enesim_renderer_rectangle01.png \
doc/img/enesim_renderer_text_span01.png \
common/m4/ens_attribute.m4 \
common/m4/ens_benchmark.m4 \
common/m4/ens_compiler.m4 \
common/m4/ens_coverage.m4 \
common/m4/ens_cpu.m4 \
common/m4/ens_doxygen.m4 \
common/m4/ens_format.m4 \
common/m4/ens_modules.m4 \
common/m4/ens_tests.m4

sed_process = \
$(SED) \
-e 's,@VERSION\@,$(VERSION),g' \
-e 's,@VMAJ\@,$(VMAJ),g' \
-e 's,@prefix\@,$(prefix),g' \
-e 's,@exec_prefix\@,$(exec_prefix),g' \
-e 's,@libdir\@,$(libdir),g' \
-e 's,@includedir\@,$(includedir),g' \
-e 's,@pkgincludedir\@,$(pkgincludedir),g' \
-e 's,@pkglibdir\@,$(pkglibdir),g' \
-e 's,@requirements_enesim_pc\@,$(requirements_enesim_pc),g' \
-e 's,@requirements_enesim_libs\@,$(requirements_enesim_libs),g' \
-e 's,@requirements_enesim_cflags\@,$(requirements_enesim_cflags),g' \
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

if BUILD_OPENGL
pkgconfig_DATA += enesim_opengl.pc
endif

.PHONY: coverage benchmark

# Unit tests

check-local:
	@echo "reconfigure with --enable-tests"

clean-local: doc-clean
	-rm -rf $(CLEAN_LOCAL)
