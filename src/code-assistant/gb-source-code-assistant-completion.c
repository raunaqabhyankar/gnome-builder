/* gb-source-code-assistant-completion.c
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

#include <glib/gi18n.h>

#include "gb-source-code-assistant-completion.h"
#include "gca-structs.h"

struct _GbSourceCodeAssistantCompletionPrivate
{
  GCancellable     *cancellable;
  GbEditorDocument *document;
};

static void init_provider (GtkSourceCompletionProviderIface *);

G_DEFINE_TYPE_EXTENDED (GbSourceCodeAssistantCompletion,
                        gb_source_code_assistant_completion,
                        G_TYPE_OBJECT,
                        0,
                        G_ADD_PRIVATE (GbSourceCodeAssistantCompletion)
                        G_IMPLEMENT_INTERFACE (GTK_SOURCE_TYPE_COMPLETION_PROVIDER,
                                               init_provider))

enum {
  PROP_0,
  PROP_DOCUMENT,
  LAST_PROP
};

static GParamSpec *gParamSpecs [LAST_PROP];

GtkSourceCompletionProvider *
gb_source_code_assistant_completion_new (void)
{
  return g_object_new (GB_TYPE_SOURCE_CODE_ASSISTANT_COMPLETION, NULL);
}

void
gb_source_code_assistant_completion_set_document (GbSourceCodeAssistantCompletion *completion,
                                                  GbEditorDocument                *document)
{
  g_return_if_fail (GB_IS_SOURCE_CODE_ASSISTANT_COMPLETION (completion));
  g_return_if_fail (!document || GB_IS_EDITOR_DOCUMENT (document));

  if (document != completion->priv->document)
    {
      g_clear_object (&completion->priv->document);
      if (document)
        completion->priv->document = g_object_ref (document);
    }
}

static void
gb_source_code_assistant_completion_finalize (GObject *object)
{
  GbSourceCodeAssistantCompletion *self = (GbSourceCodeAssistantCompletion *)object;

  g_clear_object (&self->priv->document);

  G_OBJECT_CLASS (gb_source_code_assistant_completion_parent_class)->finalize (object);
}

static void
gb_source_code_assistant_completion_get_property (GObject    *object,
                                                  guint       prop_id,
                                                  GValue     *value,
                                                  GParamSpec *pspec)
{
  GbSourceCodeAssistantCompletion *self = GB_SOURCE_CODE_ASSISTANT_COMPLETION (object);

  switch (prop_id)
    {
    case PROP_DOCUMENT:
      g_value_set_object (value, self->priv->document);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gb_source_code_assistant_completion_set_property (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec)
{
  GbSourceCodeAssistantCompletion *self = GB_SOURCE_CODE_ASSISTANT_COMPLETION (object);

  switch (prop_id)
    {
    case PROP_DOCUMENT:
      gb_source_code_assistant_completion_set_document (self,
                                                        g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gb_source_code_assistant_completion_class_init (GbSourceCodeAssistantCompletionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gb_source_code_assistant_completion_finalize;
  object_class->get_property = gb_source_code_assistant_completion_get_property;
  object_class->set_property = gb_source_code_assistant_completion_set_property;

  gParamSpecs [PROP_DOCUMENT] =
    g_param_spec_object ("document",
                         _("Document"),
                         _("The document to provide completion for."),
                         GB_TYPE_EDITOR_DOCUMENT,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (object_class, PROP_DOCUMENT,
                                   gParamSpecs [PROP_DOCUMENT]);
}

static void
gb_source_code_assistant_completion_init (GbSourceCodeAssistantCompletion *self)
{
  self->priv = gb_source_code_assistant_completion_get_instance_private (self);
}

static GdkPixbuf *
get_icon (GtkSourceCompletionProvider *provider)
{
  return NULL;
}

static void
complete_cb (GObject      *object,
             GAsyncResult *result,
             gpointer      user_data)
{
  GbSourceCodeAssistant *assistant = (GbSourceCodeAssistant *)object;
  GbSourceCodeAssistantCompletion *self;
  gpointer *data = user_data;
  GtkSourceCompletionContext *context;
  GtkSourceCompletionProvider *provider;
  GError *error = NULL;
  GArray *ar;
  guint i;
  GList *list = NULL;

  g_return_if_fail (GB_IS_SOURCE_CODE_ASSISTANT (assistant));

  self = GB_SOURCE_CODE_ASSISTANT_COMPLETION (data[0]);
  provider = GTK_SOURCE_COMPLETION_PROVIDER (data[0]);
  context = GTK_SOURCE_COMPLETION_CONTEXT (data[1]);
  ar = gb_source_code_assistant_complete_finish (assistant, result, &error);

  g_clear_object (&self->priv->cancellable);

  if (error && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    goto cleanup;

  if (ar)
    {
      for (i = 0; i < ar->len; i++)
        {
          GcaCompletionResult *cr;

          cr = &g_array_index (ar, GcaCompletionResult, i);
        }
    }

  gtk_source_completion_context_add_proposals (context, provider, list, TRUE);

cleanup:
  g_list_foreach (list, (GFunc)g_object_unref, NULL);
  g_list_free (list);
  g_clear_error (&error);
  g_clear_pointer (&ar, g_array_unref);
  g_object_unref (data[0]);
  g_object_unref (data[1]);
  g_free (data);
}

static void
populate (GtkSourceCompletionProvider *provider,
          GtkSourceCompletionContext  *context)
{
  GbSourceCodeAssistantCompletion *self = (GbSourceCodeAssistantCompletion *)provider;
  GbSourceCodeAssistant *assistant;
  GtkTextIter iter;
  gpointer *data;

  g_return_if_fail (GB_IS_SOURCE_CODE_ASSISTANT_COMPLETION (self));

  if (!self->priv->document)
    goto failure;

  assistant = gb_editor_document_get_code_assistant (self->priv->document);
  if (!assistant)
    goto failure;

  gtk_source_completion_context_get_iter (context, &iter);

  data = g_new0 (gpointer, 2);
  data[0] = g_object_ref (provider);
  data[1] = g_object_ref (context);

  g_clear_object (&self->priv->cancellable);
  self->priv->cancellable = g_cancellable_new ();

  g_signal_connect_object (context,
                           "cancelled",
                           G_CALLBACK (g_cancellable_cancel),
                           self->priv->cancellable,
                           G_CONNECT_SWAPPED);

  gb_source_code_assistant_complete (assistant, &iter, self->priv->cancellable,
                                     complete_cb, data);

  return;

failure:
  gtk_source_completion_context_add_proposals (context, provider, NULL, TRUE);
}

static void
init_provider (GtkSourceCompletionProviderIface *iface)
{
  iface->get_icon = get_icon;
  iface->populate = populate;
}
