/* egg-fill-box.c
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

#include "egg-fill-box.h"

typedef struct
{
  GtkWidget      *widget;
  gint            min_width;
  gint            nat_width;
  gint            min_height;
  gint            nat_height;
  GtkAllocation   alloc;
} EggFillBoxChild;

typedef struct
{
  GSequence *children;
  gint       min_columns;
  gint       max_columns;
  gint       height;
} EggFillBoxPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EggFillBox, egg_fill_box, GTK_TYPE_CONTAINER)

enum {
  PROP_0,
  PROP_MIN_COLUMNS,
  PROP_MAX_COLUMNS,
  LAST_PROP
};

static GParamSpec *gParamSpecs [LAST_PROP];

static gint
egg_fill_box_child_compare (gconstpointer a,
                            gconstpointer b,
                            gpointer      data)
{
  const EggFillBoxChild *ac = a;
  const EggFillBoxChild *bc = b;

  return MAX (bc->min_height, bc->nat_height) - MAX (ac->min_height, ac->nat_height);
}

static void
egg_fill_box_child_free (gpointer data)
{
  EggFillBoxChild *child = data;

  if (child != NULL)
    g_slice_free (EggFillBoxChild, child);
}

static void
egg_fill_box_relayout (EggFillBox *self,
                       gint        width)
{
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);
  GSequenceIter *iter;
  g_autofree gint *columns = NULL;
  GtkAllocation allocation;
  gint num_columns;
  gint max_min_width = -1;
  gint max_nat_width = -1;
  gint begin_space = 0;
  gint column_width;
  gint consumed;
  gint column = 0;
  gint i;

  g_assert (EGG_IS_FILL_BOX (self));

  gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);

  if (width <= 0)
    width = allocation.width;

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFillBoxChild *child = g_sequence_get (iter);

      gtk_widget_get_preferred_width (child->widget, &child->min_width, &child->nat_width);
      gtk_widget_get_preferred_height (child->widget, &child->min_height, &child->nat_height);

      if (child->min_width > max_min_width)
        max_min_width = child->min_width;

      if (child->nat_width > max_nat_width)
        max_nat_width = child->nat_width;
    }

  g_sequence_sort (priv->children, egg_fill_box_child_compare, NULL);

  max_min_width = MAX (1, max_min_width);
  max_nat_width = MAX (1, max_nat_width);

  if (max_nat_width < width)
    {
      num_columns = MAX (1, width / max_nat_width);
      consumed = num_columns * max_nat_width;
      column_width = max_nat_width;
      begin_space = (width - consumed) / 2;
    }
  else
    {
      num_columns = 1;
      consumed = width;
      column_width = width;
    }

  columns = g_new0 (gint, num_columns);

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFillBoxChild *child = g_sequence_get (iter);
      gint min_height = G_MAXINT;

      for (i = 0; i < num_columns; i++)
        {
          if (columns [i] < min_height)
            {
              min_height = columns [i];
              column = i;
            }
        }

      child->alloc.x = begin_space + (column_width * column);
      child->alloc.y = columns [column];
      child->alloc.width = column_width;

      gtk_widget_get_preferred_height_for_width (child->widget,
                                                 column_width,
                                                 NULL,
                                                 &child->alloc.height);

      columns [column] += child->alloc.height;
    }

  priv->height = 0;

  for (i = 0; i < num_columns; i++)
    {
      if ((priv->height == 0) || (columns [i] > priv->height))
        priv->height = columns [i];
    }
}

static void
egg_fill_box_add (GtkContainer *container,
                  GtkWidget    *widget)
{
  EggFillBox *self = (EggFillBox *)container;
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);
  EggFillBoxChild *child;

  g_assert (EGG_IS_FILL_BOX (self));
  g_assert (GTK_IS_WIDGET (widget));

  child = g_slice_new0 (EggFillBoxChild);
  child->widget = widget;
  gtk_widget_get_preferred_width (widget, &child->min_width, &child->nat_width);
  g_sequence_insert_sorted (priv->children, child, egg_fill_box_child_compare, NULL);
  gtk_widget_set_parent (widget, GTK_WIDGET (self));
  gtk_widget_queue_resize (GTK_WIDGET (container));
}

static void
egg_fill_box_remove (GtkContainer *container,
                     GtkWidget    *widget)
{
  EggFillBox *self = (EggFillBox *)container;
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);
  GSequenceIter *iter;

  g_assert (EGG_IS_FILL_BOX (self));
  g_assert (GTK_IS_WIDGET (widget));

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFillBoxChild *child = g_sequence_get (iter);

      if (child->widget == widget)
        {
          g_sequence_remove (iter);
          egg_fill_box_relayout (self, -1);
          break;
        }
    }
}

static void
egg_fill_box_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
  EggFillBox *self = (EggFillBox *)widget;
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);
  GSequenceIter *iter;

  g_assert (EGG_IS_FILL_BOX (self));

  GTK_WIDGET_CLASS (egg_fill_box_parent_class)->size_allocate (widget, allocation);

  egg_fill_box_relayout (self, -1);

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFillBoxChild *child = g_sequence_get (iter);

      gtk_widget_size_allocate (child->widget, &child->alloc);
    }
}

static GtkSizeRequestMode
egg_fill_box_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
egg_fill_box_get_preferred_width (GtkWidget *widget,
                                  gint      *min_width,
                                  gint      *nat_width)
{
  EggFillBox *self = (EggFillBox *)widget;
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);
  GSequenceIter *iter;
  gint real_min_width = 0;
  gint real_nat_width = 0;

  g_assert (EGG_IS_FILL_BOX (self));

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFillBoxChild *child = g_sequence_get (iter);
      gint child_min_width;
      gint child_nat_width;

      gtk_widget_get_preferred_width (child->widget, &child_min_width, &child_nat_width);

      if (child_min_width > real_min_width)
        real_min_width = child_min_width;

      if (child_nat_width > real_nat_width)
        real_nat_width = child_nat_width;
    }

  if (min_width)
    *min_width = real_min_width;

  if (nat_width)
    *nat_width = real_nat_width;
}

static void
egg_fill_box_get_preferred_height_for_width (GtkWidget *widget,
                                             gint       width,
                                             gint      *min_height,
                                             gint      *nat_height)
{
  EggFillBox *self = (EggFillBox *)widget;
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);

  g_assert (EGG_IS_FILL_BOX (self));

  egg_fill_box_relayout (self, width);

  if (min_height)
    *min_height = priv->height;

  if (nat_height)
    *nat_height = priv->height;
}

static void
egg_fill_box_realize (GtkWidget *widget)
{
  EggFillBox *self = (EggFillBox *)widget;

  g_assert (EGG_IS_FILL_BOX (self));

  GTK_WIDGET_CLASS (egg_fill_box_parent_class)->realize (widget);

  egg_fill_box_relayout (self, -1);
}

static void
egg_fill_box_forall (GtkContainer *container,
                     gboolean      include_internals,
                     GtkCallback   callback,
                     gpointer      callback_data)
{
  EggFillBox *self = (EggFillBox *)container;
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);
  GSequenceIter *iter;

  g_assert (EGG_IS_FILL_BOX (self));

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFillBoxChild *child = g_sequence_get (iter);

      callback (child->widget, callback_data);
    }
}

static void
egg_fill_box_finalize (GObject *object)
{
  EggFillBox *self = (EggFillBox *)object;
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);

  g_clear_pointer (&priv->children, g_sequence_free);

  G_OBJECT_CLASS (egg_fill_box_parent_class)->finalize (object);
}

static void
egg_fill_box_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  EggFillBox *self = EGG_FILL_BOX (object);

  switch (prop_id)
    {
    case PROP_MAX_COLUMNS:
      g_value_set_int (value, egg_fill_box_get_max_columns (self));
      break;

    case PROP_MIN_COLUMNS:
      g_value_set_int (value, egg_fill_box_get_min_columns (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
egg_fill_box_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  EggFillBox *self = EGG_FILL_BOX (object);

  switch (prop_id)
    {
    case PROP_MAX_COLUMNS:
      egg_fill_box_set_max_columns (self, g_value_get_int (value));
      break;

    case PROP_MIN_COLUMNS:
      egg_fill_box_set_min_columns (self, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
egg_fill_box_class_init (EggFillBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->finalize = egg_fill_box_finalize;
  object_class->get_property = egg_fill_box_get_property;
  object_class->set_property = egg_fill_box_set_property;

  widget_class->get_preferred_width = egg_fill_box_get_preferred_width;
  widget_class->get_request_mode = egg_fill_box_get_request_mode;
  widget_class->get_preferred_height_for_width = egg_fill_box_get_preferred_height_for_width;
  widget_class->realize = egg_fill_box_realize;
  widget_class->size_allocate = egg_fill_box_size_allocate;

  container_class->add = egg_fill_box_add;
  container_class->forall = egg_fill_box_forall;
  container_class->remove = egg_fill_box_remove;
}

static void
egg_fill_box_init (EggFillBox *self)
{
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);

  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);

  priv->children = g_sequence_new (egg_fill_box_child_free);
  priv->min_columns = -1;
  priv->max_columns = -1;
}

GtkWidget *
egg_fill_box_new (void)
{
  return g_object_new (EGG_TYPE_FILL_BOX, NULL);
}

gint
egg_fill_box_get_max_columns (EggFillBox *self)
{
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);

  g_return_val_if_fail (EGG_IS_FILL_BOX (self), -1);

  return priv->max_columns;
}

void
egg_fill_box_set_max_columns (EggFillBox *self,
                              gint        max_columns)
{
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);

  g_return_if_fail (EGG_IS_FILL_BOX (self));
  g_return_if_fail (max_columns >= -1);

  if (max_columns <= 0)
    max_columns = -1;

  if (max_columns != priv->max_columns)
    {
      priv->max_columns = max_columns;
      gtk_widget_queue_resize (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), gParamSpecs [PROP_MAX_COLUMNS]);
    }
}

gint
egg_fill_box_get_min_columns (EggFillBox *self)
{
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);

  g_return_val_if_fail (EGG_IS_FILL_BOX (self), -1);

  return priv->min_columns;
}

void
egg_fill_box_set_min_columns (EggFillBox *self,
                              gint        min_columns)
{
  EggFillBoxPrivate *priv = egg_fill_box_get_instance_private (self);

  g_return_if_fail (EGG_IS_FILL_BOX (self));
  g_return_if_fail (min_columns >= -1);

  if (min_columns <= 0)
    min_columns = -1;

  if (min_columns != priv->min_columns)
    {
      priv->min_columns = min_columns;
      gtk_widget_queue_resize (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), gParamSpecs [PROP_MIN_COLUMNS]);
    }
}
