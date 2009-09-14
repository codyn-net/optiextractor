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
	Runner/runner.cc \
	Runner/writetask.cc \
	Runner/ondispatchdata.cc \
	Runner/run.cc \
	Runner/cancel.cc \
	Runner/dispatchclose.cc \
	Runner/ondispatchedkilled.cc \
	Runner/onerrordata.cc \
	Application/fillkeyvalue1.cc \
	Application/handlerunnerstarted.cc \
	Application/fillkeyvalue2.cc \
	Application/overrideadd.cc \
	Application/open.cc \
	Application/fillboundaries.cc \
	Application/resolvedispatcher.cc \
	Application/filllog.cc \
	Application/handlerunnerstopped.cc \
	Application/scan2.cc \
	Application/runnerstate.cc \
	Application/executeclicked.cc \
	Application/destroydialog.cc \
	Application/overrideremove.cc \
	Application/fill.cc \
	Application/application.cc \
	Application/initializeui.cc \
	Application/error.cc \
	Application/scan1.cc \
	Application/formatdate.cc \
	Application/runsolution.cc \
	Application/overridenameedited.cc \
	Application/clear.cc \
	Application/window.cc \
	Application/overridevalueedited.cc \
	Application/runnerresponse.cc \
	Application/solutionchanged.cc

noinst_HEADERS = \
	Runner/runner.hh \
	Runner/runner.ih \
	Application/application.hh \
	Application/application.ih

EXTRA_DIST = $(noinst_HEADERS)
