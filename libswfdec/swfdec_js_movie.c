/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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
#include <math.h>

#include <js/jsapi.h>
#include "swfdec_js.h"
#include "swfdec_movie.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_player_internal.h"
#include "swfdec_root_movie.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"

JSBool swfdec_js_eval (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static void
movie_finalize (JSContext *cx, JSObject *obj)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate (cx, obj);
  /* since we also finalize the class, not everyone has a private object */
  if (movie) {
    g_assert (movie->jsobj != NULL);

    SWFDEC_LOG ("destroying JSObject %p for movie %p", obj, movie);
    movie->jsobj = NULL;
    g_object_unref (movie);
  } else {
    SWFDEC_LOG ("destroying JSObject %p", obj);
  }
}

static JSClass movieclip_class = {
    "MovieClip", JSCLASS_NEW_RESOLVE | JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   movie_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
mc_play (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  movie->stopped = FALSE;

  return JS_TRUE;
}

static JSBool
mc_stop (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  movie->stopped = TRUE;

  return JS_TRUE;
}

static JSBool
mc_getBytesLoaded (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;
  SwfdecDecoder *dec;

  movie = JS_GetPrivate(cx, obj);
  dec = SWFDEC_ROOT_MOVIE (movie->root)->decoder;

  *rval = INT_TO_JSVAL(MIN (dec->bytes_loaded, dec->bytes_total));

  return JS_TRUE;
}

static JSBool
mc_getBytesTotal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;
  SwfdecDecoder *dec;

  movie = JS_GetPrivate(cx, obj);
  dec = SWFDEC_ROOT_MOVIE (movie->root)->decoder;

  *rval = INT_TO_JSVAL (dec->bytes_total);

  return JS_TRUE;
}

static JSBool
mc_do_goto (JSContext *cx, SwfdecMovie *movie, jsval target)
{
  int32 frame;

  if (JSVAL_IS_STRING (target)) {
    const char *label = swfdec_js_to_string (cx, target);
    frame = swfdec_sprite_get_frame (SWFDEC_SPRITE_MOVIE (movie)->sprite, label);
    /* FIXME: nonexisting frames? */
    if (frame == -1)
      return JS_FALSE;
    frame++;
  } else if (!JS_ValueToInt32 (cx, target, &frame)) {
    return JS_FALSE;
  }
  /* FIXME: how to handle overflow? */
  frame = CLAMP (frame, 1, (int) movie->n_frames) - 1;

  swfdec_movie_goto (movie, frame);
  return JS_TRUE;
}

static JSBool
mc_gotoAndPlay (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  
  if (!mc_do_goto (cx, movie, argv[0]))
    return JS_FALSE;
  movie->stopped = FALSE;
  return JS_TRUE;
}

static JSBool
mc_gotoAndStop (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  
  if (!mc_do_goto (cx, movie, argv[0]))
    return JS_FALSE;
  movie->stopped = TRUE;
  return JS_TRUE;
}

static JSBool
swfdec_js_nextFrame (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;
  jsval frame;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  
  frame = INT_TO_JSVAL (movie->frame + 2); /* 1-indexed */
  if (!mc_do_goto (cx, movie, frame))
    return JS_FALSE;
  movie->stopped = TRUE;
  return JS_TRUE;
}

static JSBool
swfdec_js_prevFrame (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;
  jsval frame;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  
  if (movie->frame == 0)
    frame = INT_TO_JSVAL (movie->n_frames);
  else
    frame = INT_TO_JSVAL (movie->frame); /* 1-indexed */
  if (!mc_do_goto (cx, movie, frame))
    return JS_FALSE;
  movie->stopped = TRUE;
  return JS_TRUE;
}

