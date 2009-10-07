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
optiextractor_CFLAGS = D_GNU_SOURCE -g $(PACKAGE_OPTIEXTRACTOR_CFLAGS)
optiextractor_LDFLAGS = $(PACKAGE_OPTIEXTRACTOR_LIBS)
optiextractor_SOURCES = \
	optiextractor.cc \
	Application/onfileclose.cc \
	Application/resolvedispatcher.cc \
	Application/scan1.cc \
	Application/destroydialog.cc \
	Application/ondatabaseoptimize.cc \
	Application/onfileopen.cc \
	Application/onfilequit.cc \
	Application/fillkeyvalue2.cc \
	Application/formatdate.cc \
	Application/open.cc \
	Application/application.cc \
	Application/handlerunnerstarted.cc \
	Application/executeclicked.cc \
	Application/window.cc \
	Application/filloverrides.cc \
	Application/overridenameedited.cc \
	Application/clear.cc \
	Application/openresponse.cc \
	Application/overridevalueedited.cc \
	Application/filllog.cc \
	Application/fill.cc \
	Application/overrideremove.cc \
	Application/fillkeyvalue1.cc \
	Application/error.cc \
	Application/runnerstate.cc \
	Application/runsolution.cc \
	Application/runnerresponse.cc \
	Application/initializeui.cc \
	Application/overrideadd.cc \
	Application/fillboundaries.cc \
	Application/scan2.cc \
	Application/solutionchanged.cc \
	Application/handlerunnerstopped.cc \
	Application/ondatabaseexport.cc \
	Runner/ondispatchdata.cc \
	Runner/run.cc \
	Runner/dispatchclose.cc \
	Runner/ondispatchedkilled.cc \
	Runner/cancel.cc \
	Runner/onerrordata.cc \
	Runner/writetask.cc \
	Runner/runner.cc

noinst_HEADERS = \
	Application/application.ih \
	Application/application.hh \
	Runner/runner.hh \
	Runner/runner.ih

EXTRA_DIST = $(noinst_HEADERS)
