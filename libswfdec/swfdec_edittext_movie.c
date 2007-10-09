/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include <string.h>

#include "swfdec_edittext_movie.h"
#include "swfdec_as_context.h"
#include "swfdec_as_strings.h"
#include "swfdec_text_format.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecEditTextMovie, swfdec_edit_text_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_edit_text_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  swfdec_rect_union (extents, extents, 
      &SWFDEC_GRAPHIC (SWFDEC_EDIT_TEXT_MOVIE (movie)->text)->extents);
}

static void
swfdec_edit_text_movie_generate_render_block (SwfdecEditTextMovie *text,
    SwfdecTextRenderBlock *block, guint start_index, guint end_index)
{
  SwfdecTextFormat *format, *format_prev;
  guint index_;
  GSList *iter;
  gint32 length, i;
  SwfdecAsValue val;
  PangoAttribute *attr_bold, *attr_color, *attr_font, *attr_italic,
		 *attr_letter_spacing, *attr_size, *attr_underline;
  // TODO: kerning

  g_assert (SWFDEC_IS_EDIT_TEXT_MOVIE (text));
  g_assert (block != NULL);
  g_assert (start_index < end_index);
  g_assert (end_index <= strlen (text->text_display));

  block->text = text->text_display + start_index;
  block->text_length = end_index - start_index;

  g_assert (text->formats != NULL);
  for (iter = text->formats; iter->next != NULL &&
      ((SwfdecFormatIndex *)(iter->next->data))->index <= start_index;
      iter = iter->next);

  index_ = start_index;
  format = ((SwfdecFormatIndex *)(iter->data))->format;

  switch (format->align) {
    case SWFDEC_TEXT_ALIGN_LEFT:
      block->align = PANGO_ALIGN_LEFT;
      block->justify = FALSE;
      break;
    case SWFDEC_TEXT_ALIGN_RIGHT:
      block->align = PANGO_ALIGN_RIGHT;
      block->justify = FALSE;
      break;
    case SWFDEC_TEXT_ALIGN_CENTER:
      block->align = PANGO_ALIGN_CENTER;
      block->justify = FALSE;
      break;
    case SWFDEC_TEXT_ALIGN_JUSTIFY:
      block->align = PANGO_ALIGN_LEFT;
      block->justify = TRUE;
      break;
  }
  block->bullet = format->bullet;
  block->indent = format->indent;
  block->leading = format->leading;
  block->block_indent = format->block_indent;
  block->left_margin = format->left_margin;
  block->right_margin = format->right_margin;

  length = swfdec_as_array_get_length (format->tab_stops);
  block->tab_stops = pango_tab_array_new (length, TRUE);
  for (i = 0; i < length; i++) {
    swfdec_as_array_get_value (format->tab_stops, i, &val);
    g_assert (SWFDEC_AS_VALUE_IS_NUMBER (&val));
    pango_tab_array_set_tab (block->tab_stops, i, PANGO_TAB_LEFT,
	SWFDEC_AS_VALUE_GET_NUMBER (&val));
  }

  block->attrs = pango_attr_list_new ();

  // Open attributes
  attr_bold = pango_attr_weight_new (
      (format->bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL));
  attr_bold->start_index = 0;

  attr_color = pango_attr_foreground_new (SWFDEC_COLOR_R (format->color),
      SWFDEC_COLOR_G (format->color), SWFDEC_COLOR_B (format->color));
  attr_color->start_index = 0;

  if (text->text->embed_fonts)
    SWFDEC_FIXME ("Using embed fonts in TextField not supported");
  attr_font = pango_attr_family_new (format->font);
  attr_font->start_index = 0;

  attr_italic = pango_attr_style_new (
      (format->italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
  attr_italic->start_index = 0;

  attr_letter_spacing = pango_attr_letter_spacing_new (
      format->letter_spacing * 20 * PANGO_SCALE); // FIXME: correct scaling?
  attr_letter_spacing->start_index = 0;

  attr_size = pango_attr_size_new_absolute (format->size * 20 * PANGO_SCALE);
  attr_size->start_index = 0;

  attr_underline = pango_attr_underline_new (
      (format->underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));
  attr_underline->start_index = 0;

  for (iter = iter->next;
      iter != NULL && ((SwfdecFormatIndex *)(iter->data))->index < end_index;
      iter = iter->next)
  {
    format_prev = format;
    index_ = ((SwfdecFormatIndex *)(iter->data))->index;
    format = ((SwfdecFormatIndex *)(iter->data))->format;

    // Change attributes if necessary
    if (format_prev->bold != format->bold) {
      attr_bold->end_index = index_ - start_index;
      pango_attr_list_insert (block->attrs, attr_bold);

      attr_bold = pango_attr_weight_new (
	  (format->bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL));
      attr_bold->start_index = index_ - start_index;
    }

    if (format_prev->color != format->color) {
      attr_color->end_index = index_ - start_index;
      pango_attr_list_insert (block->attrs, attr_color);

      attr_color = pango_attr_foreground_new (SWFDEC_COLOR_R (format->color),
	  SWFDEC_COLOR_G (format->color), SWFDEC_COLOR_B (format->color));
      attr_color->start_index = index_ - start_index;
    }

    if (format_prev->font != format->font) {
      attr_font->end_index = index_ - start_index;
      pango_attr_list_insert (block->attrs, attr_font);

      if (text->text->embed_fonts)
	SWFDEC_FIXME ("Using embed fonts in TextField not supported");
      attr_font = pango_attr_family_new (format->font);
      attr_font->start_index = index_ - start_index;
    }

    if (format_prev->italic != format->italic) {
      attr_italic->end_index = index_ - start_index;
      pango_attr_list_insert (block->attrs, attr_italic);

      attr_italic = pango_attr_style_new (
	  (format->italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
      attr_italic->start_index = index_ - start_index;
    }

    if (format_prev->letter_spacing != format->letter_spacing) {
      attr_letter_spacing->end_index = index_ - start_index;
      pango_attr_list_insert (block->attrs, attr_letter_spacing);

      // FIXME: correct scaling?
      attr_letter_spacing = pango_attr_letter_spacing_new (
	  format->letter_spacing * 20 * PANGO_SCALE);
      attr_letter_spacing->start_index = index_ - start_index;
    }

    if (format_prev->size != format->size) {
      attr_size->end_index = index_ - start_index;
      pango_attr_list_insert (block->attrs, attr_size);

      attr_size =
	pango_attr_size_new_absolute (format->size * 20 * PANGO_SCALE);
      attr_size->start_index = index_ - start_index;
    }

    if (format_prev->underline != format->underline) {
      attr_underline->end_index = index_ - start_index;
      pango_attr_list_insert (block->attrs, attr_underline);

      attr_underline = pango_attr_underline_new (
	  (format->underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));
      attr_underline->start_index = index_ - start_index;
    }
  }

  // Close attributes
  attr_bold->end_index = end_index - start_index;
  pango_attr_list_insert (block->attrs, attr_bold);
  attr_bold = NULL;

  attr_color->end_index = end_index - start_index;
  pango_attr_list_insert (block->attrs, attr_color);
  attr_color = NULL;

  attr_font->end_index = end_index - start_index;
  pango_attr_list_insert (block->attrs, attr_font);
  attr_font = NULL;

  attr_italic->end_index = end_index - start_index;
  pango_attr_list_insert (block->attrs, attr_italic);
  attr_italic = NULL;

  attr_letter_spacing->end_index = end_index - start_index;
  pango_attr_list_insert (block->attrs, attr_letter_spacing);
  attr_letter_spacing = NULL;

  attr_size->end_index = end_index - start_index;
  pango_attr_list_insert (block->attrs, attr_size);
  attr_size = NULL;

  attr_underline->end_index = end_index - start_index;
  pango_attr_list_insert (block->attrs, attr_underline);
  attr_underline = NULL;
}

static void
swfdec_edit_text_movie_generate_render_blocks (SwfdecEditTextMovie *text)
{
  const char *p, *end;
  int lines, i;

  g_assert (SWFDEC_IS_EDIT_TEXT_MOVIE (text));

  lines = 0;
  p = text->text_display;
  while (p != NULL && *p != '\0') {
    lines++;
    p = strchr (p, '\r');
    if (p != NULL) p++;
  }

  text->blocks = g_new0 (SwfdecTextRenderBlock, lines + 1);

  i = 0;
  p = text->text_display;
  while (*p != '\0') {
    g_assert (i < lines);
    end = strchr (p, '\r');
    if (end == NULL)
      end = strchr (p, '\0');

    swfdec_edit_text_movie_generate_render_block (text, &text->blocks[i],
	p - text->text_display, end - text->text_display);

    p = end;
    if (*p == '\r') p++;
  }
}

static void
swfdec_edit_text_movie_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (movie);

  if (!fill) {
    cairo_rectangle (cr, movie->extents.x0, movie->extents.y0,
	movie->extents.x1 - movie->extents.x0,
	movie->extents.y1 - movie->extents.y0);
    return;
  }

  if (text->blocks == NULL)
    swfdec_edit_text_movie_generate_render_blocks (text);

  if (text->blocks[0].text == NULL)
    return;
  swfdec_edit_text_render (text->text, cr, text->blocks, trans, inval, fill);
}

void
swfdec_edit_text_movie_format_changed (SwfdecEditTextMovie *text)
{
  int i;

  if (text->blocks != NULL) {
    for (i = 0; text->blocks[i].text != NULL; i++) {
      // tabs
      pango_attr_list_unref (text->blocks[i].attrs);
    }
    g_free (text->blocks);
    text->blocks = NULL;

    swfdec_movie_invalidate (SWFDEC_MOVIE (text));
  }
}

static void
swfdec_edit_text_movie_dispose (GObject *object)
{
  int i;
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (object);

  if (text->blocks) {
    for (i = 0; text->blocks[i].text != NULL; i++) {
      if (text->blocks[i].attrs != NULL)
	pango_attr_list_unref (text->blocks[i].attrs);
      if (text->blocks[i].tab_stops != NULL)
	pango_tab_array_free (text->blocks[i].tab_stops);
    }
    g_free (text->blocks);
    text->blocks = NULL;
  }

  G_OBJECT_CLASS (swfdec_edit_text_movie_parent_class)->dispose (object);
}

static void
swfdec_edit_text_movie_iterate (SwfdecMovie *movie)
{
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (movie);
  SwfdecAsObject *parent;
  const char *s;
  SwfdecAsValue val = { 0, };

  if (text->text->variable == NULL)
    return;

  parent = SWFDEC_AS_OBJECT (movie->parent);
  swfdec_as_context_eval (parent->context, parent, text->text->variable, &val);
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&val))
    return;

  s = swfdec_as_value_to_string (parent->context, &val);
  g_assert (s);
  if (text->text_input == s)
    return;

  swfdec_edit_text_movie_set_text (text, s, text->text->html);
}

static void
swfdec_edit_text_movie_init_movie (SwfdecMovie *movie)
{
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (movie);
  SwfdecAsContext *cx;
  SwfdecAsObject *parent;
  SwfdecAsValue val = { 0, };
  const char *s;

  cx = SWFDEC_AS_OBJECT (movie)->context;

  // format
  text->format_new = SWFDEC_TEXT_FORMAT (swfdec_text_format_new (cx));
  swfdec_text_format_set_defaults (text->format_new);
  text->format_new->color = text->text->color;
  text->format_new->align = text->text->align;
  text->format_new->left_margin = text->text->left_margin;
  text->format_new->right_margin = text->text->right_margin;
  text->format_new->indent = text->text->indent;
  text->format_new->leading = text->text->leading;

  // text
  if (text->text->text_input != NULL) {
    swfdec_edit_text_movie_set_text (text,
	swfdec_as_context_get_string (cx, text->text->text_input),
	text->text->html);
  }

  // variable
  if (text->text->variable != NULL)
  {
    text->variable = swfdec_as_context_get_string (cx, text->text->variable);

    parent = SWFDEC_AS_OBJECT (movie->parent);
    swfdec_as_context_eval (parent->context, parent, text->variable, &val);
    if (!SWFDEC_AS_VALUE_IS_UNDEFINED (&val)) {
      s = swfdec_as_value_to_string (parent->context, &val);
      g_assert (s);
      if (text->text_input != s)
	swfdec_edit_text_movie_set_text (text, s, text->text->html);
    } else {
      SWFDEC_LOG ("setting variable %s to \"%s\"", text->variable,
	  text->text_input ? text->text_input : "");
      s = text->text_input ? swfdec_as_context_get_string (parent->context,
	  text->text_input) : SWFDEC_AS_STR_EMPTY;
      SWFDEC_AS_VALUE_SET_STRING (&val, s);
      swfdec_as_context_eval_set (parent->context, parent, text->variable,
	  &val);
    }
  }
}

static void
swfdec_edit_text_movie_class_init (SwfdecEditTextMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_edit_text_movie_dispose;

  movie_class->init_movie = swfdec_edit_text_movie_init_movie;
  movie_class->update_extents = swfdec_edit_text_movie_update_extents;
  movie_class->render = swfdec_edit_text_movie_render;
  movie_class->iterate_start = swfdec_edit_text_movie_iterate;
}

static void
swfdec_edit_text_movie_init (SwfdecEditTextMovie *text)
{
}

void
swfdec_edit_text_movie_set_text_format (SwfdecEditTextMovie *text,
    SwfdecTextFormat *format, guint start_index, guint end_index)
{
  SwfdecFormatIndex *findex, *findex_new;
  guint findex_end_index;
  GSList *iter, *next;

  g_return_if_fail (SWFDEC_IS_EDIT_TEXT_MOVIE (text));
  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (format));
  g_return_if_fail (start_index < end_index);
  g_return_if_fail (end_index <= strlen (text->text_display));

  g_assert (text->formats != NULL);
  g_assert (text->formats->data != NULL);
  g_assert (((SwfdecFormatIndex *)text->formats->data)->index == 0);
  for (iter = text->formats; iter != NULL &&
      ((SwfdecFormatIndex *)iter->data)->index < end_index;
      iter = next)
  {
    next = iter->next;
    findex = iter->data;
    if (iter->next != NULL) {
      findex_end_index =
	((SwfdecFormatIndex *)iter->next->data)->index;
    } else {
      findex_end_index = strlen (text->text_display);
    }

    if (findex_end_index < start_index)
      continue;

    if (findex_end_index > end_index) {
      findex_new = g_new (SwfdecFormatIndex, 1);
      findex_new->index = end_index;
      findex_new->format = swfdec_text_format_copy (findex->format);

      iter = g_slist_insert (iter, findex_new, 1);
    }

    if (findex->index < start_index) {
      findex_new = g_new (SwfdecFormatIndex, 1);
      findex_new->index = start_index;
      findex_new->format = swfdec_text_format_copy (findex->format);
      swfdec_text_format_add (findex_new->format, format);

      iter = g_slist_insert (iter, findex_new, 1);
    } else {
      swfdec_text_format_add (findex->format, format);
    }
  }

  swfdec_edit_text_movie_format_changed (text);
}

