/* Swfedit
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <gtk/gtk.h>
#include <libswfdec/swfdec_buffer.h>
#include "swfedit_token.h"

/*** CONVERTERS ***/

static gboolean
swfedit_binary_read (const char *s, gpointer* result)
{
  GByteArray *array = g_byte_array_new ();
  guint8 byte;

  while (g_ascii_isspace (*s)) s++;
  do {
    if (s[0] >= '0' && s[0] <= '9')
      byte = s[0] - '0';
    else if (s[0] >= 'a' && s[0] <= 'f')
      byte = s[0] + 10 - 'a';
    else if (s[0] >= 'A' && s[0] <= 'F')
      byte = s[0] + 10 - 'A';
    else
      break;
    s++;
    byte *= 255;
    if (s[0] >= '0' && s[0] <= '9')
      byte = s[0] - '0';
    else if (s[0] >= 'a' && s[0] <= 'f')
      byte = s[0] + 10 - 'a';
    else if (s[0] >= 'A' && s[0] <= 'F')
      byte = s[0] + 10 - 'A';
    else
      break;
    s++;
    g_byte_array_append (array, &byte, 1);
    while (g_ascii_isspace (*s)) s++;
  } while (TRUE);
  if (*s == '\0') {
    SwfdecBuffer *buffer = swfdec_buffer_new ();
    buffer->length = array->len;
    buffer->data = array->data;
    g_byte_array_free (array, FALSE);
    *result = buffer;
    return TRUE;
  }
  g_byte_array_free (array, TRUE);
  return FALSE;
}

static char *
swfedit_binary_write (gconstpointer value)
{
  guint i;
  const SwfdecBuffer *buffer = value;
  GString *string = g_string_new ("");

  for (i = 0; i < buffer->length; i++) {
    if (i && i % 4 == 0)
      g_string_append_c (string, ' ');
    g_string_append_printf (string, "%2X", buffer->data[i]);
  }
  return g_string_free (string, FALSE);
}

static gboolean
swfedit_read_unsigned (const char *s, gulong max, gpointer* result)
{
  char *end;
  gulong u;

  g_assert (max <= G_MAXUINT);
  u = strtoul (s, &end, 10);
  if (*end != '\0')
    return FALSE;
  if (u > max)
    return FALSE;
  *result = GUINT_TO_POINTER ((guint) u);
  return TRUE;
}

static gboolean
swfedit_uint8_read (const char *s, gpointer* result)
{
  return swfedit_read_unsigned (s, G_MAXUINT8, result);
}

static gboolean
swfedit_uint16_read (const char *s, gpointer* result)
{
  return swfedit_read_unsigned (s, G_MAXUINT16, result);
}

static gboolean
swfedit_uint32_read (const char *s, gpointer* result)
{
  return swfedit_read_unsigned (s, G_MAXUINT32, result);
}

static char *
swfedit_write_unsigned (gconstpointer value)
{
  return g_strdup_printf ("%u", GPOINTER_TO_UINT (value));
}

struct {
  gboolean	(* read)	(const char *s, gpointer *);
  char *	(* write)	(gconstpointer value);
  void	  	(* free)	(gpointer value);
} converters[SWFEDIT_N_TOKENS] = {
  { NULL, NULL, g_object_unref },
  { swfedit_binary_read, swfedit_binary_write, (GDestroyNotify) swfdec_buffer_unref },
  { swfedit_uint8_read, swfedit_write_unsigned, NULL },
  { swfedit_uint16_read, swfedit_write_unsigned, NULL },
  { swfedit_uint32_read, swfedit_write_unsigned, NULL },
};

/*** STRUCTS ***/

typedef struct {
  char *		name;
  SwfeditTokenType	type;
  gpointer		value;
} Entry;

/*** GTK_TREE_MODEL ***/

static GtkTreeModelFlags 
swfedit_token_get_flags (GtkTreeModel *tree_model)
{
  return 0;
}

static gint
swfedit_token_get_n_columns (GtkTreeModel *tree_model)
{
  SwfeditToken *token = SWFEDIT_TOKEN (tree_model);

  return token->tokens->len;
}

