/* gb-panel-addin.c
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

#define G_LOG_DOMAIN "gb-panel-addin"

#include <ide.h>

#include "gb-panel-addin.h"

G_DEFINE_INTERFACE (GbPanelAddin, gb_panel_addin, GTK_TYPE_WIDGET)

GbPanelPosition
gb_panel_addin_get_position (GbPanelAddin *self)
{
  g_return_val_if_fail (GB_IS_PANEL_ADDIN (self), GB_PANEL_LEFT);

  if (GB_PANEL_ADDIN_GET_IFACE (self)->get_position)
    return GB_PANEL_ADDIN_GET_IFACE (self)->get_position (self);

  return GB_PANEL_LEFT;
}

const gchar *
gb_panel_addin_get_icon_name (GbPanelAddin *self)
{
  g_return_val_if_fail (GB_IS_PANEL_ADDIN (self), NULL);

  if (GB_PANEL_ADDIN_GET_IFACE (self)->get_icon_name)
    return GB_PANEL_ADDIN_GET_IFACE (self)->get_icon_name (self);

  return NULL;
}

const gchar *
gb_panel_addin_get_title (GbPanelAddin *self)
{
  g_return_val_if_fail (GB_IS_PANEL_ADDIN (self), NULL);

  if (GB_PANEL_ADDIN_GET_IFACE (self)->get_title)
    return GB_PANEL_ADDIN_GET_IFACE (self)->get_title (self);

  return NULL;
}

void
gb_panel_addin_load (GbPanelAddin *self,
                     GbWorkbench  *workbench)
{
  g_return_if_fail (GB_IS_PANEL_ADDIN (self));
  g_return_if_fail (GB_IS_WORKBENCH (workbench));

  if (GB_PANEL_ADDIN_GET_IFACE (self)->load)
    GB_PANEL_ADDIN_GET_IFACE (self)->load (self, workbench);
}

void
gb_panel_addin_unload (GbPanelAddin *self,
                       GbWorkbench  *workbench)
{
  g_return_if_fail (GB_IS_PANEL_ADDIN (self));
  g_return_if_fail (GB_IS_WORKBENCH (workbench));

  if (GB_PANEL_ADDIN_GET_IFACE (self)->unload)
    GB_PANEL_ADDIN_GET_IFACE (self)->unload (self, workbench);
}

static void
gb_panel_addin_default_init (GbPanelAddinInterface *iface)
{
}
