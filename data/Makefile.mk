EXTRA_DIST += data/enesim.ender

if ENS_BUILD_ENDER

descriptionsdir = @ENDER_DESCRIPTIONS_DIR@
descriptions_DATA = data/enesim.ender

ender: doxygen
	@xsltproc --param lib "'enesim'" --param version 0 --param case "'underscore'" @ENDER_TOOLS_DIR@/fromdoxygen.xslt $(top_builddir)/doc/xml/Enesim_8h.xml > out.xml
	@xmllint -format out.xml > $(top_srcdir)/data/enesim-main.ender
	@xsltproc --param tomerge "'$(top_srcdir)/data/enesim-callbacks.ender'" $(top_srcdir)/data/merge.xslt $(top_srcdir)/data/enesim-main.ender > out.xml
	@xmllint -format out.xml > $(top_srcdir)/data/enesim.ender
	@rm out.xml

endif
