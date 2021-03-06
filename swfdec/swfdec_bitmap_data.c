/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
 *		 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_bitmap_data.h"

#include <math.h>

#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_color.h"
#include "swfdec_color_transform_as.h"
#include "swfdec_debug.h"
#include "swfdec_image.h"
#include "swfdec_player_internal.h"
#include "swfdec_rectangle.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_resource.h"
#include "swfdec_utils.h"

enum {
  INVALIDATE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];
G_DEFINE_TYPE (SwfdecBitmapData, swfdec_bitmap_data, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_bitmap_data_invalidate (SwfdecBitmapData *bitmap, guint x, guint y, guint w, guint h)
{
  SwfdecRectangle rect = { x, y, w, h };

  g_return_if_fail (w > 0);
  g_return_if_fail (h > 0);

  if (bitmap->surface)
    cairo_surface_mark_dirty_rectangle (bitmap->surface, x, y, w, h);
  g_signal_emit (bitmap, signals[INVALIDATE], 0, &rect);
}

static void
swfdec_bitmap_data_clear (SwfdecBitmapData *bitmap)
{
  if (bitmap->surface == NULL)
    return;

  swfdec_bitmap_data_invalidate (bitmap, 0, 0, bitmap->width, bitmap->height);
  cairo_surface_destroy (bitmap->surface);
  swfdec_as_context_unuse_mem (swfdec_gc_object_get_context (bitmap), 
      4 * bitmap->width * bitmap->height);
  bitmap->surface = NULL;
  bitmap->width = 0;
  bitmap->height = 0;
}

static void
swfdec_bitmap_data_dispose (GObject *object)
{
  SwfdecBitmapData *bitmap = SWFDEC_BITMAP_DATA (object);

  swfdec_bitmap_data_clear (bitmap);

  G_OBJECT_CLASS (swfdec_bitmap_data_parent_class)->dispose (object);
}

static void
swfdec_bitmap_data_class_init (SwfdecBitmapDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_bitmap_data_dispose;

  signals[INVALIDATE] = g_signal_new ("invalidate", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__BOXED,
      G_TYPE_NONE, 1, SWFDEC_TYPE_RECTANGLE);
}

static void
swfdec_bitmap_data_init (SwfdecBitmapData *transform)
{
}

SwfdecBitmapData *
swfdec_bitmap_data_new (SwfdecAsContext *context, gboolean transparent, guint width, guint height)
{
  SwfdecBitmapData *bitmap;
  SwfdecAsObject *object;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (!swfdec_as_context_try_use_mem (context, width * height * 4))
    return NULL;

  bitmap = g_object_new (SWFDEC_TYPE_BITMAP_DATA, "context", context, NULL);
  bitmap->width = width;
  bitmap->height = height;
  bitmap->surface = cairo_image_surface_create (
      transparent ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, width, height);

  object = swfdec_as_object_new (context, NULL);
  swfdec_as_object_set_constructor_by_name (object,
      SWFDEC_AS_STR_flash, SWFDEC_AS_STR_display, SWFDEC_AS_STR_BitmapData, NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (bitmap));

  return bitmap;
}

#define swfdec_surface_has_alpha(surface) (cairo_surface_get_content (surface) & CAIRO_CONTENT_ALPHA)

SWFDEC_AS_NATIVE (1100, 40, swfdec_bitmap_data_loadBitmap)
void
swfdec_bitmap_data_loadBitmap (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  SwfdecImage *image;
  SwfdecMovie *movie;
  const char *name;
  cairo_surface_t *isurface;
  cairo_t *cr;

  SWFDEC_AS_CHECK (0, NULL, "s", &name);

  movie = swfdec_as_frame_get_target (cx->frame);
  if (movie == NULL) {
    SWFDEC_ERROR ("no target to load from");
    return;
  }
  image = swfdec_resource_get_export (movie->resource, name);
  if (!SWFDEC_IS_IMAGE (image)) {
    SWFDEC_ERROR ("loadBitmap cannot find image with name %s", name);
    return;
  }

  /* FIXME: improve this to not create an image if there is one cached */
  isurface = swfdec_image_create_surface (image, NULL);
  if (isurface == NULL)
    return;

  /* FIXME: use image directly instead of doing a copy and then deleting it */
  bitmap = swfdec_bitmap_data_new (cx, 
      swfdec_surface_has_alpha (isurface),
      cairo_image_surface_get_width (isurface),
      cairo_image_surface_get_height (isurface));
  if (bitmap == NULL)
    return;

  cr = cairo_create (bitmap->surface);
  cairo_set_source_surface (cr, isurface, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (isurface);
  SWFDEC_AS_VALUE_SET_OBJECT (ret, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (bitmap)));
}

// properties
SWFDEC_AS_NATIVE (1100, 100, swfdec_bitmap_data_do_get_width)
void
swfdec_bitmap_data_do_get_width (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  *ret = swfdec_as_value_from_integer (cx, bitmap->surface ? (int) bitmap->width : -1);
}

SWFDEC_AS_NATIVE (1100, 101, swfdec_bitmap_data_set_width)
void
swfdec_bitmap_data_set_width (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.width (set)");
}

SWFDEC_AS_NATIVE (1100, 102, swfdec_bitmap_data_do_get_height)
void
swfdec_bitmap_data_do_get_height (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  *ret = swfdec_as_value_from_integer (cx, bitmap->surface ? (int) bitmap->height : -1);
}

SWFDEC_AS_NATIVE (1100, 103, swfdec_bitmap_data_set_height)
void
swfdec_bitmap_data_set_height (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.height (set)");
}

SWFDEC_AS_NATIVE (1100, 104, swfdec_bitmap_data_get_rectangle)
void
swfdec_bitmap_data_get_rectangle (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  SwfdecAsObject *o;
  SwfdecAsValue args[4];

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  *ret = swfdec_as_value_from_integer (cx, -1);
  if (bitmap->surface == NULL)
    return;
  
  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_flash, args);
  swfdec_as_value_get_variable (cx, args, SWFDEC_AS_STR_geom, args);
  swfdec_as_value_get_variable (cx, args, SWFDEC_AS_STR_Rectangle, args);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (*args))
    return;
  o = SWFDEC_AS_VALUE_GET_OBJECT (*args);
  if (!SWFDEC_IS_AS_FUNCTION (o->relay))
    return;

  args[0] = swfdec_as_value_from_integer (cx, 0);
  args[1] = swfdec_as_value_from_integer (cx, 0);
  args[2] = swfdec_as_value_from_integer (cx, bitmap->width);
  args[3] = swfdec_as_value_from_integer (cx, bitmap->height);
  swfdec_as_object_create (SWFDEC_AS_FUNCTION (o->relay), 4, args, ret);
}

