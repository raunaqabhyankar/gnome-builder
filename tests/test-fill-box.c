/* test-fill-box.c
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

#include <stdlib.h>

#include "egg-fill-box.h"

static void
new_pixbuf_cb (GObject      *object,
               GAsyncResult *result,
               gpointer      user_data)
{
  GdkPixbuf *pixbuf = NULL;
  EggFillBox *fill_box = user_data;
  GtkWidget *image;
  GError *error = NULL;

  pixbuf = gdk_pixbuf_new_from_stream_finish (result, &error);

  if (pixbuf == NULL)
    {
      g_printerr ("%s\n", error->message);
      g_clear_error (&error);
      return;
    }

  image = g_object_new (GTK_TYPE_IMAGE,
                        "pixbuf", pixbuf,
                        "visible", TRUE,
                        NULL);
  gtk_container_add (GTK_CONTAINER (fill_box), image);

  g_clear_object (&pixbuf);
}

static void
load_directory (EggFillBox  *fill_box,
                const gchar *directory)
{
  const gchar *name;
  GError *error = NULL;
  GDir *dir;

  dir = g_dir_open (directory, 0, &error);

  if (dir == NULL)
    {
      g_printerr ("%s\n", error->message);
      g_clear_error (&error);
      exit (1);
    }

  while ((name = g_dir_read_name (dir)))
    {
      g_autofree gchar *filename = NULL;
      g_autoptr(GFile) file = NULL;
      g_autoptr(GFileInputStream) stream = NULL;

      if (!(g_str_has_suffix (name, ".jpg") ||
            g_str_has_suffix (name, ".JPG") ||
            g_str_has_suffix (name, ".png")))
        continue;

      filename = g_build_filename (directory, name, NULL);
      file = g_file_new_for_path (filename);
      stream = g_file_read (file, NULL, NULL);

      if (stream == NULL)
        continue;

      gdk_pixbuf_new_from_stream_at_scale_async (G_INPUT_STREAM (stream),
                                                 150,
                                                 -1,
                                                 TRUE,
                                                 NULL,
                                                 new_pixbuf_cb,
                                                 fill_box);
    }

  g_dir_close (dir);
}

gint
main (gint   argc,
      gchar *argv[])
{
  GtkWindow *window;
  GtkScrolledWindow *scroller;
  EggFillBox *fill_box;
  GtkSettings *settings;
  const gchar *path;

  gtk_init (&argc, &argv);

  if (argc < 2)
    path = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  else
    path = argv [1];

  settings = gtk_settings_get_default ();
  g_object_set (settings, "gtk-application-prefer-dark-theme", TRUE, NULL);

  window = g_object_new (GTK_TYPE_WINDOW, NULL);
  scroller = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                           "visible", TRUE,
                           NULL);
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (scroller));
  fill_box = g_object_new (EGG_TYPE_FILL_BOX,
                           "visible", TRUE,
                           NULL);
  gtk_container_add (GTK_CONTAINER (scroller), GTK_WIDGET (fill_box));
  load_directory (fill_box, path);
  g_signal_connect (window, "delete-event", gtk_main_quit, NULL);
  gtk_window_present (window);
  gtk_main ();

  return 0;
}
