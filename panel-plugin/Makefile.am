plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES = libcpugraph.la

AM_CPPFLAGS = -I${top_srcdir}

libcpugraph_la_CFLAGS = \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	@GLIB_CFLAGS@ \
	@GTK_CFLAGS@ \
	@LIBXFCE4UI_CFLAGS@ \
	@LIBXFCE4UTIL_CFLAGS@ \
	@LIBXFCE4PANEL_CFLAGS@ \
	@XFCONF_CFLAGS@

libcpugraph_la_CXXFLAGS = $(libcpugraph_la_CFLAGS)

libcpugraph_la_LIBADD = \
	@GLIB_LIBS@ \
	@GTK_LIBS@ \
	@LIBXFCE4UI_LIBS@ \
	@LIBXFCE4UTIL_LIBS@ \
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

%.desktop: %.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = \
	$(desktop_in_files) \
	meson.build \
	$(NULL)

DISTCLEANFILES = $(desktop_DATA)