SWFDEC_AS_NATIVE (1100, 105, swfdec_bitmap_data_set_rectangle)
void
swfdec_bitmap_data_set_rectangle (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.rectangle (set)");
}

SWFDEC_AS_NATIVE (1100, 106, swfdec_bitmap_data_get_transparent)
void
swfdec_bitmap_data_get_transparent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  if (bitmap->surface) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, 
	swfdec_surface_has_alpha (bitmap->surface) ? TRUE : FALSE);
  } else {
    *ret = swfdec_as_value_from_integer (cx, -1);
  }
}

SWFDEC_AS_NATIVE (1100, 107, swfdec_bitmap_data_set_transparent)
void
swfdec_bitmap_data_set_transparent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.transparent (set)");
}

#define SWFDEC_COLOR_MULTIPLY(color) SWFDEC_COLOR_COMBINE ( \
    (SWFDEC_COLOR_ALPHA (color) * SWFDEC_COLOR_RED (color) + 128) / 255, \
    (SWFDEC_COLOR_ALPHA (color) * SWFDEC_COLOR_GREEN (color) + 128) / 255, \
    (SWFDEC_COLOR_ALPHA (color) * SWFDEC_COLOR_BLUE (color) + 128) / 255, \
    SWFDEC_COLOR_ALPHA (color))

