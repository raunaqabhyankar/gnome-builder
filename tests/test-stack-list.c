/* test-stack-list.c
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

/* this code leaks, it's just a quick prototype */

#include <gtk/gtk.h>

#include "egg-directory-model.h"
#include "egg-stack-list.h"

#define TEST_CSS \
  "EggStackList GtkListBox.view {" \
  " background-color: #fafafa;" \
  "}\n" \
  "EggStackList GtkListBox GtkListBoxRow {" \
  " background-color: #f2f2f2;" \
  " color: #2e3436;" \
  " padding-bottom: 3px;" \
  "}\n" \
  "EggStackList GtkListBox.stack-header GtkListBoxRow {" \
  " background-color: #fafafa;" \
  " color: #919191;" \
  "}\n" \
  "EggStackList GtkListBox.stack-header GtkListBoxRow:last-child {" \
  " border-bottom: 1px solid #dbdbdb;" \
  " color: #000000;" \
  "}\n"

static GtkWidget *
create_row_func (gpointer item,
                 gboolean is_header,
                 gpointer user_data)
{
  GFileInfo *file_info = item;
  GFile *parent = user_data;
  GtkWidget *row;
  GtkWidget *box;
  GtkWidget *image;
  GtkWidget *label;
  g_autoptr(GIcon) local_gicon = NULL;
  GObject *gicon;
  const gchar *display_name;
  g_autoptr(GFile) copy = NULL;

  g_assert (!file_info || G_IS_FILE_INFO (file_info));
  g_assert (G_IS_FILE (parent));

  if (parent == NULL)
    parent = copy = g_file_new_for_path (".");

  row = g_object_new (GTK_TYPE_LIST_BOX_ROW,
                      "visible", TRUE,
                      NULL);

  g_object_set_data_full (G_OBJECT (row),
                          "G_FILE_INFO",
                          file_info ? g_object_ref (file_info) : NULL,
                          g_object_unref);

  g_object_set_data_full (G_OBJECT (row),
                          "G_FILE",
                          g_file_get_child (parent, file_info ? g_file_info_get_name (file_info) : "."),
                          g_object_unref);

  box = g_object_new (GTK_TYPE_BOX,
                      "border-width", 3,
                      "spacing", 6,
                      "orientation", GTK_ORIENTATION_HORIZONTAL,
                      "visible", TRUE,
                      NULL);
  gtk_container_add (GTK_CONTAINER (row), GTK_WIDGET (box));

  if (is_header)
    {
      local_gicon = g_themed_icon_new ("folder-open-symbolic");
      gicon = G_OBJECT (local_gicon);
    }
  else
    gicon = g_file_info_get_attribute_object (file_info, G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON);

  display_name = g_file_info_get_display_name (file_info);

  image = g_object_new (GTK_TYPE_IMAGE,
                        "gicon", gicon,
                        "visible", TRUE,
                        "margin-start", 6,
                        "margin-end", 3,
                        NULL);
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (image));

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", display_name,
                        "hexpand", TRUE,
                        "visible", TRUE,
                        "xalign", 0.0f,
                        NULL);
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (label));

  return row;
}

static GtkWidget *
create_regular_func (gpointer item,
                     gpointer user_data)
{
  return create_row_func (item, FALSE, user_data);
}

static GtkWidget *
create_header_func (gpointer item,
                     gpointer user_data)
{
  return create_row_func (item, TRUE, user_data);
}

static gboolean
hide_dot_files (EggDirectoryModel *model,
                GFile             *directory,
                GFileInfo         *file_info,
                gpointer           user_data)
{
  const gchar *name = g_file_info_get_display_name (file_info);
  return (name && *name != '.');
}

static void
header_activated (EggStackList  *stack_list,
                  GtkListBoxRow *row,
                  gpointer       user_data)
{
  GListModel *model;
  GFile *directory;

  directory = g_object_get_data (G_OBJECT (row), "G_FILE");
  model = egg_stack_list_get_model (stack_list);

  while (model != NULL)
    {
      GFile *current = egg_directory_model_get_directory (EGG_DIRECTORY_MODEL (model));

      if (g_file_equal (current, directory))
        break;

      if (egg_stack_list_get_depth (stack_list) == 1)
        break;

      egg_stack_list_pop (stack_list);

      model = egg_stack_list_get_model (stack_list);
    }
}

static void
row_activated (EggStackList  *stack_list,
               GtkListBoxRow *row,
               gpointer       user_data)
{
  GFileInfo *file_info;
  EggDirectoryModel *model;
  g_autoptr(GFile) child = NULL;
  GFile *directory;

  model = EGG_DIRECTORY_MODEL (egg_stack_list_get_model (stack_list));
  directory = egg_directory_model_get_directory (model);
  file_info = g_object_get_data (G_OBJECT (row), "G_FILE_INFO");

  if ((file_info == NULL) || (G_FILE_TYPE_DIRECTORY != g_file_info_get_file_type (file_info)))
    return;

  child = g_file_get_child (directory, g_file_info_get_name (file_info));
  model = EGG_DIRECTORY_MODEL (egg_directory_model_new (child));

  egg_stack_list_push (stack_list,
                       create_header_func (file_info, directory),
                       G_LIST_MODEL (model),
                       create_regular_func,
                       directory, NULL);

  g_object_unref (model);
}

gint
main (gint argc,
      gchar *argv[])
{
  GtkWindow *window;
  EggStackList *stack_list;
  GListModel *model;
  GFile *root;
  GFile *directory;
  GFileInfo *info;
  GtkCssProvider *provider;

  gtk_init (&argc, &argv);

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, TEST_CSS, -1, NULL);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  window = g_object_new (GTK_TYPE_WINDOW,
                         "default-width", 300,
                         "default-height", 700,
                         NULL);

  root = g_file_new_for_path (g_get_current_dir ());
  directory = g_file_get_parent (root);

  info = g_file_info_new ();
  g_file_info_set_name (info, g_file_get_basename (directory));
  g_file_info_set_display_name (info, g_file_get_basename (directory));
  g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);

  model = egg_directory_model_new (directory);
  egg_directory_model_set_visible_func (EGG_DIRECTORY_MODEL (model), hide_dot_files, NULL, NULL);

  stack_list = g_object_new (EGG_TYPE_STACK_LIST,
                             "visible", TRUE,
                             NULL);
  g_signal_connect (stack_list,
                    "header-activated",
                    G_CALLBACK (header_activated),
                    NULL);
  g_signal_connect (stack_list,
                    "row-activated",
                    G_CALLBACK (row_activated),
                    NULL);
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (stack_list));

  egg_stack_list_push (stack_list,
                       create_header_func (info, directory),
                       model,
                       create_regular_func,
                       directory,
                       NULL);

  g_signal_connect (window, "delete-event", gtk_main_quit, NULL);
  gtk_window_present (window);

  gtk_main ();

  return 0;
}
