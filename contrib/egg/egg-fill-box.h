/* egg-fill-box.h
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

#ifndef EGG_FILL_BOX_H
#define EGG_FILL_BOX_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * SECTION:egg-fill-box:
 * @title: EggFillBox
 * @short_description: a box that fills content based on child allocation size
 *
 * #EggFillBox is a container type that will allocate children in a series of
 * columns. You can specify the number of columns, or allow the container to
 * determine this based on the largest allocation width of the children.
 *
 * The #EggFillBox will layout children starting from the tallest to the
 * shortest. Each item will be placed into the column with the shortest
 * combined height. This gives the overall appearance that all the columns
 * are of similar height.
 */

#define EGG_TYPE_FILL_BOX (egg_fill_box_get_type())

G_DECLARE_DERIVABLE_TYPE (EggFillBox, egg_fill_box, EGG, FILL_BOX, GtkContainer)

struct _EggFillBoxClass
{
  GtkContainerClass parent;
};

GtkWidget *egg_fill_box_new             (void);
gint       egg_fill_box_get_max_columns (EggFillBox *self);
void       egg_fill_box_set_max_columns (EggFillBox *self,
                                         gint        max_columns);
gint       egg_fill_box_get_min_columns (EggFillBox *self);
void       egg_fill_box_set_min_columns (EggFillBox *self,
                                         gint        min_columns);

G_END_DECLS

#endif /* EGG_FILL_BOX_H */
