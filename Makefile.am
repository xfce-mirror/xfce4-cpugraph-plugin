@SET_MAKE@

# Note: The order of directories in this list isn't arbitrary
SUBDIRS = \
	panel-plugin \
	po \
	icons

distclean-local:
	rm -rf *.cache *~

distuninstallcheck_listfiles = \
	find . -type f -print | grep -v ./share/icons/hicolor/icon-theme.cache

AUTOMAKE_OPTIONS = 1.8 dist-bzip2

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

EXTRA_DIST = \
	meson.build \
	po/meson.build \
	xfce-revision.h.in \
	xfce4cpp \
	README.md \
	$(NULL)
