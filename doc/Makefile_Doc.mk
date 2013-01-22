
.PHONY: doc

PACKAGE_DOCNAME = $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-doc

if EFL_BUILD_DOC

doc-clean:
	rm -rf doc/html/ doc/latex/ doc/man/ doc/xml/ $(PACKAGE_DOCNAME).tar*

doc: doc-clean
	$(efl_doxygen) doc/Doxyfile
	cp $(srcdir)/doc/img/* doc/html/
	cp $(srcdir)/doc/img/* doc/latex/
	rm -rf $(PACKAGE_DOCNAME).tar*
	mkdir -p $(PACKAGE_DOCNAME)/doc
	cp -R doc/html/ doc/latex/ doc/man/ doc/xml $(PACKAGE_DOCNAME)/doc
	tar cf $(PACKAGE_DOCNAME).tar $(PACKAGE_DOCNAME)/
	bzip2 -9 $(PACKAGE_DOCNAME).tar
	rm -rf $(PACKAGE_DOCNAME)/

clean-local: doc-clean

else

doc:
	@echo "Documentation not built. Run ./configure --help"

endif
