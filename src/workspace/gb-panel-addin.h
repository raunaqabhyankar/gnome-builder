/* gb-panel-addin.h
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

#ifndef GB_PANEL_ADDIN_H
#define GB_PANEL_ADDIN_H

#include "gb-enums.h"
#include "gb-workbench.h"

G_BEGIN_DECLS

#define GB_TYPE_PANEL_ADDIN (gb_panel_addin_get_type())

G_DECLARE_INTERFACE (GbPanelAddin, gb_panel_addin, GB, PANEL_ADDIN, GtkWidget)

typedef enum
{
  GB_PANEL_LEFT,
  GB_PANEL_RIGHT,
  GB_PANEL_BOTTOM,
} GbPanelPosition;

struct _GbPanelAddinInterface
{
  GTypeInterface parent_instance;

  const gchar     *(*get_icon_name) (GbPanelAddin *self);
  const gchar     *(*get_title)     (GbPanelAddin *self);
  GbPanelPosition  (*get_position)  (GbPanelAddin *self);
  void             (*load)          (GbPanelAddin *self,
                                     GbWorkbench  *workbench);
  void             (*unload)        (GbPanelAddin *self,
                                     GbWorkbench  *workbench);
};

const gchar     *gb_panel_addin_get_title     (GbPanelAddin *self);
const gchar     *gb_panel_addin_get_icon_name (GbPanelAddin *self);
GbPanelPosition  gb_panel_addin_get_position  (GbPanelAddin *self);
void             gb_panel_addin_load          (GbPanelAddin *self,
                                               GbWorkbench  *workbench);
void             gb_panel_addin_unload        (GbPanelAddin *self,
                                               GbWorkbench  *workbench);

G_END_DECLS

#endif /* GB_PANEL_ADDIN_H */