/* FIXME: This algorithm rounds wrong, no idea how though */
#define SWFDEC_COLOR_UNMULTIPLY(color) (SWFDEC_COLOR_ALPHA (color) ? (\
    SWFDEC_COLOR_ALPHA (color) == 0xFF ? color : SWFDEC_COLOR_COMBINE ( \
    (SWFDEC_COLOR_RED (color) * 255 + SWFDEC_COLOR_ALPHA (color) / 2) / SWFDEC_COLOR_ALPHA (color), \
    (SWFDEC_COLOR_GREEN (color) * 255 + SWFDEC_COLOR_ALPHA (color) / 2) / SWFDEC_COLOR_ALPHA (color), \
    (SWFDEC_COLOR_BLUE (color) * 255 + SWFDEC_COLOR_ALPHA (color) / 2) / SWFDEC_COLOR_ALPHA (color), \
    SWFDEC_COLOR_ALPHA (color))) : 0)

SWFDEC_AS_NATIVE (1100, 1, swfdec_bitmap_data_getPixel)
void
swfdec_bitmap_data_getPixel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  guint x, y, color;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "ii", &x, &y);

  if (bitmap->surface == NULL || x >= (guint) bitmap->width || y >= (guint) bitmap->height)
    return;

  color = swfdec_bitmap_data_get_pixel (bitmap, x, y);
  color = SWFDEC_COLOR_UNMULTIPLY (color);
  color &= SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0);
  *ret = swfdec_as_value_from_integer (cx, color);
}

SWFDEC_AS_NATIVE (1100, 2, swfdec_bitmap_data_setPixel)
void
swfdec_bitmap_data_setPixel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  guint x, y, color;
  SwfdecColor old;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "iii", &x, &y, &color);

  if (bitmap->surface == NULL || x >= bitmap->width || y >= bitmap->height)
    return;

  old = swfdec_bitmap_data_get_pixel (bitmap, x, y);
  old |= SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0);
  color = old & SWFDEC_COLOR_OPAQUE (color);
  color = SWFDEC_COLOR_MULTIPLY (color);
  swfdec_bitmap_data_set_pixel (bitmap, x, y, color);
}

static gboolean
swfdec_rectangle_from_as_object (SwfdecRectangle *rect, SwfdecAsObject *object)
{
  SwfdecAsValue *val;
  SwfdecAsContext *cx = object->context;

  /* FIXME: This function is untested */
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_x);
  if (val)
    rect->x = swfdec_as_value_to_integer (cx, *val);
  else
    rect->x = 0;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_y);
  if (val)
    rect->y = swfdec_as_value_to_integer (cx, *val);
  else
    rect->y = 0;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_width);
  if (val)
    rect->width = swfdec_as_value_to_integer (cx, *val);
  else
    rect->width = 0;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_height);
  if (val)
    rect->height = swfdec_as_value_to_integer (cx, *val);
  else
    rect->height = 0;
  return rect->width > 0 && rect->height > 0;
}

static void
swfdec_point_from_as_object (int *x, int *y, SwfdecAsObject *object)
{
  SwfdecAsValue *val;
  SwfdecAsContext *cx = object->context;

  /* FIXME: This function is untested */
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_x);
  *x = swfdec_as_value_to_integer (cx, *val);
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_y);
  *y = swfdec_as_value_to_integer (cx, *val);
}

SWFDEC_AS_NATIVE (1100, 3, swfdec_bitmap_data_fillRect)
void
swfdec_bitmap_data_fillRect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  guint color;
  SwfdecAsObject *recto;
  SwfdecRectangle rect;
  cairo_t *cr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "oi", &recto, &color);

  if (!swfdec_rectangle_from_as_object (&rect, recto) || bitmap->surface == NULL)
    return;

  if (!swfdec_surface_has_alpha(bitmap->surface))
    /* We can treat a guint as a SwfdecColor */
    color = SWFDEC_COLOR_OPAQUE (color);

  cr = cairo_create (bitmap->surface);
  swfdec_color_set_source (cr, color);
  cairo_rectangle (cr, rect.x, rect.y, rect.width, rect.height);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_fill (cr);
  cairo_destroy(cr);
  swfdec_bitmap_data_invalidate (bitmap, rect.x, rect.y, rect.width, rect.height);
}

