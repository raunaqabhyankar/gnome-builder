/* gb-source-code-assistant-completion.h
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

#ifndef GB_SOURCE_CODE_ASSISTANT_COMPLETION_H
#define GB_SOURCE_CODE_ASSISTANT_COMPLETION_H

#include <gtksourceview/gtksource.h>

#include "gb-editor-document.h"

G_BEGIN_DECLS

#define GB_TYPE_SOURCE_CODE_ASSISTANT_COMPLETION            (gb_source_code_assistant_completion_get_type())
#define GB_SOURCE_CODE_ASSISTANT_COMPLETION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GB_TYPE_SOURCE_CODE_ASSISTANT_COMPLETION, GbSourceCodeAssistantCompletion))
#define GB_SOURCE_CODE_ASSISTANT_COMPLETION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GB_TYPE_SOURCE_CODE_ASSISTANT_COMPLETION, GbSourceCodeAssistantCompletion const))
#define GB_SOURCE_CODE_ASSISTANT_COMPLETION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GB_TYPE_SOURCE_CODE_ASSISTANT_COMPLETION, GbSourceCodeAssistantCompletionClass))
#define GB_IS_SOURCE_CODE_ASSISTANT_COMPLETION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GB_TYPE_SOURCE_CODE_ASSISTANT_COMPLETION))
#define GB_IS_SOURCE_CODE_ASSISTANT_COMPLETION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GB_TYPE_SOURCE_CODE_ASSISTANT_COMPLETION))
#define GB_SOURCE_CODE_ASSISTANT_COMPLETION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GB_TYPE_SOURCE_CODE_ASSISTANT_COMPLETION, GbSourceCodeAssistantCompletionClass))

typedef struct _GbSourceCodeAssistantCompletion        GbSourceCodeAssistantCompletion;
typedef struct _GbSourceCodeAssistantCompletionClass   GbSourceCodeAssistantCompletionClass;
typedef struct _GbSourceCodeAssistantCompletionPrivate GbSourceCodeAssistantCompletionPrivate;

struct _GbSourceCodeAssistantCompletion
{
  GObject parent;

  /*< private >*/
  GbSourceCodeAssistantCompletionPrivate *priv;
};

struct _GbSourceCodeAssistantCompletionClass
{
  GObjectClass parent;
};

GType                        gb_source_code_assistant_completion_get_type      (void);
GtkSourceCompletionProvider *gb_source_code_assistant_completion_new           (void);
void                         gb_source_code_assistant_completion_set_document  (GbSourceCodeAssistantCompletion *completion,
                                                                                GbEditorDocument                *document);

G_END_DECLS

#endif /* GB_SOURCE_CODE_ASSISTANT_COMPLETION_H */