static JSBool
mc_hitTest (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  
  if (argc == 1) {
    SwfdecMovie *other;
    if (!JSVAL_IS_OBJECT (argv[0]) ||
	JS_GetClass (JSVAL_TO_OBJECT (argv[0])) != &movieclip_class) {
      g_assert_not_reached ();
      return JS_FALSE;
    }
    other = SWFDEC_MOVIE (JS_GetPrivate(cx, JSVAL_TO_OBJECT (argv[0])));
    swfdec_movie_update (movie);
    swfdec_movie_update (other);
    /* FIXME */
    g_assert (movie->parent == other->parent);
#if 0
    g_print ("%g %g  %g %g --- %g %g  %g %g\n", 
	SWFDEC_OBJECT (movie)->extents.x0, SWFDEC_OBJECT (movie)->extents.y0,
	SWFDEC_OBJECT (movie)->extents.x1, SWFDEC_OBJECT (movie)->extents.y1,
	SWFDEC_OBJECT (other)->extents.x0, SWFDEC_OBJECT (other)->extents.y0,
	SWFDEC_OBJECT (other)->extents.x1, SWFDEC_OBJECT (other)->extents.y1);
#endif
    if (swfdec_rect_intersect (NULL, &movie->extents, &other->extents)) {
      *rval = BOOLEAN_TO_JSVAL (JS_TRUE);
    } else {
      *rval = BOOLEAN_TO_JSVAL (JS_FALSE);
    }
  } else if (argc == 3) {
    g_assert_not_reached ();
  } else {
    return JS_FALSE;
  }

  return JS_TRUE;
}

/* FIXME: replace with eval */
static SwfdecMovie *
get_target (SwfdecMovie *movie, const char *target, JSBool case_sensitive)
{
  char *tmp;
  guint len;
  GList *walk;

  //g_print ("get_target: %s\n", target);
  if (target[0] == '\0')
    return movie;

  if (g_str_has_prefix (target, "../")) {
    if (movie->parent == NULL)
      return NULL;
    return get_target (movie->parent, target + 3, case_sensitive);
  }
  tmp = strchr (target, '/');
  if (tmp)
    len = tmp - target;
  else
    len = strlen (target);

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (cur->content->name) {
      if ((case_sensitive && strncmp (cur->content->name, target, len) == 0) || 
	  (!case_sensitive && g_ascii_strncasecmp (cur->content->name, target, len) == 0)) {
	if (target[len] == '\0')
	  return cur;
	else 
	  return get_target (cur, target + len + 1, case_sensitive);
      }
    }
  }
  return NULL;
}

extern  JSPropertySpec movieclip_props[];

static JSBool
swfdec_js_getProperty (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  uint32 id;
  SwfdecMovie *movie;

  if (JSVAL_IS_OBJECT (argv[0])) {
    movie = JS_GetPrivate(cx, JSVAL_TO_OBJECT (argv[0]));
  } else if (JSVAL_IS_STRING (argv[0])) {
    char *str = JS_GetStringBytes (JSVAL_TO_STRING (argv[0]));
    movie = JS_GetPrivate(cx, obj);
    movie = get_target (movie, str, JS_GetContextCaseSensitive (cx));
    if (movie == NULL) {
      SWFDEC_INFO ("no target name \"%s\"", str);
      return JS_FALSE;
    }
  } else {
    return JS_FALSE;
  }
  if (!JS_ValueToECMAUint32 (cx, argv[1], &id))
    return JS_FALSE;

  if (id > 19)
    return JS_FALSE;

  if (movie->jsobj == NULL &&
      !swfdec_js_add_movie (movie))
    return JS_FALSE;
  return movieclip_props[id].getter (cx, movie->jsobj, INT_TO_JSVAL (id) /* FIXME */, rval);
}