SWFDEC_AS_NATIVE (1100, 4, swfdec_bitmap_data_copyPixels)
void
swfdec_bitmap_data_copyPixels (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap, *source, *alpha = NULL;
  SwfdecAsObject *recto = NULL, *pt, *apt = NULL, *so, *ao = NULL;
  SwfdecRectangle rect;
  gboolean copy_alpha = FALSE;
  SwfdecColorTransform ctrans;
  cairo_pattern_t *pattern;
  cairo_t *cr;
  int x, y;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "ooo|OOb", &so, &recto, &pt,
      &ao, &apt, &copy_alpha);

  if (bitmap->surface == NULL ||
      !SWFDEC_IS_BITMAP_DATA (so->relay) ||
      (source = SWFDEC_BITMAP_DATA (so->relay))->surface == NULL ||
      (ao != NULL && (!SWFDEC_IS_BITMAP_DATA (ao->relay) || 
		    (alpha = SWFDEC_BITMAP_DATA (ao->relay))->surface == NULL)) ||
      !swfdec_rectangle_from_as_object (&rect, recto))
    return;

  x = rect.x;
  y = rect.y;
  swfdec_point_from_as_object (&rect.x, &rect.y, pt);
  cr = cairo_create (bitmap->surface);
  cairo_rectangle (cr, rect.x, rect.y, rect.width, rect.height);
  cairo_clip (cr);
  cairo_translate (cr, rect.x - x, rect.y - y);
  swfdec_color_transform_init_identity (&ctrans);
  pattern = swfdec_bitmap_data_get_pattern (source,
	SWFDEC_PLAYER (cx)->priv->renderer,
	&ctrans);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
  if (bitmap == source) {
    /* FIXME Is this necessary or does Cairo handle source == target? */
    cairo_push_group_with_content (cr, cairo_surface_get_content (source->surface));
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    cairo_pop_group_to_source (cr);
  }

  if (swfdec_surface_has_alpha (bitmap->surface) && !copy_alpha) {
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  }

  if (alpha) {
    cairo_push_group_with_content (cr, cairo_surface_get_content (source->surface));
    pattern = swfdec_bitmap_data_get_pattern (alpha,
	  SWFDEC_PLAYER (cx)->priv->renderer,
	  &ctrans);
    if (apt) {
      int mask_x, mask_y;
      swfdec_point_from_as_object (&mask_x, &mask_y, apt);
      cairo_translate (cr, x - mask_x, y - mask_y);
    } else {
      cairo_translate (cr, x, y);
    }
    cairo_mask (cr, pattern);
    cairo_pattern_destroy (pattern);
    cairo_pop_group_to_source (cr);
  }
  cairo_paint (cr);
  cairo_destroy (cr);
  swfdec_bitmap_data_invalidate (bitmap, rect.x, rect.y, rect.width, rect.height);
}

SWFDEC_AS_NATIVE (1100, 5, swfdec_bitmap_data_applyFilter)
void
swfdec_bitmap_data_applyFilter (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.applyFilter");
}

SWFDEC_AS_NATIVE (1100, 6, swfdec_bitmap_data_scroll)
void
swfdec_bitmap_data_scroll (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.scroll");
}

SWFDEC_AS_NATIVE (1100, 7, swfdec_bitmap_data_threshold)
void
swfdec_bitmap_data_threshold (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.threshold");
}

