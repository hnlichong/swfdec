#include "swfdec_internal.h"
#include "swfdec_js.h"
#include "swfdec_movieclip.h"

static JSBool
swfdec_js_trace (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecDecoder *dec = JS_GetContextPrivate (cx);
  char *bytes;
  JSString *string;

  string = JS_ValueToString(cx, argv[0]);
  if (string == NULL)
    return JS_FALSE;

  bytes = JS_GetStringBytes (string);
  if (bytes == NULL)
    return JS_FALSE;

  /* FIXME: accumulate and emit after JS handling? */
  g_signal_emit_by_name (dec, "trace", bytes);
  return JS_TRUE;
}

static JSBool
swfdec_js_random (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  gint32 max, result;

  if (!JS_ValueToECMAInt32 (cx, argv[0], &max))
    return JS_FALSE;
  
  result = g_random_int_range (0, max);

  return JS_NewNumberValue(cx, result, rval);
}

static JSBool
swfdec_js_startDrag (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_TRUE;
}

static JSFunctionSpec global_methods[] = {
  { "random",		swfdec_js_random,	1, 0, 0 },
  { "startDrag",     	swfdec_js_startDrag,	1, 0, 0 },
  { "trace",     	swfdec_js_trace,	1, 0, 0 },
  { NULL, NULL, 0, 0, 0 }
};

void
swfdec_js_add_globals (SwfdecDecoder *dec, JSObject *global)
{
  if (!JS_DefineFunctions (dec->jscx, global, global_methods)) {
    SWFDEC_ERROR ("failed to initialize global methods");
  }
}