static GType
swfedit_token_get_column_type (GtkTreeModel *tree_model, gint index_)
{
  switch (index_) {
    case SWFEDIT_COLUMN_NAME:
      return G_TYPE_STRING;
    case SWFEDIT_COLUMN_VALUE_VISIBLE:
      return G_TYPE_BOOLEAN;
    case SWFEDIT_COLUMN_VALUE:
      return G_TYPE_STRING;
    default:
      break;
  }
  g_assert_not_reached ();
  return G_TYPE_NONE;
}

static gboolean
swfedit_token_get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
  SwfeditToken *token = SWFEDIT_TOKEN (tree_model);
  guint i = gtk_tree_path_get_indices (path)[0];
  Entry *entry;
  
  if (i > token->tokens->len)
    return FALSE;
  entry = &g_array_index (token->tokens, Entry, i);
  if (gtk_tree_path_get_depth (path) > 1) {
    GtkTreePath *new;
    int j;
    int *indices;
    gboolean ret;

    if (entry->type != SWFEDIT_TOKEN_OBJECT)
      return FALSE;
    new = gtk_tree_path_new ();
    indices = gtk_tree_path_get_indices (path);
    for (j = 1; j < gtk_tree_path_get_depth (path); j++) {
      gtk_tree_path_append_index (path, indices[j]);
    }
    ret = swfedit_token_get_iter (GTK_TREE_MODEL (entry->value), iter, new);
    gtk_tree_path_free (new);
    return ret;
  } else {
    iter->stamp = 0; /* FIXME */
    iter->user_data = token;
    iter->user_data2 = GINT_TO_POINTER (i);
    return TRUE;
  }
}

static GtkTreePath *
swfedit_token_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);
  GtkTreePath *path = gtk_tree_path_new_from_indices (GPOINTER_TO_INT (iter->user_data2), -1);

  while (token->parent) {
    guint i;
    SwfeditToken *parent = token->parent;
    for (i = 0; i < parent->tokens->len; i++) {
      Entry *entry = &g_array_index (parent->tokens, Entry, i);
      if (entry->type != SWFEDIT_TOKEN_OBJECT)
	continue;
      if (entry->value == token)
	break;
    }
    gtk_tree_path_prepend_index (path, i);
    token = parent;
  }
  return path;
}

static void 
swfedit_token_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter,
    gint column, GValue *value)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);
  Entry *entry = &g_array_index (token->tokens, Entry, GPOINTER_TO_INT (iter->user_data2));

  switch (column) {
    case SWFEDIT_COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, entry->name);
      return;
    case SWFEDIT_COLUMN_VALUE_VISIBLE:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, converters[entry->type].write != NULL);
      return;
    case SWFEDIT_COLUMN_VALUE:
      g_value_init (value, G_TYPE_STRING);
      if (converters[entry->type].write)
	g_value_take_string (value, converters[entry->type].write (entry->value));
      return;
    default:
      break;
  }
  g_assert_not_reached ();
}

static gboolean
swfedit_token_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);

  if ((guint) GPOINTER_TO_INT (iter->user_data2) + 1 >= token->tokens->len)
    return FALSE;

  iter->user_data2++;
  return TRUE;
}

static gboolean
swfedit_token_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
  SwfeditToken *token = SWFEDIT_TOKEN (parent->user_data);
  Entry *entry = &g_array_index (token->tokens, Entry, GPOINTER_TO_INT (parent->user_data2));

  if (entry->type != SWFEDIT_TOKEN_OBJECT)
    return FALSE;

  iter->stamp = 0; /* FIXME */
  iter->user_data = entry->value;
  iter->user_data2 = GINT_TO_POINTER (0);
  return TRUE;
}

static gboolean
swfedit_token_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);
  Entry *entry = &g_array_index (token->tokens, Entry, GPOINTER_TO_INT (iter->user_data2));

  return entry->type == SWFEDIT_TOKEN_OBJECT;
}

static gint
swfedit_token_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);
  Entry *entry = &g_array_index (token->tokens, Entry, GPOINTER_TO_INT (iter->user_data2));

  if (entry->type != SWFEDIT_TOKEN_OBJECT)
    return FALSE;

  token = entry->value;
  return token->tokens->len;
}