static JSBool
swfdec_js_setProperty (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  uint32 id;
  SwfdecMovie *movie;

  if (JSVAL_IS_OBJECT (argv[0])) {
    movie = JS_GetPrivate(cx, JSVAL_TO_OBJECT (argv[0]));
  } else if (JSVAL_IS_STRING (argv[0])) {
    char *str = JS_GetStringBytes (JSVAL_TO_STRING (argv[0]));
    movie = JS_GetPrivate(cx, obj);
    movie = get_target (movie, str, JS_GetContextCaseSensitive (cx));
    if (movie == NULL) {
      SWFDEC_INFO ("no target name \"%s\"", str);
      return JS_FALSE;
    }
  } else {
    return JS_FALSE;
  }
  if (!JS_ValueToECMAUint32 (cx, argv[1], &id))
    return JS_FALSE;

  if (id > 19)
    return JS_FALSE;

  if (movie->jsobj == NULL &&
      !swfdec_js_add_movie (movie))
    return JS_FALSE;
  *rval = argv[2];
  return movieclip_props[id].setter (cx, movie->jsobj, INT_TO_JSVAL (id) /* FIXME */, rval);
}

static JSBool
swfdec_js_startDrag (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;
  JSBool center = JS_FALSE;
  SwfdecRect rect;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);
  if (argc > 0) {
    if (!JS_ValueToBoolean (cx, argv[0], &center))
      return JS_FALSE;
  }
  if (argc >= 5) {
    if (!JS_ValueToNumber (cx, argv[1], &rect.x0) || 
        !JS_ValueToNumber (cx, argv[2], &rect.y0) || 
        !JS_ValueToNumber (cx, argv[3], &rect.x1) || 
        !JS_ValueToNumber (cx, argv[4], &rect.y1))
      return JS_FALSE;
    swfdec_rect_scale (&rect, &rect, SWFDEC_SCALE_FACTOR);
    swfdec_player_set_drag_movie (SWFDEC_ROOT_MOVIE (movie->root)->player, movie,
	center, &rect);
  } else {
    swfdec_player_set_drag_movie (SWFDEC_ROOT_MOVIE (movie->root)->player, movie,
	center, NULL);
  }
  
  return JS_TRUE;
}

static JSBool
swfdec_js_stopDrag (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;
  SwfdecPlayer *player;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);
  player = SWFDEC_ROOT_MOVIE (movie->root)->player;
  swfdec_player_set_drag_movie (player, NULL, FALSE, NULL);
  return JS_TRUE;
}

static JSBool
swfdec_js_getURL (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  const char *url;
  const char *target;
  SwfdecMovie *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);
  url = swfdec_js_to_string (cx, argv[0]);
  if (!url)
    return FALSE;
  if (argc > 1) {
    target = swfdec_js_to_string (cx, argv[1]);
    if (!target)
      return JS_FALSE;
  } else {
    /* FIXME: figure out default target */
    g_assert_not_reached ();
  }
  if (argc > 2) {
    /* variables not implemented yet */
    g_assert_not_reached ();
  }
  swfdec_root_movie_load (SWFDEC_ROOT_MOVIE (movie->root), url, target);
  return JS_TRUE;
}

static GString *
get_name (SwfdecMovie *movie)
{
  GString *s;

  if (movie->parent) {
    s = get_name (movie->parent);
    g_string_append_c (s, '.');
    g_string_append (s, movie->name);
  } else {
    s = g_string_new (movie->name);
  }
  return s;
}

static JSBool
swfdec_js_movie_to_string (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  GString *s;
  JSString *string;
  SwfdecMovie *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  s = get_name (movie);
  string = JS_NewStringCopyZ (cx, s->str);
  g_string_free (s, TRUE);
  if (string == NULL)
    return JS_FALSE;
  *rval = STRING_TO_JSVAL (string);
  return JS_TRUE;
}

