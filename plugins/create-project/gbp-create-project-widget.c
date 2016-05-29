/* gbp-create-project-widget.c
 *
 * Copyright (C) 2016 Christian Hergert <christian@hergert.me>
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

#include <glib/gi18n.h>
#include <libpeas/peas.h>
#include <stdlib.h>

#include "ide-macros.h"
#include "gbp-create-project-template-icon.h"
#include "gbp-create-project-widget.h"

struct _GbpCreateProjectWidget
{
  GtkBin                parent;

  GtkEntry             *project_name_entry;
  GtkEntry             *project_location_entry;
  GtkFileChooserButton *project_location_button;
  GtkComboBoxText      *project_language_chooser;
  GtkFlowBox           *project_template_chooser;
  GtkFileChooserDialog *select_folder_dialog;
  GtkComboBoxText      *versioning_chooser;
  GtkComboBoxText      *license_chooser;

  guint                 auto_update : 1;
};

struct raunaq
{
  GTask                  *task;
  GHashTable             *params;
  IdeProjectTemplate     *template;
  GbpCreateProjectWidget *self;
};

enum {
  PROP_0,
  PROP_IS_READY,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

G_DEFINE_TYPE (GbpCreateProjectWidget, gbp_create_project_widget, GTK_TYPE_BIN)

static int
sort_by_name (gconstpointer a,
              gconstpointer b)
{
  const gchar * const *astr = a;
  const gchar * const *bstr = b;

  return g_utf8_collate (*astr, *bstr);
}

static void
gbp_create_project_widget_add_languages (GbpCreateProjectWidget *self,
                                         GList                  *project_templates)
{
  g_autoptr(GHashTable) languages = NULL;
  const GList *iter;
  const gchar **keys;
  guint len;
  guint i;

  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));

  languages = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  for (iter = project_templates; iter != NULL; iter = iter->next)
    {
      IdeProjectTemplate *template = iter->data;
      g_auto(GStrv) template_languages = NULL;

      g_assert (IDE_IS_PROJECT_TEMPLATE (template));

      template_languages = ide_project_template_get_languages (template);

      for (i = 0; template_languages [i]; i++)
        g_hash_table_add (languages, g_strdup (template_languages [i]));
    }

  keys = (const gchar **)g_hash_table_get_keys_as_array (languages, &len);
  qsort (keys, len, sizeof (gchar *), sort_by_name);
  for (i = 0; keys [i]; i++)
    gtk_combo_box_text_append (self->project_language_chooser, NULL, keys [i]);
  g_free (keys);
}

static gboolean
validate_name (const gchar *name)
{
  for (; *name; name = g_utf8_next_char (name))
    {
      gunichar ch = g_utf8_get_char (name);

      if (ch == '/')
        return FALSE;
    }

  return TRUE;
}

static void
gbp_create_project_widget_location_changed (GbpCreateProjectWidget *self,
                                            GtkEntry               *entry)
{
  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));
  g_assert (GTK_IS_ENTRY (entry));

  self->auto_update = FALSE;

  g_signal_handlers_disconnect_by_func (self->project_location_entry,
                                        gbp_create_project_widget_location_changed,
                                        self);
}

static void
gbp_create_project_widget_name_changed (GbpCreateProjectWidget *self,
                                        GtkEntry               *entry)
{
  const gchar *text;
  g_autofree gchar *project_name = NULL;
  g_autofree gchar *project_dir = NULL;

  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));
  g_assert (GTK_IS_ENTRY (entry));

  text = gtk_entry_get_text (entry);
  project_name = g_strstrip (g_strdup (text));

  if (ide_str_empty0 (project_name) || !validate_name (project_name))
    {
      g_object_set (self->project_name_entry,
                    "secondary-icon-name", "dialog-warning-symbolic",
                    NULL);

      project_dir = g_strdup ("");
    }
  else
    {
      g_object_set (self->project_name_entry,
                    "secondary-icon-name", NULL,
                    NULL);

      project_dir = g_ascii_strdown (g_strdelimit (project_name, " ", '-'), -1);
    }

  if (self->auto_update)
    {
      g_signal_handlers_block_by_func (self->project_location_entry,
                                       gbp_create_project_widget_location_changed,
                                       self);

      gtk_entry_set_text (self->project_location_entry, project_dir);

      g_signal_handlers_unblock_by_func (self->project_location_entry,
                                         gbp_create_project_widget_location_changed,
                                         self);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_IS_READY]);
}

static gboolean
gbp_create_project_widget_flow_box_filter (GtkFlowBoxChild *template_container,
                                           gpointer         object)
{
  GbpCreateProjectWidget *self = object;
  GbpCreateProjectTemplateIcon *template_icon;
  IdeProjectTemplate *template;
  g_autofree gchar *language = NULL;
  g_auto(GStrv) template_languages = NULL;
  gint i;

  g_assert (GTK_IS_FLOW_BOX_CHILD (template_container));
  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));

  language = gtk_combo_box_text_get_active_text (self->project_language_chooser);

  if (ide_str_empty0 (language))
    return TRUE;

  template_icon = GBP_CREATE_PROJECT_TEMPLATE_ICON (gtk_bin_get_child (GTK_BIN (template_container)));
  g_object_get (template_icon,
                "template", &template,
                NULL);
  template_languages = ide_project_template_get_languages (template);
  g_object_unref (template);

  for (i = 0; template_languages [i]; i++)
    {
      if (g_str_equal (language, template_languages [i]))
        return TRUE;
    }

  gtk_flow_box_unselect_child (self->project_template_chooser, template_container);

  return FALSE;
}

static void
gbp_create_project_widget_language_changed (GbpCreateProjectWidget *self,
                                            GtkComboBox            *language_chooser)
{
  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));

  gtk_flow_box_invalidate_filter (self->project_template_chooser);

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_IS_READY]);
}

static void
gbp_create_project_widget_template_selected (GbpCreateProjectWidget *self,
                                             GtkFlowBox             *box,
                                             GtkFlowBoxChild        *child)
{
  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_IS_READY]);
}

static void
gbp_create_project_widget_add_template_buttons (GbpCreateProjectWidget *self,
                                                GList                  *project_templates)
{
  const GList *iter;

  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));

  for (iter = project_templates; iter != NULL; iter = iter->next)
    {
      IdeProjectTemplate *template = iter->data;
      GbpCreateProjectTemplateIcon *template_icon;
      GtkFlowBoxChild *template_container;

      g_assert (IDE_IS_PROJECT_TEMPLATE (template));

      template_icon = g_object_new (GBP_TYPE_CREATE_PROJECT_TEMPLATE_ICON,
                                    "visible", TRUE,
                                    "template", template,
                                    NULL);

      template_container = g_object_new (GTK_TYPE_FLOW_BOX_CHILD,
                                         "visible", TRUE,
                                         NULL);
      gtk_container_add (GTK_CONTAINER (template_container), GTK_WIDGET (template_icon));
      gtk_flow_box_insert (self->project_template_chooser, GTK_WIDGET (template_container), -1);
    }
}

static void
template_providers_foreach_cb (PeasExtensionSet *set,
                               PeasPluginInfo   *plugin_info,
                               PeasExtension    *exten,
                               gpointer          user_data)
{
  GbpCreateProjectWidget *self = user_data;
  IdeTemplateProvider *provider = (IdeTemplateProvider *)exten;
  GList *templates;

  g_assert (PEAS_IS_EXTENSION_SET (set));
  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));
  g_assert (IDE_IS_TEMPLATE_PROVIDER (provider));

  templates = ide_template_provider_get_project_templates (provider);

  gbp_create_project_widget_add_template_buttons (self, templates);
  gbp_create_project_widget_add_languages (self, templates);

  g_list_free_full (templates, g_object_unref);
}

static void
vcs_initializers_foreach_cb (PeasExtensionSet *set,
                             PeasPluginInfo   *plugin_info,
                             PeasExtension    *exten,
                             gpointer          user_data)
{
  GbpCreateProjectWidget *self = user_data;
  IdeVcsInitializer *initializer = (IdeVcsInitializer *)exten;
  g_autofree gchar *title = NULL;
  g_autofree gchar *id = NULL;

  g_assert (PEAS_IS_EXTENSION_SET (set));
  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));
  g_assert (IDE_IS_VCS_INITIALIZER (initializer));

  title = ide_vcs_initializer_get_title (initializer);
  id = g_strdup (peas_plugin_info_get_module_name (plugin_info));

  gtk_combo_box_text_append (self->versioning_chooser, id, title);
}

static void
gbp_create_project_widget_constructed (GObject *object)
{
  GbpCreateProjectWidget *self = GBP_CREATE_PROJECT_WIDGET (object);
  PeasEngine *engine;
  PeasExtensionSet *extensions;

  engine = peas_engine_get_default ();

  /* Load templates */
  extensions = peas_extension_set_new (engine, IDE_TYPE_TEMPLATE_PROVIDER, NULL);
  peas_extension_set_foreach (extensions, template_providers_foreach_cb, self);
  g_clear_object (&extensions);

  /* Load version control backends */
  extensions = peas_extension_set_new (engine, IDE_TYPE_VCS_INITIALIZER, NULL);
  peas_extension_set_foreach (extensions, vcs_initializers_foreach_cb, self);
  g_clear_object (&extensions);
  gtk_combo_box_text_append (self->versioning_chooser, NULL, _("Without version control"));

  G_OBJECT_CLASS (gbp_create_project_widget_parent_class)->constructed (object);

  gtk_combo_box_set_active (GTK_COMBO_BOX (self->project_language_chooser), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX (self->versioning_chooser), 0);
  gtk_combo_box_set_active_id (GTK_COMBO_BOX (self->license_chooser), "gpl_3");

  self->auto_update = TRUE;
}

