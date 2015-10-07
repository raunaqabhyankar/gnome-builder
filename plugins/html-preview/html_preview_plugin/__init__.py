#!/usr/bin/env python3

#
# html_preview_plugin.py
#
# Copyright (C) 2015 Christian Hergert <chris@dronelabs.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

from gettext import gettext as _

import gi
import os
import re

gi.require_version('Builder', '1.0')
gi.require_version('Ide', '1.0')
gi.require_version('WebKit2', '4.0')

from gi.repository import Builder
from gi.repository import GLib
from gi.repository import Gio
from gi.repository import Gtk
from gi.repository import GObject
from gi.repository import Ide
from gi.repository import WebKit2
from gi.repository import Peas

TAG = re.compile(u'(<.+?>)')
LINK_REF = re.compile(u'^\[.+?\]:')
TABLEROW_REF = re.compile(u'^\|(?![ \t]*[-:])(.+)')
IDENT = u'53bde44bb4f156e94a85723fe633c80b54f11f69'
RIDENT = IDENT[::-1]
B = re.compile(u'(?<=[a-zA-Z\d\u4e00-\u9fa5])\B(?=[a-zA-Z\d\u4e00-\u9fa5])')

class HtmlPreviewData(GObject.Object, Builder.ApplicationAddin):
    MARKDOWN_CSS = None
    MARKED_JS = None
    MARKDOWN_VIEW_JS = None

    def do_load(self, app):
        HtmlPreviewData.MARKDOWN_CSS = self.get_data('markdown.css')
        HtmlPreviewData.MARKED_JS = self.get_data('marked.js')
        HtmlPreviewData.MARKDOWN_VIEW_JS = self.get_data('markdown-view.js')

    def get_data(self, name):
        engine = Peas.Engine.get_default()
        info = engine.get_plugin_info('html_preview_plugin')
        datadir = info.get_data_dir()
        path = os.path.join(datadir, name)
        return open(path, 'r').read()


class HtmlPreviewAddin(GObject.Object, Builder.EditorViewAddin):
    def do_load(self, editor):
        self.menu = HtmlPreviewMenu(editor.get_menu())

        actions = editor.get_action_group('view')
        action = Gio.SimpleAction(name='preview-as-html', enabled=True)
        action.connect('activate', lambda *_: self.preview_activated(editor))
        actions.add_action(action)

    def do_unload(self, editor):
        self.menu.hide()

    def do_language_changed(self, language_id):
        self.menu.hide()
        if language_id in ('html', 'markdown'):
            self.menu.show()

    def preview_activated(self, editor):
        document = editor.get_document()
        view = HtmlPreviewView(document, visible=True)
        stack = editor.get_ancestor(Builder.ViewStack)
        stack.add(view)

class HtmlPreviewMenu:
    exten = None

    def __init__(self, menu):
        self.exten = Builder.MenuExtension.new_for_section(menu, 'preview-section')

    def show(self):
        item = Gio.MenuItem.new(_("Preview as HTML"), 'view.preview-as-html')
        self.exten.append_menu_item(item)

    def hide(self):
        self.exten.remove_items()

class HtmlPreviewView(Builder.View):
    markdown = False

    def __init__(self, document, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.document = document
        self.line = -1
        self.loaded = False
        self.changed = False

        self.webview = WebKit2.WebView(visible=True, expand=True)
        self.add(self.webview)

        settings = self.webview.get_settings()
        settings.enable_html5_local_storage = False

        language = document.get_language()
        if language and language.get_id() == 'markdown':
            self.markdown = True

        self.webview.connect ('load-changed', self.on_webview_loaded)
        document.connect('changed', self.on_changed)
        self.on_changed(document)

        document.connect('notify::cursor-position', self.on_cursor_position_changed)
        self.webview.connect_object ('destroy', self.on_webview_closed, document)

    def do_get_title(self):
        title = self.document.get_title()
        return '%s (Preview)' % title

    def do_get_document(self):
        return self.document

    def get_markdown(self, text):
        lines = text.split('\n')
        new_lines = []

        for index, line in enumerate(lines):
            prefix = IDENT + str(index).zfill(6)

            if (line != ''):
                if TAG.search(line) is not None:
                    new_line = TAG.sub(u'\\1 ' + prefix, line, 1)
                elif LINK_REF.search(line) is not None:
                    # we don't put ident on link reference
                    new_line = line
                elif TABLEROW_REF.search(line) is not None:
                    # we put an ident only on row, after the pipe, not on a header
                    new_line = TABLEROW_REF.sub(u'|' + prefix + '\\1', line, 1)
                else:
                    new_line = B.sub(prefix, line, 1)

                new_lines.append(new_line.replace("\\n", "\\\\n").replace("\\r", "\\\\r").replace("\\t", "\\\\t"))
            else:
                new_lines.append(' ')

        # Generate our custom HTML with replaced text
        text = '\n'.join(new_lines).replace("\"", "\\\"").replace("\n", "\\n")

        params = (HtmlPreviewData.MARKDOWN_CSS,
                  text,
                  HtmlPreviewData.MARKED_JS,
                  HtmlPreviewData.MARKDOWN_VIEW_JS)

        return """
<html>
 <head>
  <style>%s</style>
  <script>var str="%s";</script>
  <script>%s</script>
  <script>%s</script>
 </head>
 <body>
  <div class="markdown-body" id="preview">
  </div>
 </body>
</html>
""" % params

    def on_changed(self, document):
        self.loaded = False
        self.changed = True
        base_uri = self.document.get_file().get_file().get_uri()
        begin, end = self.document.get_bounds()
        text = self.document.get_text(begin, end, True)

        if self.markdown:
            text = self.get_markdown(text)

        self.webview.load_html(text, base_uri)

    def on_webview_closed (self, document):
        document.disconnect_by_func(self.on_changed)
        document.disconnect_by_func(self.on_cursor_position_changed)

    def on_webview_loaded (self, webview, event):
        if event == WebKit2.LoadEvent.FINISHED and self.loaded == False:
            self.loaded = True
            self.webview.run_javascript('preview();', None, self.on_webview_loaded_cb)

    def on_webview_loaded_cb (self, webview, result):
        res = webview.run_javascript_finish(result)

        self.on_cursor_position_changed (None, self.document)

    def on_cursor_position_changed (self, pspec, document):
        if self.loaded:
            # Determine the location of the insert cursor.
            insert = self.document.get_insert()
            iter = self.document.get_iter_at_mark(insert)
            line = iter.get_line()

            if (line != self.line or self.changed):
                self.changed = False
                # give some context while editing.
                # So try to give 10 lines of context above.
                context = 0

                script = """
var id;
var line = ({0} > 0) ? {0} : 0;

for (; line >= 0; --line) {{
  id = rFlagSign + ("000000" + line).slice(-6);
  if (document.getElementById(id)) {{
    break;
  }}
}}

location.hash = id;
""".format(line - context)

                self.webview.run_javascript(script, None, self.cursor_position_changed_cb)
                self.line = line

    def cursor_position_changed_cb (self, webview, result):
        res = webview.run_javascript_finish(result)