static JSFunctionSpec movieclip_methods[] = {
  //{"attachMovie", mc_attachMovie, 4, 0},
  { "eval",		swfdec_js_eval,			1, 0, 0 },
  { "getBytesLoaded",	mc_getBytesLoaded,		0, 0, 0 },
  { "getBytesTotal",	mc_getBytesTotal,		0, 0, 0 },
  { "getProperty",    	swfdec_js_getProperty,		2, 0, 0 },
  { "getURL",    	swfdec_js_getURL,		2, 0, 0 },
  { "gotoAndPlay",	mc_gotoAndPlay,			1, 0, 0 },
  { "gotoAndStop",	mc_gotoAndStop,			1, 0, 0 },
  { "nextFrame",	swfdec_js_nextFrame,	      	0, 0, 0 },
  { "play",		mc_play,			0, 0, 0 },
  { "prevFrame",	swfdec_js_prevFrame,	      	0, 0, 0 },
  { "stop",		mc_stop,			0, 0, 0 },
  { "hitTest",		mc_hitTest,			1, 0, 0 },
  { "setProperty",    	swfdec_js_setProperty,		3, 0, 0 },
  { "startDrag",    	swfdec_js_startDrag,		0, 0, 0 },
  { "stopDrag",    	swfdec_js_stopDrag,		0, 0, 0 },
  { "toString",	  	swfdec_js_movie_to_string,	0, 0, 0 },
  { NULL }
};

#if 0
static JSBool
mc_attachMovie(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
    jsval *rval)
{
  SwfdecMovie *parent_movie, *child_movie;
  SwfdecSprite *parent_sprite, *attach_sprite;
  JSString *idName, *newName;
  int depth;
  JSObject *initObject, *child_mc;
  SwfdecActionContext *context;

  context = JS_GetContextPrivate (cx);

  idName = JS_ValueToString (cx, argv[0]);
  newName = JS_ValueToString (cx, argv[1]);
  JS_ValueToInt32 (cx, argv[2], &depth);
  JS_ValueToObject (cx, argv[3], &initObject);

  SWFDEC_DEBUG("placing sprite %s as %s at depth %d",
      JS_GetStringBytes (idName), JS_GetStringBytes (newName), depth);

  attach_sprite = SWFDEC_SPRITE(swfdec_exports_lookup (context->s,
    JS_GetStringBytes(idName)));
  if (!attach_sprite) {
    SWFDEC_WARNING("Couldn't find sprite %s", JS_GetStringBytes (idName));
    *rval = JSVAL_VOID;
    return JS_TRUE;
  }

  parent_movie = JS_GetPrivate (cx, obj);
  if (!parent_movie) {
    SWFDEC_WARNING("couldn't get moviement");
    *rval = JSVAL_VOID;
    return JS_TRUE;
  }
  if (parent_movie->id == 0)
    parent_sprite = context->s->main_sprite;
  else
    parent_sprite = SWFDEC_SPRITE(swfdec_object_get (context->s,
        parent_movie->id));

  swfdec_sprite_frame_remove_movie (context->s, &parent_sprite->frames[parent_movie->first_index],
      depth);

  /* FIXME we need a separate list of added moviements */
  child_movie = swfdec_spritemovie_new ();
  child_movie->depth = depth;
  cairo_matrix_init_identity (&child_movie->transform);
  swfdec_color_transform_init_identity (&child_movie->color_transform);

  swfdec_sprite_frame_add_movie (&parent_sprite->frames[parent_movie->first_index],
      child_movie);
  child_mc = movieclip_new (context, child_movie);
  *rval = OBJECT_TO_JSVAL(child_mc);

  JS_SetProperty (cx, obj, JS_GetStringBytes(newName), rval);

  return JS_TRUE;
}
#endif

static JSBool
mc_x_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  swfdec_movie_update (movie);
  d = (movie->x + movie->original_extents.x0) / SWFDEC_SCALE_FACTOR;
  return JS_NewNumberValue (cx, d, vp);
}

static JSBool
mc_x_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  if (!JS_ValueToNumber (cx, *vp, &d))
    return JS_FALSE;
  if (!finite (d)) {
    SWFDEC_WARNING ("trying to move x to a non-finite value, ignoring");
    return JS_TRUE;
  }
  movie->x = d * SWFDEC_SCALE_FACTOR - movie->original_extents.x0;
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);

  return JS_TRUE;
}

