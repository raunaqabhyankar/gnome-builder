/* egg-stack-list.h
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

#ifndef EGG_STACK_LIST_H
#define EGG_STACK_LIST_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EGG_TYPE_STACK_LIST (egg_stack_list_get_type())

G_DECLARE_DERIVABLE_TYPE (EggStackList, egg_stack_list, EGG, STACK_LIST, GtkBin)

struct _EggStackListClass
{
  GtkBinClass parent_instance;

  void (*row_activated)    (EggStackList  *self,
                            GtkListBoxRow *row);
  void (*header_activated) (EggStackList  *self,
                            GtkListBoxRow *row);
};

typedef GtkWidget *(*EggStackListCreateWidgetFunc) (gpointer item,
                                                    gpointer user_data);

GtkWidget *egg_stack_list_new        (void);
void       egg_stack_list_push       (EggStackList                 *self,
                                      GtkWidget                    *header,
                                      GListModel                   *model,
                                      EggStackListCreateWidgetFunc  create_widget_func,
                                      gpointer                      user_data,
                                      GDestroyNotify                user_data_free_func);
void        egg_stack_list_pop       (EggStackList                 *self);
GListModel *egg_stack_list_get_model (EggStackList                 *self);
guint       egg_stack_list_get_depth (EggStackList                 *self);

G_END_DECLS

#endif /* EGG_STACK_LIST_H */