static void
gbp_create_project_widget_finalize (GObject *object)
{
  G_OBJECT_CLASS (gbp_create_project_widget_parent_class)->finalize (object);
}

static gboolean
gbp_create_project_widget_is_ready (GbpCreateProjectWidget *self)
{
  const gchar *text;
  g_autofree gchar *project_name = NULL;
  g_autofree gchar *language = NULL;
  GList *selected_template = NULL;

  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));

  text = gtk_entry_get_text (self->project_name_entry);
  project_name = g_strstrip (g_strdup (text));

  if (ide_str_empty0 (project_name) || !validate_name (project_name))
    return FALSE;

  language = gtk_combo_box_text_get_active_text (self->project_language_chooser);

  if (ide_str_empty0 (language))
    return FALSE;

  selected_template = gtk_flow_box_get_selected_children (self->project_template_chooser);

  if (selected_template == NULL)
    return FALSE;

  g_list_free (selected_template);

  return TRUE;
}

static void
make_dialog_modal (GbpCreateProjectWidget *self,
                   GtkFileChooserDialog   *dialog)
{
  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));
  g_assert (GTK_IS_FILE_CHOOSER_DIALOG (dialog));

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
}

static void
gbp_create_project_widget_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GbpCreateProjectWidget *self = GBP_CREATE_PROJECT_WIDGET(object);

  switch (prop_id)
    {
    case PROP_IS_READY:
      g_value_set_boolean (value, gbp_create_project_widget_is_ready (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
gbp_create_project_widget_class_init (GbpCreateProjectWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = gbp_create_project_widget_constructed;
  object_class->finalize = gbp_create_project_widget_finalize;
  object_class->get_property = gbp_create_project_widget_get_property;

  properties [PROP_IS_READY] =
    g_param_spec_boolean ("is-ready",
                          "Is Ready",
                          "Is Ready",
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_css_name (widget_class, "createprojectwidget");
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/builder/plugins/create-project-plugin/gbp-create-project-widget.ui");
  gtk_widget_class_bind_template_child (widget_class, GbpCreateProjectWidget, project_name_entry);
  gtk_widget_class_bind_template_child (widget_class, GbpCreateProjectWidget, project_location_button);
  gtk_widget_class_bind_template_child (widget_class, GbpCreateProjectWidget, project_location_entry);
  gtk_widget_class_bind_template_child (widget_class, GbpCreateProjectWidget, project_language_chooser);
  gtk_widget_class_bind_template_child (widget_class, GbpCreateProjectWidget, project_template_chooser);
  gtk_widget_class_bind_template_child (widget_class, GbpCreateProjectWidget, select_folder_dialog);
  gtk_widget_class_bind_template_child (widget_class, GbpCreateProjectWidget, versioning_chooser);
  gtk_widget_class_bind_template_child (widget_class, GbpCreateProjectWidget, license_chooser);
}

static void
gbp_create_project_widget_init (GbpCreateProjectWidget *self)
{
  g_autoptr(GSettings) settings = NULL;
  g_autofree gchar *path = NULL;
  g_autofree char *projects_dir = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  settings = g_settings_new ("org.gnome.builder");
  path = g_settings_get_string (settings, "projects-directory");

  if (!ide_str_empty0 (path))
    {
      if (!g_path_is_absolute (path))
        projects_dir = g_build_filename (g_get_home_dir (), path, NULL);
      else
        projects_dir = g_steal_pointer (&path);

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (self->project_location_button),
                                           projects_dir);
    }

  gtk_flow_box_set_filter_func (self->project_template_chooser,
                                gbp_create_project_widget_flow_box_filter,
                                self,
                                NULL);

  g_signal_connect_object (self->project_name_entry,
                           "changed",
                           G_CALLBACK (gbp_create_project_widget_name_changed),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->project_location_entry,
                           "changed",
                           G_CALLBACK (gbp_create_project_widget_location_changed),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->project_language_chooser,
                           "changed",
                           G_CALLBACK (gbp_create_project_widget_language_changed),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->project_template_chooser,
                           "child-activated",
                           G_CALLBACK (gbp_create_project_widget_template_selected),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->select_folder_dialog,
                           "show",
                           G_CALLBACK (make_dialog_modal),
                           self,
                           G_CONNECT_SWAPPED);

  gtk_dialog_add_buttons (GTK_DIALOG (self->select_folder_dialog),
                          _("Select"), GTK_RESPONSE_OK,
                          _("Cancel"), GTK_RESPONSE_CANCEL,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (self->select_folder_dialog), GTK_RESPONSE_OK);
}

static void
init_vcs_cb (GObject      *object,
             GAsyncResult *result,
             gpointer      user_data)
{
  g_autoptr(GTask) task = user_data;
  IdeVcsInitializer *vcs = (IdeVcsInitializer *)object;
  GbpCreateProjectWidget *self;
  IdeWorkbench *workbench;
  GFile *project_file;
  GError *error = NULL;

  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (G_IS_TASK (task));

  if (!ide_vcs_initializer_initialize_finish (vcs, result, &error))
    {
      g_task_return_error (task, error);
      return;
    }

  self = g_task_get_source_object (task);
  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));

  project_file = g_task_get_task_data (task);
  g_assert (G_IS_FILE (project_file));

  workbench = ide_widget_get_workbench (GTK_WIDGET (self));
  ide_workbench_open_project_async (workbench, project_file, NULL, NULL, NULL);

  g_task_return_boolean (task, TRUE);
}

static void
extract_cb (GObject      *object,
            GAsyncResult *result,
            gpointer      user_data)
{
  IdeProjectTemplate *template = (IdeProjectTemplate *)object;
  g_autoptr(GTask) task = user_data;
  g_autoptr(IdeVcsInitializer) vcs = NULL;
  GbpCreateProjectWidget *self;
  IdeWorkbench *workbench;
  PeasEngine *engine;
  PeasPluginInfo *plugin_info;
  GFile *project_file;
  GError *error = NULL;
  const gchar *vcs_id;

  g_assert (IDE_IS_PROJECT_TEMPLATE (template));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (G_IS_TASK (task));

  if (!ide_project_template_expand_finish (template, result, &error))
    {
      g_task_return_error (task, error);
      return;
    }

  self = g_task_get_source_object (task);
  g_assert (GBP_IS_CREATE_PROJECT_WIDGET (self));

  project_file = g_task_get_task_data (task);
  g_assert (G_IS_FILE (project_file));

  vcs_id = gtk_combo_box_get_active_id (GTK_COMBO_BOX (self->versioning_chooser));

  if (vcs_id == NULL)
    {
      workbench = ide_widget_get_workbench (GTK_WIDGET (self));
      ide_workbench_open_project_async (workbench, project_file, NULL, NULL, NULL);
      g_task_return_boolean (task, TRUE);
      return;
    }

  engine = peas_engine_get_default ();
  plugin_info = peas_engine_get_plugin_info (engine, vcs_id);
  if (plugin_info == NULL)
    goto failure;

  vcs = (IdeVcsInitializer *)peas_engine_create_extension (engine, plugin_info,
                                                           IDE_TYPE_VCS_INITIALIZER,
                                                           NULL);
  if (vcs == NULL)
    goto failure;

  ide_vcs_initializer_initialize_async (vcs,
                                        project_file,
                                        g_task_get_cancellable (task),
                                        init_vcs_cb,
                                        g_object_ref (task));

  return;

failure:
  g_task_return_new_error (task,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           _("A failure occurred while initializing version control"));
}

void
gbp_create_project_widget_on_response (GtkDialog     *dialog,
                                       gint           response,
                                       gpointer  user_data)
{
  struct raunaq *r;
  r = g_new0 (struct raunaq, 1);
  r = user_data;

  if (response == GTK_RESPONSE_OK)
    {
      ide_project_template_expand_async (r->template,
                                         r->params,
                                         NULL,
                                         extract_cb,
                                         g_object_ref (r->task));

      gtk_widget_destroy (dialog);
    }
  else
    {
      gtk_widget_grab_focus (r->self->project_location_entry);
      goto failure;
    }


  return;

failure:
  g_task_return_new_error (r->task,
                           G_IO_ERROR,
                           G_IO_ERROR_EXISTS,
                           _("The directory exists and could not be replaced"));
  gtk_widget_destroy (dialog);
}

void
gbp_create_project_widget_create_async (GbpCreateProjectWidget *self,
                                        GCancellable           *cancellable,
                                        GAsyncReadyCallback     callback,
                                        gpointer                user_data)
{
  struct raunaq *r;
  r = g_new0 (struct raunaq, 1);
  r->task = NULL;
  r->params = NULL;
  r->template = NULL;
  r->self = self;

  g_autofree gchar *name = NULL;
  g_autofree gchar *location = NULL;
  g_autofree gchar *path = NULL;
  g_autofree gchar *language = NULL;
  GtkFlowBoxChild *template_container;
  GbpCreateProjectTemplateIcon *template_icon;
  const gchar *text;
  const gchar *child_name;
  const gchar *license_id;
  GList *selected_box_child;

  g_return_if_fail (GBP_CREATE_PROJECT_WIDGET (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  selected_box_child = gtk_flow_box_get_selected_children (self->project_template_chooser);
  template_container = selected_box_child->data;
  template_icon = GBP_CREATE_PROJECT_TEMPLATE_ICON (gtk_bin_get_child (GTK_BIN (template_container)));
  g_object_get (template_icon,
                "template", &r->template,
                NULL);
  g_list_free (selected_box_child);

  r->params = g_hash_table_new_full (g_str_hash,
                                  g_str_equal,
                                  g_free,
                                  (GDestroyNotify)g_variant_unref);

  text = gtk_entry_get_text (self->project_name_entry);
  name = g_strstrip (g_strdup (text));
  g_hash_table_insert (r->params,
                       g_strdup ("name"),
                       g_variant_ref_sink (g_variant_new_string (g_strdelimit (name, " ", '-'))));

  child_name = gtk_entry_get_text (self->project_location_entry);
  location = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (self->project_location_button));

  if (!ide_str_empty0 (child_name))
    path = g_build_filename (location, child_name, NULL);
  else
    path = g_steal_pointer (&location);

  g_hash_table_insert (r->params,
                       g_strdup ("path"),
                       g_variant_ref_sink (g_variant_new_string (path)));

  language = gtk_combo_box_text_get_active_text (self->project_language_chooser);
  g_hash_table_insert (r->params,
                       g_strdup ("language"),
                       g_variant_ref_sink (g_variant_new_string (language)));

  license_id = gtk_combo_box_get_active_id (GTK_COMBO_BOX (self->license_chooser));

  if (!g_str_equal (license_id, "none"))
    {
      g_autofree gchar *license_full_path = NULL;
      g_autofree gchar *license_short_path = NULL;

      license_full_path = g_strjoin (NULL, "resource://", "/org/gnome/builder/plugins/create-project-plugin/license/full/", license_id, NULL);
      license_short_path = g_strjoin (NULL, "resource://", "/org/gnome/builder/plugins/create-project-plugin/license/short/", license_id, NULL);

      g_hash_table_insert (r->params,
                           g_strdup ("license_full"),
                           g_variant_ref_sink (g_variant_new_string (license_full_path)));

      g_hash_table_insert (r->params,
                           g_strdup ("license_short"),
                           g_variant_ref_sink (g_variant_new_string (license_short_path)));
    }

  r->task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (r->task, g_file_new_for_path (path), g_object_unref);

  if (g_file_test (path, G_FILE_TEST_IS_DIR))
    {
      GtkWidget *dialog;
      GtkWidget *window;

      name = g_path_get_basename (path);

      dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_QUESTION,
                                       GTK_BUTTONS_NONE,
                                       _("The directory “%s” already exists. Would you like to overwrite it?"),
                                       name);

      gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                              _("Cancel"), GTK_RESPONSE_CANCEL,
                              _("Overwrite directory"), GTK_RESPONSE_OK,
                              NULL);

      gtk_window_set_transient_for (GTK_WINDOW (dialog), gtk_widget_get_toplevel (self));

      gtk_window_present (dialog);

      g_signal_connect (GTK_DIALOG (dialog),
                        "response",
                        G_CALLBACK (gbp_create_project_widget_on_response),
                        r);
    }
  else
    {
      ide_project_template_expand_async (r->template,
                                         r->params,
                                         NULL,
                                         extract_cb,
                                         g_object_ref (r->task));
    }
}

gboolean
gbp_create_project_widget_create_finish (GbpCreateProjectWidget *self,
                                         GAsyncResult           *result,
                                         GError                **error)
{
  g_return_val_if_fail (GBP_IS_CREATE_PROJECT_WIDGET (self), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}