void
swfdec_edit_text_movie_set_text (SwfdecEditTextMovie *text, const char *str,
    gboolean html)
{
  SwfdecFormatIndex *block;
  GSList *iter;

  g_return_if_fail (SWFDEC_IS_EDIT_TEXT_MOVIE (text));

  text->text_input = str;

  // remove old formatting info
  iter = text->formats;
  while (iter) {
    g_free (iter->data);
    iter = g_slist_next (iter);
  }
  g_slist_free (text->formats);
  text->formats = NULL;

  // add the default style
  if (html && SWFDEC_AS_OBJECT (text)->context->version < 8)
    swfdec_text_format_set_defaults (text->format_new);
  block = g_new (SwfdecFormatIndex, 1);
  block->index = 0;
  g_assert (SWFDEC_IS_TEXT_FORMAT (text->format_new));
  block->format = text->format_new;
  text->formats = g_slist_prepend (text->formats, block);

  if (html) {
    swfdec_edit_text_movie_html_parse (text, str);
  } else {
    // change all \n to \r
    if (strchr (str, '\n') != NULL) {
      char *string, *p;

      string = g_strdup (str);
      p = string;
      while ((p = strchr (p, '\n')) != NULL) {
	*p = '\r';
      }
      text->text_display = swfdec_as_context_give_string (
	  SWFDEC_AS_OBJECT (text)->context, string);
    } else {
      text->text_display = str;
    }
  }

  swfdec_edit_text_movie_format_changed (text);
}