SWFDEC_AS_NATIVE (1100, 8, swfdec_bitmap_data_draw)
void
swfdec_bitmap_data_draw (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsObject *o, *matrix = NULL, *trans = NULL;
  SwfdecColorTransform ctrans;
  SwfdecBitmapData *bitmap;
  SwfdecRenderer *renderer;
  SwfdecRectangle area;
  cairo_matrix_t mat;
  cairo_t *cr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "o|OO", &o, &matrix, &trans);

  if (bitmap->surface == NULL)
    return;

  if (argc >= 2) {
    if (matrix == NULL || !swfdec_matrix_from_as_object (&mat, matrix))
      return;
  } else {
    cairo_matrix_init_identity (&mat);
  }
  if (trans && SWFDEC_IS_COLOR_TRANSFORM_AS (trans->relay)) {
    swfdec_color_transform_get_transform (SWFDEC_COLOR_TRANSFORM_AS (trans->relay), &ctrans);
  } else {
    swfdec_color_transform_init_identity (&ctrans);
  }

  if (argc > 3) {
    SWFDEC_FIXME ("only the first 3 arguments to Bitmap.draw() are implemented");
  }
  /* FIXME: compute area from arguments */
  area.x = 0;
  area.y = 0;
  area.width = bitmap->width;
  area.height = bitmap->height;

  cr = cairo_create (bitmap->surface);
  /* FIXME: Do we have a better renderer? */
  renderer = SWFDEC_PLAYER (cx)->priv->renderer;
  swfdec_renderer_attach (renderer, cr);
  cairo_transform (cr, &mat);

  if (SWFDEC_IS_BITMAP_DATA (o->relay)) {
    cairo_pattern_t *pattern = swfdec_bitmap_data_get_pattern (
	SWFDEC_BITMAP_DATA (o->relay), renderer, &ctrans);
    if (pattern) {
      cairo_set_source (cr, pattern);
      cairo_paint (cr);
      cairo_pattern_destroy (pattern);
    }
  } else if (o->movie) {
    SwfdecMovie *movie = SWFDEC_MOVIE (o->relay);
    swfdec_movie_update (movie);
    cairo_scale (cr, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR);
    cairo_transform (cr, &movie->inverse_matrix);
    swfdec_movie_render (movie, cr, &ctrans);
  } else {
    SWFDEC_FIXME ("BitmapData.draw() with a %s?", G_OBJECT_TYPE_NAME (o));
  }

  cairo_destroy (cr);
  swfdec_bitmap_data_invalidate (bitmap, area.x, area.y, area.width, area.height);
}

SWFDEC_AS_NATIVE (1100, 9, swfdec_bitmap_data_pixelDissolve)
void
swfdec_bitmap_data_pixelDissolve (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.pixelDissolve");
}

SWFDEC_AS_NATIVE (1100, 10, swfdec_bitmap_data_getPixel32)
void
swfdec_bitmap_data_getPixel32 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  guint x, y, color;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "ii", &x, &y);

  if (bitmap->surface == NULL || x >= bitmap->width || y >= bitmap->height)
    return;

  color = swfdec_bitmap_data_get_pixel (bitmap, x, y);
  color = SWFDEC_COLOR_UNMULTIPLY (color);
  *ret = swfdec_as_value_from_integer (cx, color);
}

SWFDEC_AS_NATIVE (1100, 11, swfdec_bitmap_data_setPixel32)
void
swfdec_bitmap_data_setPixel32 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  guint x, y, color;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "iii", &x, &y, &color);

  if (bitmap->surface == NULL || x >= bitmap->width || y >= bitmap->height)
    return;

  if (swfdec_surface_has_alpha (bitmap->surface)) {
    color = SWFDEC_COLOR_MULTIPLY ((SwfdecColor) color);
  } else {
    color = SWFDEC_COLOR_OPAQUE ((SwfdecColor) color);
  }
  swfdec_bitmap_data_set_pixel (bitmap, x, y, color);
}

SWFDEC_AS_NATIVE (1100, 12, swfdec_bitmap_data_floodFill)
void
swfdec_bitmap_data_floodFill (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.floodFill");
}

SWFDEC_AS_NATIVE (1100, 13, swfdec_bitmap_data_getColorBoundsRect)
void
swfdec_bitmap_data_getColorBoundsRect (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.getColorBoundsRect");
}

SWFDEC_AS_NATIVE (1100, 14, swfdec_bitmap_data_perlinNoise)
void
swfdec_bitmap_data_perlinNoise (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.perlinNoise");
}

