plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =						\
	libcpugraph.la

libcpugraph_la_LDFLAGS =					\
	-avoid-version						\
	-export-dynamic						\
	-module							

libcpugraph_la_SOURCES =					\
	cpu.c							\
	cpu.h						\
	os.c							\
	os.h

libcpugraph_la_CFLAGS =						\
        -DPACKAGE_LOCALE_DIR=\"$(localedir)\"			\
	@XFCE4_PANEL_CFLAGS@

libcpugraph_la_LIBADD = 					\
	@XFCE4_PANEL_LIBS@