static JSBool
mc_y_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  swfdec_movie_update (movie);
  d = (movie->y + movie->original_extents.y0) / SWFDEC_SCALE_FACTOR;
  return JS_NewNumberValue (cx, d, vp);
}

static JSBool
mc_y_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  if (!JS_ValueToNumber (cx, *vp, &d))
    return JS_FALSE;
  if (!finite (d)) {
    SWFDEC_WARNING ("trying to move y to a non-finite value, ignoring");
    return JS_TRUE;
  }
  movie->y = d * SWFDEC_SCALE_FACTOR - movie->original_extents.y0;
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);

  return JS_TRUE;
}

static JSBool
mc_xscale_get (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  d = movie->xscale * 100;
  return JS_NewNumberValue (cx, d, vp);
}

static JSBool
mc_xscale_set (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  if (!JS_ValueToNumber (cx, *vp, &d))
    return JS_FALSE;
  if (!finite (d)) {
    SWFDEC_WARNING ("trying to set xscale to a non-finite value, ignoring");
    return JS_TRUE;
  }
  movie->xscale = d / 100;
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);

  return JS_TRUE;
}

static JSBool
mc_currentframe (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  *vp = INT_TO_JSVAL (movie->frame + 1);

  return JS_TRUE;
}

static JSBool
mc_framesloaded (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;
  guint loaded;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  /* only root movies can be partially loaded */
  if (SWFDEC_IS_ROOT_MOVIE (movie) || 
      SWFDEC_IS_ROOT_MOVIE (movie->parent)) {
    SwfdecDecoder *dec = SWFDEC_ROOT_MOVIE (movie->root)->decoder;
    loaded = dec->frames_loaded;
    g_assert (loaded <= movie->n_frames);
  } else {
    loaded = movie->n_frames;
  }
  *vp = INT_TO_JSVAL (loaded);

  return JS_TRUE;
}

static JSBool
mc_totalframes (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  *vp = INT_TO_JSVAL (movie->n_frames);

  return JS_TRUE;
}

static JSBool
mc_parent (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  /* FIXME: what do we do if we're the root movie? */
  if (movie->parent)
    movie = movie->parent;

  if (movie->jsobj == NULL)
    swfdec_js_add_movie (movie);
  if (movie->jsobj == NULL)
    return JS_FALSE;

  *vp = OBJECT_TO_JSVAL (movie->jsobj);

  return JS_TRUE;
}

static JSBool
mc_root (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  movie = movie->root;
  if (movie->jsobj == NULL) {
    /* the root movie only holds this as long as there's no parent */
    movie = movie->list->data;
    if (movie->jsobj == NULL)
      swfdec_js_add_movie (movie);
  }
  if (movie->jsobj == NULL)
    return JS_FALSE;

  *vp = OBJECT_TO_JSVAL (movie->jsobj);

  return JS_TRUE;
}

/* Movie AS standard class */

enum {
  PROP_X,
  PROP_Y,
  PROP_XSCALE,
  PROP_YSCALE,
  PROP_CURRENTFRAME,
  PROP_TOTALFRAMES,
  PROP_ALPHA,
  PROP_VISIBLE,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_ROTATION,
  PROP_FRAMESLOADED,
  PROP_NAME,
  PROP_DROPTARGET,
  PROP_URL,
  PROP_HIGHQUALITY,
  PROP_FOCUSRECT,
  PROP_SOUNDBUFTIME,
  PROP_QUALITY,
  PROP_XMOUSE,
  PROP_YMOUSE
};

static JSBool
not_reached (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  const char *str = swfdec_js_to_string (cx, id);
  SWFDEC_ERROR ("reading and writing property %s is not implemented", str);
  return JS_TRUE;
}