SWFDEC_AS_NATIVE (1100, 15, swfdec_bitmap_data_colorTransform)
void
swfdec_bitmap_data_colorTransform (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  SwfdecAsObject *rect, *trans;
  SwfdecRectangle area;
  SwfdecColorTransform ctrans;
  cairo_surface_t *surface;
  cairo_t *cr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "oO", &rect, &trans);

  if (bitmap->surface == NULL)
    return;

  if (!swfdec_rectangle_from_as_object (&area, rect))
    return;
  if (trans &&  SWFDEC_IS_COLOR_TRANSFORM_AS (trans->relay))
    swfdec_color_transform_get_transform (SWFDEC_COLOR_TRANSFORM_AS (trans->relay), &ctrans);
  else
    return;

  if (area.x < 0) {
    area.width += area.x;
    area.x = 0;
  } else if ((guint) area.x >= bitmap->width) {
    return;
  }
  if (area.y < 0) {
    area.height += area.y;
    area.y = 0;
  } else if ((guint) area.y >= bitmap->height) {
    return;
  }
  if (area.width + area.x > (int) bitmap->width) {
    area.width = bitmap->width - area.x;
  } else if (area.width <= 0) {
    return;
  }
  if (area.height + area.y > (int) bitmap->height) {
    area.height = bitmap->height - area.y;
  } else if (area.height <= 0) {
    return;
  }

  surface = swfdec_renderer_transform (SWFDEC_PLAYER (cx)->priv->renderer,
      bitmap->surface, &ctrans, &area);
  cr = cairo_create (bitmap->surface);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

SWFDEC_AS_NATIVE (1100, 16, swfdec_bitmap_data_hitTest)
void
swfdec_bitmap_data_hitTest (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.hitTest");
}

SWFDEC_AS_NATIVE (1100, 17, swfdec_bitmap_data_paletteMap)
void
swfdec_bitmap_data_paletteMap (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.paletteMap");
}

SWFDEC_AS_NATIVE (1100, 18, swfdec_bitmap_data_merge)
void
swfdec_bitmap_data_merge (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.merge");
}

SWFDEC_AS_NATIVE (1100, 19, swfdec_bitmap_data_noise)
void
swfdec_bitmap_data_noise (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.noise");
}

SWFDEC_AS_NATIVE (1100, 20, swfdec_bitmap_data_copyChannel)
void
swfdec_bitmap_data_copyChannel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.copyChannel");
}

SWFDEC_AS_NATIVE (1100, 21, swfdec_bitmap_data_clone)
void
swfdec_bitmap_data_clone (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap, *clone;
  cairo_t *cr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  if (bitmap->surface == NULL)
    return;

  clone = swfdec_bitmap_data_new (cx, swfdec_surface_has_alpha (bitmap->surface),
      bitmap->width, bitmap->height);
  if (clone == NULL)
    return;

  cr = cairo_create (clone->surface);
  cairo_set_source_surface (cr, bitmap->surface, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  SWFDEC_AS_VALUE_SET_OBJECT (ret, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (clone)));
}

SWFDEC_AS_NATIVE (1100, 22, swfdec_bitmap_data_do_dispose)
void
swfdec_bitmap_data_do_dispose (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  swfdec_bitmap_data_clear (bitmap);
}

SWFDEC_AS_NATIVE (1100, 23, swfdec_bitmap_data_generateFilterRect)
void
swfdec_bitmap_data_generateFilterRect (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.generateFilterRect");
}

SWFDEC_AS_NATIVE (1100, 24, swfdec_bitmap_data_compare)
void
swfdec_bitmap_data_compare (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.compare");
}

