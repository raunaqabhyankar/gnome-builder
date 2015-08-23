plugindir = $(libdir)/gnome-builder/plugins
plugin_LTLIBRARIES = libgit-plugin.la
plugin_DATA = git.plugin

libgit_plugin_la_SOURCES = \
	git-plugin.c \
	ide-git-buffer-change-monitor.c \
	ide-git-buffer-change-monitor.h \
	ide-git-remote-callbacks.c \
	ide-git-remote-callbacks.h \
	ide-git-vcs.c \
	ide-git-vcs.h \
	$(NULL)

libgit_plugin_la_CFLAGS = \
	$(BUILDER_CFLAGS) \
	-I$(top_srcdir)/libide \
	-I$(top_srcdir)/contrib/egg \
	$(NULL)

libgit_plugin_la_LDFLAGS = \
	-avoid-version \
	-module \
	$(NULL)

-include $(top_srcdir)/git.mk