/* NB: order needs to be kept for GetProperty/SetProperty actions */
#define MC_PROP_ATTRS (JSPROP_PERMANENT|JSPROP_SHARED)
JSPropertySpec movieclip_props[] = {
  {"_x",	    -1,		MC_PROP_ATTRS,			  mc_x_get,	    mc_x_set },
  {"_y",	    -1,		MC_PROP_ATTRS,			  mc_y_get,	    mc_y_set },
  {"_xscale",	    -1,		MC_PROP_ATTRS,			  mc_xscale_get,    mc_xscale_set },
  {"_yscale",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_currentframe", -1,		MC_PROP_ATTRS | JSPROP_READONLY,  mc_currentframe,  NULL },
  {"_totalframes",  -1,		MC_PROP_ATTRS | JSPROP_READONLY,  mc_totalframes,   NULL },
  {"_alpha",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_visble",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_width",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_height",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_rotation",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_framesloaded", -1,		MC_PROP_ATTRS | JSPROP_READONLY,  mc_framesloaded,  NULL },
  {"_name",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_droptarget",   -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_url",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_highquality",  -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_focusrect",    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_soundbuftime", -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_xmouse",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_ymouse",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_parent",	    -1,	      	MC_PROP_ATTRS | JSPROP_READONLY,  mc_parent,	    NULL},
  {"_root",	    -1,	      	MC_PROP_ATTRS | JSPROP_READONLY,  mc_root,	    NULL},
  {NULL}
};

#if 0
JSObject *
movieclip_find (SwfdecActionContext *context,
    SwfdecMovie *movie)
{
  GList *g;
  struct mc_list_entry *listentry;

  for (g = g_list_first (context->movielist); g; g = g_list_next (g)) {
    listentry = (struct mc_list_entry *)g->data;

    if (listentry->movie == movie)
      return listentry->mc;
  }

  return NULL;
}

static void
swfdec_native_ASSetPropFlags (SwfdecActionContext *context, int num_args,
  ActionVal *_this)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;
  ActionVal *d;
  int allowFalse = 0;
  int flags;

  a = stack_pop (context); /* obj */
  action_val_convert_to_object (a);
  b = stack_pop (context); /* property list */
  c = stack_pop (context); /* flags */
  action_val_convert_to_number (c);
  if (num_args >= 4) {
    d = stack_pop (context); /* allowFalse */
    action_val_convert_to_boolean (d);
    allowFalse = d->number;
    action_val_free (d);
  }

  flags = (int)c->number & 0x7;
  /* The flags appear to be 0x1 for DontEnum, 0x2 for DontDelete, and 0x4 for
   * DontWrite, though the tables I found on the web are poorly written.
   */

  if (ACTIONVAL_IS_NULL(b)) {
    GList *g;

    SWFDEC_DEBUG("%d args", num_args);

    for (g = g_list_first (a->obj->properties); g; g = g_list_next (g)) {
      ScriptObjectProperty *prop = g->data;
      if (allowFalse) {
        prop->flags = flags;
      } else {
        prop->flags |= flags;
      }
    }
  } else {
    action_val_convert_to_string (b);
    SWFDEC_WARNING("ASSetPropFlags not implemented (properties %s, flags 0x%x)",
      b->string, (int)c->number);
  }

  action_val_free (a);
  action_val_free (b);
  action_val_free (c);

  a = action_val_new ();
  a->type = ACTIONVAL_TYPE_UNDEF;
  stack_push (context, a);
}
#endif

static JSBool
swfdec_js_movieclip_new (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SWFDEC_ERROR ("This should not exist, but currently has to for instanceof to work");
  return JS_FALSE;
}

/**
 * swfdec_js_add_movieclip_class:
 * @player: a @SwfdecPlayer
 *
 * Adds the movieclip class to the JS Context of @player.
 **/
void
swfdec_js_add_movieclip_class (SwfdecPlayer *player)
{
  JS_InitClass (player->jscx, player->jsobj, NULL,
      &movieclip_class, swfdec_js_movieclip_new, 0, movieclip_props, movieclip_methods,
      NULL, NULL);
}

void
swfdec_js_movie_add_property (SwfdecMovie *movie)
{
  jsval val;
  JSObject *jsobj;

  if (movie->jsobj == NULL) {
    if (!swfdec_js_add_movie (movie))
      return;
  }
  val = OBJECT_TO_JSVAL (movie->jsobj);
  if (movie->parent) {
    jsobj = movie->parent->jsobj;
    if (jsobj == NULL)
      return;
    SWFDEC_LOG ("setting %s as property for %s", movie->name, 
	movie->parent->name);
  } else {
    jsobj = SWFDEC_ROOT_MOVIE (movie->root)->player->jsobj;
    SWFDEC_LOG ("setting %s as property for _global", movie->name);
  }
  JS_SetProperty (SWFDEC_ROOT_MOVIE (movie->root)->player->jscx, 
      jsobj, movie->name, &val);
}

void
swfdec_js_movie_remove_property (SwfdecMovie *movie)
{
  JSObject *jsobj;

  if (movie->jsobj == NULL)
    return;

  if (movie->parent) {
    jsobj = movie->parent->jsobj;
    if (jsobj == NULL)
      return;
  } else {
    jsobj = SWFDEC_ROOT_MOVIE (movie->root)->player->jsobj;
  }

  SWFDEC_LOG ("removing %s as property", movie->name);
  JS_DeleteProperty (SWFDEC_ROOT_MOVIE (movie->root)->player->jscx, 
      jsobj, movie->name);
}

/**
 * swfdec_js_add_movie:
 * @movie: a #SwfdecMovie
 *
 * Ensures that a JSObject for the given @movie exists.
 **/
gboolean
swfdec_js_add_movie (SwfdecMovie *movie)
{
  JSContext *cx;
  GList *walk;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), FALSE);
  g_return_val_if_fail (movie->jsobj == NULL, FALSE);

  cx = SWFDEC_ROOT_MOVIE (movie->root)->player->jscx;

  movie->jsobj = JS_NewObject (cx, &movieclip_class, NULL, NULL);
  if (movie->jsobj == NULL) {
    SWFDEC_ERROR ("failed to create JS object for movie %p", movie);
    return FALSE;
  }
  SWFDEC_LOG ("created JSObject %p for movie %p", movie->jsobj, movie);
  g_object_ref (movie);
  JS_SetPrivate (cx, movie->jsobj, movie);
  /* add all children */
  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *child = walk->data;
    if (child->has_name)
      swfdec_js_movie_add_property (child);
  }
  return TRUE;
}

