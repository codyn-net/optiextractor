INCLUDES = \
	-I$(top_srcdir)\
	-I$(top_builddir)\
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(PACKAGE_OPTIEXTRACTOR_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DCONFDIR=\""$(sysconfdir)"\" \
	-DPREFIXDIR=\""$(prefix)"\" \
	-DPKGDATADIR=\""$(pkgdatadir)"\" \
	-DPKGLIBDIR=\""$(pkglibdir)"\"

bin_PROGRAMS = optiextractor
optiextractor_CFLAGS = -g $(PACKAGE_OPTIEXTRACTOR_CFLAGS)
optiextractor_LDFLAGS = $(PACKAGE_OPTIEXTRACTOR_LIBS)
optiextractor_SOURCES = \
	optiextractor.cc \
	application.cc \
	exporter.cc \
	runner.cc \
	window.cc \
	dispatcher.cc \
	utils.cc

noinst_HEADERS = \
	application.hh \
	exporter.hh \
	runner.hh \
	window.hh \
	dispatcher.hh \
	utils.hh

EXTRA_DIST = $(noinst_HEADERS)
