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

bin_PROGRAMS = optiextractor1
optiextractor1_CFLAGS = -g $(PACKAGE_OPTIEXTRACTOR_CFLAGS)
optiextractor1_LDFLAGS = $(PACKAGE_OPTIEXTRACTOR_LIBS)
optiextractor1_SOURCES = \
	optiextractor.cc \
	application.cc \
	runner.cc

noinst_HEADERS = \
	application.hh \
	runner.hh

EXTRA_DIST = $(noinst_HEADERS)
