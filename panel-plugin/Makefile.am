plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =						\
	libcpugraph.la

libcpugraph_la_LDFLAGS =					\
	-avoid-version						\
	-export-dynamic						\
	-module							

libcpugraph_la_SOURCES =					\
	cpu.cc							\
	cpu.h						\
	os.cc							\
	os.h

libcpugraph_la_CFLAGS =						\
        -DPACKAGE_LOCALE_DIR=\"$(localedir)\"			\
	@XFCE4_PANEL_CFLAGS@

INCLUDES = 							\
	@XFCE4_PANEL_CFLAGS@

LIBS = 								\
	-lstdc++

libcpugraph_la_LIBADD = 					\
	@XFCE4_PANEL_LIBS@

libcpugraph_CFLAGS =						\
	@XFCE4_PANEL_CFLAGS@

libcpugraph_LIBS =						\
	@XFCE4_PANEL_LIBS@