SWFDEC_AS_NATIVE (1100, 0, swfdec_bitmap_data_construct)
void
swfdec_bitmap_data_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  int w, h;
  gboolean transparent = TRUE;
  guint color = 0;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  SWFDEC_AS_CHECK (0, NULL, "ii|bi", 
      &w, &h, &transparent, &color);
  
  if (w > 2880 || w <= 0 || h > 2880 || h <= 0) {
    SWFDEC_FIXME ("the constructor should return undefined here");
    return;
  }

  if (!swfdec_as_context_try_use_mem (cx, w * h * 4))
    return;
  bitmap = g_object_new (SWFDEC_TYPE_BITMAP_DATA, "context", cx, NULL);
  bitmap->width = w;
  bitmap->height = h;
  bitmap->surface = cairo_image_surface_create (
      transparent ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, w, h);

  if (color) {
    cairo_t *cr = cairo_create (bitmap->surface);
    swfdec_color_set_source (cr, transparent ? color : SWFDEC_COLOR_OPAQUE (color));
    cairo_paint (cr);
    cairo_destroy (cr);
  }
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (bitmap));
  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}

/*** PUBLIC API ***/

guint
swfdec_bitmap_data_get_width (SwfdecBitmapData *bitmap)
{
  g_return_val_if_fail (SWFDEC_IS_BITMAP_DATA (bitmap), 0);

  return bitmap->surface ? bitmap->width : 0;
}

guint
swfdec_bitmap_data_get_height (SwfdecBitmapData *bitmap)
{
  g_return_val_if_fail (SWFDEC_IS_BITMAP_DATA (bitmap), 0);

  return bitmap->surface ? bitmap->height : 0;
}

cairo_pattern_t *
swfdec_bitmap_data_get_pattern (SwfdecBitmapData *bitmap, SwfdecRenderer *renderer,
    const SwfdecColorTransform *ctrans)
{
  cairo_pattern_t *pattern;

  g_return_val_if_fail (SWFDEC_IS_BITMAP_DATA (bitmap), NULL);
  g_return_val_if_fail (SWFDEC_IS_RENDERER (renderer), NULL);
  g_return_val_if_fail (ctrans != NULL, NULL);
  g_return_val_if_fail (!swfdec_color_transform_is_mask (ctrans), NULL);

  /* FIXME: Is this correct for the case where the surface is NULL?
   * Do we want a red surface */
  if (bitmap->surface == NULL)
    return NULL;

  if (swfdec_color_transform_is_identity (ctrans)) {
    pattern = cairo_pattern_create_for_surface (bitmap->surface);
  } else {
    /* FIXME: do caching? */
    SwfdecRectangle area = { 0, 0, bitmap->width, bitmap->height };
    cairo_surface_t *surface = swfdec_renderer_transform (renderer,
	bitmap->surface, ctrans, &area);
    SWFDEC_FIXME ("unmodified pixels will be treated as -1, not as 0 as in our "
	"transform code, but we don't know if a pixel is unmodified.");
    pattern = cairo_pattern_create_for_surface (surface);
    cairo_surface_destroy (surface);
  }

  return pattern;
}

SwfdecColor
swfdec_bitmap_data_get_pixel (SwfdecBitmapData *bitmap, guint x, guint y)
{
  guint8 *addr;

  g_return_val_if_fail (SWFDEC_IS_BITMAP_DATA (bitmap), 0);
  g_return_val_if_fail (x < bitmap->width, 0);
  g_return_val_if_fail (y < bitmap->height, 0);

  addr = cairo_image_surface_get_data (bitmap->surface);
  addr += cairo_image_surface_get_stride (bitmap->surface) * y;
  addr += 4 * x;
  return *(guint32 *) (gpointer) addr;
}

void
swfdec_bitmap_data_set_pixel (SwfdecBitmapData *bitmap, guint x, guint y, SwfdecColor color)
{
  guint8 *addr;

  g_return_if_fail (SWFDEC_IS_BITMAP_DATA (bitmap));
  g_return_if_fail (x < bitmap->width);
  g_return_if_fail (y < bitmap->height);

  addr = cairo_image_surface_get_data (bitmap->surface);
  addr += cairo_image_surface_get_stride (bitmap->surface) * y;
  addr += 4 * x;
  *(guint32 *) (gpointer) addr = color;
  swfdec_bitmap_data_invalidate (bitmap, x, y, 1, 1);
}

