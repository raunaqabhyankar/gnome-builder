/* egg-directory-model.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EGG_DIRECTORY_MODEL_H
#define EGG_DIRECTORY_MODEL_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define EGG_TYPE_DIRECTORY_MODEL (egg_directory_model_get_type())

G_DECLARE_FINAL_TYPE (EggDirectoryModel, egg_directory_model, EGG, DIRECTORY_MODEL, GObject)

typedef gboolean (*EggDirectoryModelVisibleFunc) (EggDirectoryModel *self,
                                                  GFile             *directory,
                                                  GFileInfo         *file_info,
                                                  gpointer           user_data);

GListModel *egg_directory_model_new              (GFile                        *directory);
GFile      *egg_directory_model_get_directory    (EggDirectoryModel            *self);
void        egg_directory_model_set_directory    (EggDirectoryModel            *self,
                                                  GFile                        *directory);
void        egg_directory_model_set_visible_func (EggDirectoryModel            *self,
                                                  EggDirectoryModelVisibleFunc  visible_func,
                                                  gpointer                      user_data,
                                                  GDestroyNotify                user_data_free_func);

G_END_DECLS

#endif /* EGG_DIRECTORY_MODEL_H */