static gboolean
swfedit_token_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *parent, gint n)
{
  SwfeditToken *token;
  Entry *entry;

  if (parent) {
    token = SWFEDIT_TOKEN (parent->user_data);
    entry = &g_array_index (token->tokens, Entry, GPOINTER_TO_INT (parent->user_data2));

    if (entry->type != SWFEDIT_TOKEN_OBJECT)
      return FALSE;

    token = entry->value;
    if ((guint) n >= token->tokens->len)
      return FALSE;
  }
  iter->stamp = 0; /* FIXME */
  iter->user_data = token;
  iter->user_data2 = GINT_TO_POINTER (n);
  return TRUE;
}

static gboolean
swfedit_token_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
  guint i;
  SwfeditToken *token = SWFEDIT_TOKEN (child->user_data);
  SwfeditToken *parent = token->parent;

  if (parent == NULL)
    return FALSE;

  for (i = 0; i < parent->tokens->len; i++) {
    Entry *entry = &g_array_index (parent->tokens, Entry, i);
    if (entry->type != SWFEDIT_TOKEN_OBJECT)
      continue;
    if (entry->value == token)
      break;
  }
  iter->stamp = 0; /* FIXME */
  iter->user_data = token;
  iter->user_data2 = GINT_TO_POINTER (i);
  return TRUE;
}

static void
swfedit_token_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = swfedit_token_get_flags;
  iface->get_n_columns = swfedit_token_get_n_columns;
  iface->get_column_type = swfedit_token_get_column_type;
  iface->get_iter = swfedit_token_get_iter;
  iface->get_path = swfedit_token_get_path;
  iface->get_value = swfedit_token_get_value;
  iface->iter_next = swfedit_token_iter_next;
  iface->iter_children = swfedit_token_iter_children;
  iface->iter_has_child = swfedit_token_iter_has_child;
  iface->iter_n_children = swfedit_token_iter_n_children;
  iface->iter_nth_child = swfedit_token_iter_nth_child;
  iface->iter_parent = swfedit_token_iter_parent;
}

/*** SWFEDIT_TOKEN ***/

G_DEFINE_TYPE_WITH_CODE (SwfeditToken, swfedit_token, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, swfedit_token_tree_model_init))

static void
swfedit_token_dispose (GObject *object)
{
  SwfeditToken *token = SWFEDIT_TOKEN (object);
  guint i;

  for (i = 0; i < token->tokens->len; i++) {
    Entry *entry = &g_array_index (token->tokens, Entry, i);
    g_free (entry->name);
    if (converters[entry->type].free)
      converters[entry->type].free (entry->value);
  }
  g_array_free (token->tokens, TRUE);

  G_OBJECT_CLASS (swfedit_token_parent_class)->dispose (object);
}

static void
swfedit_token_class_init (SwfeditTokenClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = swfedit_token_dispose;
}

static void
swfedit_token_init (SwfeditToken *token)
{
  token->tokens = g_array_new (FALSE, FALSE, sizeof (Entry));
}

SwfeditToken *
swfedit_token_new (void)
{
  SwfeditToken *token;

  token = g_object_new (SWFEDIT_TYPE_TOKEN, NULL);
  return token;
}

void
swfedit_token_add (SwfeditToken *token, const char *name, SwfeditTokenType type, gpointer value)
{
  Entry entry = { NULL, type, value };

  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (name != NULL);
  g_return_if_fail (type < SWFEDIT_N_TOKENS);

  entry.name = g_strdup (name);
  g_array_append_val (token->tokens, entry);
}

void
swfedit_token_set (SwfeditToken *token, GtkTreeIter *iter, const char *value)
{
  GtkTreeModel *model;
  Entry *entry;
  guint i;
  gpointer new;
  GtkTreePath *path;

  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (value != NULL);

  model = GTK_TREE_MODEL (token);
  token = iter->user_data;
  i = GPOINTER_TO_UINT (iter->user_data2);
  entry = &g_array_index (token->tokens, Entry, i);
  if (converters[entry->type].read == NULL)
    return;
  if (!converters[entry->type].read (value, &new))
    return;
  if (converters[entry->type].free != NULL)
    converters[entry->type].free (entry->value);
  entry->value = new;

  path = gtk_tree_model_get_path (model, iter);
  gtk_tree_model_row_changed (model, path, iter);
  gtk_tree_path_free (path);
}

