dnl
dnl Copyright (c) 2004 - 2022 The Xfce development team
dnl
dnl 2004 Alexander Nordfelth <alex.nordfelth@telia.com>
dnl

dnl Version information
XDT_VERSION_INIT([1.2.10], [git])

dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2004-2024 The Xfce development team. All rights reserved.])
AC_INIT([xfce4-cpugraph-plugin],[xdt_version],[https://gitlab.xfce.org/panel-plugins/xfce4-cpugraph-plugin/],[xfce4-cpugraph-plugin])
AC_PREREQ([2.69])
AC_CONFIG_MACRO_DIRS([m4])
AC_REVISION([xdt_version_build])

dnl Initialize automake
AM_INIT_AUTOMAKE([1.11 dist-bzip2 tar-ustar no-dist-gzip foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE()
AM_SILENT_RULES([yes])

dnl Check for basic programs
AC_PROG_CC()
AC_PROG_CXX()
LT_PATH_LD([])
AC_PROG_INSTALL
AX_CXX_COMPILE_STDCXX_17()
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl configure the panel plugin
AC_CHECK_FUNCS_ONCE([malloc_trim])
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.22.0])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-2.0], [4.16.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.16.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.17.2])
XDT_CHECK_PACKAGE([XFCONF], [libxfconf-0], [4.12.0])

dnl translations
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl Check for debugging support
XDT_FEATURE_DEBUG([xdt_debug_default])

AC_CONFIG_FILES([
Makefile
panel-plugin/Makefile
icons/Makefile
icons/16x16/Makefile
icons/24x24/Makefile
icons/32x32/Makefile
icons/128x128/Makefile
icons/scalable/Makefile
po/Makefile.in
])
AC_OUTPUT