#if 0
void
action_register_sprite_movie (SwfdecDecoder * s, SwfdecMovie *movie)
{
  SwfdecActionContext *context;
  JSObject *mc;
  JSBool ok;
  jsval val;

  SWFDEC_DEBUG ("Placing Movie %s", movie->name ? movie->name : "(no name)");

  if (s->context == NULL)
    swfdec_init_context (s);
  context = s->context;
#if SWFDEC_ACTIONS_DEBUG_GC
  JS_GC(context->jscx);
#endif

  mc = movieclip_new (context, movie);
  val = OBJECT_TO_JSVAL(mc);

  if (movie->name) {
    JSObject *parentclip;
    char *parentname;

    parentclip = movieclip_find (context, s->parse_sprite_movie);
    parentname = name_object (context, parentclip);
    SWFDEC_INFO("%s is a child of %s", movie->name, parentname);
    g_free (parentname);

    /* FIXME: This helps sbemail out a bit, but I'm guessing it's wrong.  There
     * are still some scope issues, it seems -- for example, a clip is created
     * while parsing _root, but is then accessed as a member of another movie
     * clip which is also a child of _root.
     */
    ok = JS_SetProperty (context->jscx, context->global, movie->name, &val);
    ok &= JS_SetProperty (context->jscx, parentclip, movie->name, &val);
    if (!ok)
      SWFDEC_WARNING("Failed to register %s", movie->name);
  }
}
#endif
