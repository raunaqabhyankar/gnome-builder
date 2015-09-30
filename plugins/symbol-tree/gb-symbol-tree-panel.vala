/* symbol-tree.vala
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

using Gb;
using GLib;
using Gtk;
using Ide;
using Peas;

namespace Gb
{
	[GtkTemplate (ui = "/org/gnome/builder/plugins/symbol-tree/gb-symbol-tree-panel.ui")]
	public class SymbolTreePanel: Gtk.Box, Gb.PanelAddin
	{
		[GtkChild]
		Gtk.SearchEntry search_entry;

		[GtkChild]
		Gb.Tree tree;

		weak Gb.View? _current_view;
		TreeBuilder tree_builder;
		ulong handler_id;

		construct {
			this.tree_builder = new SymbolTreeBuilder ();
		}

		~SymbolTreePanel ()
		{
stdout.printf ("FINALIZE\n");
		}

		public Gb.View? current_view {
			get { return this._current_view; }
			set {
				if (this._current_view != value) {
					if (value == null) {
						this._current_view = null;
						this.reload ();
					}
					this._current_view = value;
					this.reload ();
				}
			}
		}

		public Gb.PanelPosition get_position () { return Gb.PanelPosition.RIGHT; }
		public unowned string get_icon_name () { return _("lang-function-symbolic"); }
		public unowned string get_title () { return _("Symbol Tree"); }

		public void load (Gb.Workbench workbench)
		{
			this.handler_id = workbench.notify["active-view"].connect (() => {
				var view = workbench.get_active_view ();
				if (view is Gb.EditorView)
					this.current_view = (Gb.View)view;
				else
					this.current_view = null;
			});
		}

		public void unload (Gb.Workbench workbench)
		{
stdout.printf ("unload.....\n");
			workbench.disconnect (this.handler_id);
		}

		[GtkCallback]
		void on_search_entry_changed (Gtk.Editable editable) {
			var text = search_entry.get_text ();

			if (text == null || text[0] == 0) {
				this.tree.set_filter (null);
				return;
			}

			var pattern = new Ide.PatternSpec (text);
			this.tree.set_filter ((tree, node) => {
				return pattern.match (node.text);
			});
		}

		public void reload ()
		{
			/* Clear existing state */
			this.search_entry.text = "";
			this.tree.root = new Gb.TreeNode ();

			if (this._current_view == null)
				return;

			var document = this._current_view.document;
			if (!(document is Ide.Buffer))
				return;
		}
	}

	public class SymbolTreeBuilder: Gb.TreeBuilder
	{
		Ide.SymbolTree? find_symbol_tree (Gb.TreeNode? node)
		{
			if (node == null)
				return null;
			if (node.item is Ide.SymbolTree)
				return (Ide.SymbolTree)node.item;
			return find_symbol_tree (node.parent);
		}

		public override void build_node (Gb.TreeNode node)
		{
			if (!(node.item is Ide.SymbolNode))
				return;

			var symbol_tree = find_symbol_tree (node);
			var parent = node.item as Ide.SymbolNode;
			var n_children = symbol_tree.get_n_children (parent);

			for (var i = 0; i < n_children; i++) {
				var symbol = symbol_tree.get_nth_child (parent, i);
				unowned string? icon_name;

				switch (symbol.kind) {
				case Ide.SymbolKind.FUNCTION:
					icon_name = "lang-function-symbolic";
					break;

				case Ide.SymbolKind.ENUM:
					icon_name = "lang-enum-symbolic";
					break;

				case Ide.SymbolKind.ENUM_VALUE:
					icon_name = "lang-enum-value-symbolic";
					break;

				case Ide.SymbolKind.STRUCT:
					icon_name = "lang-struct-symbolic";
					break;

				case Ide.SymbolKind.CLASS:
					icon_name = "lang-class-symbolic";
					break;

				case Ide.SymbolKind.METHOD:
					icon_name = "lang-method-symbolic";
					break;

				case Ide.SymbolKind.UNION:
					icon_name = "lang-union-symbolic";
					break;

				case Ide.SymbolKind.SCALAR:
				case Ide.SymbolKind.FIELD:
				case Ide.SymbolKind.VARIABLE:
					icon_name = "lang-variable-symbolic";
					break;

				case Ide.SymbolKind.NONE:
				default:
					icon_name = null;
					break;
				}

				var child = new Gb.TreeNode () {
					text = symbol.name,
					icon_name = icon_name,
					item = symbol
				};

				node.append (child);
			}
		}

		public override bool node_activated (Gb.TreeNode node)
		{
			var workbench = this.tree.get_ancestor (typeof (Gb.Workbench)) as Gb.Workbench;
			var view_grid = workbench.get_view_grid () as Gb.ViewGrid;
			var stack = view_grid.get_last_focus () as Gb.ViewStack;

			if (node.item is Ide.SymbolNode) {
				var symbol_node = node.item as Ide.SymbolNode;
				var location = symbol_node.get_location ();
				if (location != null) {
					stack.focus_location (location);
					return true;
				}
			}

			return false;
		}
	}
}

[ModuleInit]
public void peas_register_types (GLib.TypeModule module)
{
	var peas = (Peas.ObjectModule)module;

	peas.register_extension_type (typeof (Gb.PanelAddin), typeof (Gb.SymbolTreePanel));
}
