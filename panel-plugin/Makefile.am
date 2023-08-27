plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES = libcpugraph.la

AM_CPPFLAGS = -I${top_srcdir}

libcpugraph_la_CFLAGS = \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	@LIBXFCE4UI_CFLAGS@ \
	@LIBXFCE4PANEL_CFLAGS@ \
	@XFCONF_CFLAGS@

libcpugraph_la_CXXFLAGS = $(libcpugraph_la_CFLAGS)

libcpugraph_la_LIBADD = \
	@LIBXFCE4UI_LIBS@ \
	@LIBXFCE4PANEL_LIBS@ \
	-lm \
	@XFCONF_LIBS@

libcpugraph_la_SOURCES = \
	mode.cc \
	mode.h \
	cpu.cc \
	cpu.h \
	os.cc \
	os.h \
	plugin.h \
	plugin.c \
	properties.cc \
	properties.h \
	settings.cc \
	settings.h

libcpugraph_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

#
# .desktop file
#
desktop_in_files = cpugraph.desktop.in

desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_files)

DISTCLEANFILES = $(desktop_DATA)
