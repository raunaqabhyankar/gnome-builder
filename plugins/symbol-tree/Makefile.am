if ENABLE_SYMBOL_TREE_PLUGIN

DISTCLEANFILES =
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST = $(plugin_DATA)

plugindir = $(libdir)/gnome-builder/plugins
plugin_LTLIBRARIES = libsymbol-tree-plugin.la
dist_plugin_DATA = symbol-tree.plugin

libsymbol_tree_plugin_la_SOURCES = \
	gb-symbol-tree-panel.vala \
	gb-symbol-tree-resources.c \
	gb-symbol-tree-resources.h \
	$(NULL)

libsymbol_tree_plugin_la_VALAFLAGS = \
	--gresources gb-symbol-tree-plugin.gresource.xml \
	--target-glib=2.38 \
	--vapidir $(top_builddir)/libide \
	--vapidir $(top_builddir)/src \
	--pkg libide-1.0 \
	--pkg gnome-builder-1.0 \
	$(NULL)

# XXX: temporary, since we need to extract lots of src/ into plugins
libsymbol_tree_plugin_la_CFLAGS = \
	-DGETTEXT_PACKAGE="\"gnome-builder\"" \
	$(BUILDER_CFLAGS) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/documents \
	-I$(top_srcdir)/src/editor \
	-I$(top_srcdir)/src/views \
	-I$(top_srcdir)/src/tree \
	-I$(top_srcdir)/src/util \
	-I$(top_srcdir)/src/workbench \
	-I$(top_srcdir)/src/workspace \
	-I$(top_srcdir)/libide \
	-I$(top_srcdir)/contrib/egg \
	$(NULL)

libsymbol_tree_plugin_la_LIBADD = \
	$(BUILDER_LIBS) \
	$(top_builddir)/libide/libide-1.0.la \
	$(NULL)

libsymbol_tree_plugin_la_LDFLAGS = \
	$(OPTIMIZE_LDFLAGS) \
	-avoid-version \
	-module \
	$(NULL)

glib_resources_c = gb-symbol-tree-resources.c
glib_resources_h = gb-symbol-tree-resources.h
glib_resources_xml = gb-symbol-tree-plugin.gresource.xml
glib_resources_namespace = gb_symbol_tree
include $(top_srcdir)/build/autotools/Makefile.am.gresources

include $(top_srcdir)/plugins/Makefile.plugin

endif

-include $(top_srcdir)/git.mk
