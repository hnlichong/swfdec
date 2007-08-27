/* Swfdec
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

#include <string.h>

#include "swfdec_xml_node.h"
#include "swfdec_as_array.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_load_object.h"

G_DEFINE_TYPE (SwfdecXmlNode, swfdec_xml_node, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_xml_node_class_init (SwfdecXmlNodeClass *klass)
{
}

static void
swfdec_xml_node_init (SwfdecXmlNode *xml_node)
{
}

static int
swfdec_xml_node_get_nodeType (SwfdecAsObject *object)
{
  SwfdecAsValue val;

  swfdec_as_object_get_variable (object, SWFDEC_AS_STR_nodeType, &val);
  return swfdec_as_value_to_integer (object->context, &val);
}

static SwfdecAsObject*
swfdec_xml_node_get_attributes (SwfdecXmlNode *node)
{
  SwfdecAsValue val;

  g_assert (SWFDEC_IS_XML_NODE (node));

  swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (node),
      SWFDEC_AS_STR_attributes, &val);
  return swfdec_as_value_to_object (SWFDEC_AS_OBJECT (node)->context, &val);
}

static SwfdecAsArray*
swfdec_xml_node_get_childNodes (SwfdecXmlNode *node)
{
  SwfdecAsValue val;
  SwfdecAsObject *childNodes;

  g_assert (SWFDEC_IS_XML_NODE (node));

  swfdec_as_object_get_variable (SWFDEC_AS_OBJECT(node),
      SWFDEC_AS_STR_childNodes, &val);
  childNodes = swfdec_as_value_to_object (
      SWFDEC_AS_OBJECT(node)->context, &val);

  g_assert (SWFDEC_IS_AS_ARRAY (childNodes));

  return SWFDEC_AS_ARRAY (childNodes);
}

static SwfdecXmlNode*
swfdec_xml_node_get_child (SwfdecXmlNode *node, gint32 i)
{
  SwfdecAsArray *childNodes;
  SwfdecAsObject *child;
  SwfdecAsValue val;
  const char *var;

  g_return_val_if_fail (SWFDEC_IS_XML_NODE (node), NULL);
  g_return_val_if_fail (i >= 0, NULL);

  childNodes = swfdec_xml_node_get_childNodes (node);
  var = swfdec_as_double_to_string (SWFDEC_AS_OBJECT(node)->context, i);
  swfdec_as_object_get_variable (SWFDEC_AS_OBJECT(node), var, &val);
  child = swfdec_as_value_to_object (SWFDEC_AS_OBJECT(node)->context, &val);

  if (!SWFDEC_IS_XML_NODE (child))
    return NULL;

  return SWFDEC_XML_NODE (child);
}

typedef struct {
  const char	character;
  const char	*escaped;
} EntityConversion;

static EntityConversion xml_entities[] = {
  { '&', "&amp;" },
  { '"', "&quot;" },
  { '\'', "&apos;" },
  { '<', "&lt;" },
  { '>', "&gt;" },
  { '\0', NULL }
};

static char *
swfdec_xml_node_escape (const char *orginal)
{
  int i;
  const char *p, *start;
  GString *string;

  string = g_string_new ("");

  p = start = orginal;
  while (*(p += strcspn (p, "&<>\"'")) != '\0') {
    string = g_string_append_len (string, start, p - start);
    for (i = 0; xml_entities[i].escaped != NULL; i++) {
      if (xml_entities[i].character == *p) {
	string = g_string_append (string, xml_entities[i].escaped);
	break;
      }
    }
    g_assert (xml_entities[i].escaped != NULL);

    p++;
    start = p;
  }
  string = g_string_append (string, start);

  return g_string_free (string, FALSE);
}

SWFDEC_AS_NATIVE (253, 1, swfdec_xml_node_cloneNode)
void
swfdec_xml_node_cloneNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_FIXME ("XMLNode.cloneNode not implemented");
}

void
swfdec_xml_node_removeNode (SwfdecXmlNode *node)
{
  SWFDEC_FIXME ("XMLNode.removeNode not implemented");
}

SWFDEC_AS_NATIVE (253, 2, swfdec_xml_node_do_removeNode)
void
swfdec_xml_node_do_removeNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  swfdec_xml_node_removeNode (SWFDEC_XML_NODE (object));
}

SWFDEC_AS_NATIVE (253, 3, swfdec_xml_node_insertBefore)
void
swfdec_xml_node_insertBefore (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_FIXME ("XMLNode.insertBefore not implemented");
}

void
swfdec_xml_node_appendChild (SwfdecXmlNode *node, SwfdecXmlNode *child)
{
  SwfdecAsValue val;
  SwfdecAsArray *childNodes;

  g_return_if_fail (SWFDEC_IS_XML_NODE (node));
  g_return_if_fail (SWFDEC_IS_XML_NODE (child));

  // remove the previous parent of the child
  swfdec_xml_node_removeNode (child);

  // set child in the childNodes array, to the lastChild property
  // and depending on position to either firstChild property or another child's
  // nextSibling property
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (child));

  childNodes = swfdec_xml_node_get_childNodes (node);
  swfdec_as_array_push (childNodes, &val);

  swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (node),
      SWFDEC_AS_STR_lastChild, &val);
  if (swfdec_as_array_length (childNodes) == 1) {
    swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (node),
	SWFDEC_AS_STR_firstChild, &val);
  } else {
    SwfdecXmlNode *previous = swfdec_xml_node_get_child (node,
	swfdec_as_array_length (childNodes) - 1);
    g_assert (previous != NULL);
    swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (previous),
	SWFDEC_AS_STR_nextSibling, &val);
    // set the previousSibling property of the child
    SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (previous));
    swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (child),
	SWFDEC_AS_STR_previousSibling, &val);
  }

  // set node as parent of child
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (node));
  swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (child),
      SWFDEC_AS_STR_parentNode, &val);
}

SWFDEC_AS_NATIVE (253, 4, swfdec_xml_node_do_appendChild)
void
swfdec_xml_node_do_appendChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsObject *child;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]))
    return;

  child = SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]);
  if (!SWFDEC_IS_XML_NODE (child))
    return;

  swfdec_xml_node_appendChild (SWFDEC_XML_NODE (object),
      SWFDEC_XML_NODE (child));
}

SWFDEC_AS_NATIVE (253, 5, swfdec_xml_node_hasChildNodes)
void
swfdec_xml_node_hasChildNodes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsValue val;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (
	swfdec_xml_node_get_childNodes (SWFDEC_XML_NODE (object))),
      SWFDEC_AS_STR_length, &val);
  if (swfdec_as_value_to_integer (object->context, &val) > 0) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
  }
}

static gboolean
swfdec_xml_node_foreach_string_append_attribute (SwfdecAsObject *object,
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data)
{
  GString *string = *(GString **)data;
  char *escaped;

  string = g_string_append (string, " ");
  string = g_string_append (string, variable);
  string = g_string_append (string, "=\"");
  escaped =
    swfdec_xml_node_escape (swfdec_as_value_to_string (object->context, value));
  string = g_string_append (string, escaped);
  g_free (escaped);
  string = g_string_append (string, "\"");

  return TRUE;
}

SWFDEC_AS_NATIVE (253, 6, swfdec_xml_node_toString)
void
swfdec_xml_node_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;
  SwfdecAsValue val;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  node = SWFDEC_XML_NODE (object);

  switch (swfdec_xml_node_get_nodeType (object)) {
    case SWFDEC_XML_NODE_ELEMENT:
      {
	SwfdecAsObject *attributes;
	SwfdecAsArray *childNodes;
	SwfdecXmlNode *child;
	GString *string;
	gint32 i, length;

	string = g_string_new ("<");
	swfdec_as_object_get_variable (object, SWFDEC_AS_STR_nodeName, &val);
	string = g_string_append (string,
	    swfdec_as_value_to_string (object->context, &val));

	attributes = swfdec_xml_node_get_attributes (node);
	swfdec_as_object_foreach (attributes,
	    swfdec_xml_node_foreach_string_append_attribute, &string);

	childNodes = swfdec_xml_node_get_childNodes (node);
	length = swfdec_as_array_length (childNodes);

	if (length > 0) {
	  string = g_string_append (string, ">");

	  for (i = 0; i < length; i++) {
	    child = swfdec_xml_node_get_child (node, i);
	    g_assert (child != NULL);
	    swfdec_as_object_call (SWFDEC_AS_OBJECT (child),
		SWFDEC_AS_STR_toString, 0, NULL, &val);
	    string = g_string_append (string, swfdec_as_value_to_string (
		  SWFDEC_AS_OBJECT (child)->context, &val));
	  }

	  string = g_string_append (string, "</");
	  swfdec_as_object_get_variable (object, SWFDEC_AS_STR_nodeName, &val);
	  string = g_string_append (string,
	      swfdec_as_value_to_string (object->context, &val));
	  string = g_string_append (string, ">");
	} else {
	  string = g_string_append (string, " />");
	}

	SWFDEC_AS_VALUE_SET_STRING (ret,
	    swfdec_as_context_give_string (object->context,
	      g_string_free (string, FALSE)));
	break;
      }
    case SWFDEC_XML_NODE_TEXT:
    default:
      swfdec_as_object_get_variable (object, SWFDEC_AS_STR_nodeValue, ret);
      break;
  }
}

static void
swfdec_xml_node_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  if (argc >= 2) {
    int type;
    const char *value, *p;
    SwfdecAsValue val;
    SwfdecAsObject *attributes;
    SwfdecAsObject *childNodes;

    SWFDEC_AS_VALUE_SET_NULL (&val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_namespaceURI, &val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_localName, &val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_prefix, &val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_previousSibling, &val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_parentNode, &val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_nodeValue, &val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_nodeName, &val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_nextSibling, &val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_lastChild, &val);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_firstChild, &val);

    // FIXME need to get _global.Array
    childNodes = swfdec_as_array_new (object->context);
    SWFDEC_AS_VALUE_SET_OBJECT (&val, childNodes);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_childNodes, &val);

    attributes = swfdec_as_object_new_empty (object->context);
    SWFDEC_AS_VALUE_SET_OBJECT (&val, attributes);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_attributes, &val);

    type = swfdec_as_value_to_integer (object->context, &argv[0]);
    SWFDEC_AS_VALUE_SET_INT (&val, type);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_nodeType, &val);

    switch (type) {
      case SWFDEC_XML_NODE_ELEMENT:
	value = swfdec_as_value_to_string (object->context, &argv[1]);

	SWFDEC_AS_VALUE_SET_STRING (&val, value);
	swfdec_as_object_set_variable (object, SWFDEC_AS_STR_nodeName, &val);

	p = strchr (value, ':');
	if (p != NULL) {
	  char *prefix = g_strndup (value, p - value);
	  char *localName = g_strdup (p + 1);

	  SWFDEC_AS_VALUE_SET_STRING (&val,
	      swfdec_as_context_give_string (object->context, prefix));
	  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_prefix, &val);
	  SWFDEC_AS_VALUE_SET_STRING (&val,
	      swfdec_as_context_give_string (object->context, localName));
	  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_localName, &val);
	} else {
	  SWFDEC_AS_VALUE_SET_STRING (&val, SWFDEC_AS_STR_EMPTY);
	  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_prefix, &val);
	  SWFDEC_AS_VALUE_SET_STRING (&val, value);
	  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_localName, &val);
	}
	break;
      case SWFDEC_XML_NODE_TEXT:
      default:
	value = swfdec_as_value_to_string (object->context, &argv[1]);

	SWFDEC_AS_VALUE_SET_STRING (&val, value);
	swfdec_as_object_set_variable (object, SWFDEC_AS_STR_nodeValue, &val);
	break;
    }
  }

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}

/**
 * swfdec_xml_node_new:
 * @player: a #SwfdecPlayer
 * @type: the #SwfdecXmlNodeType
 * @value: initial value of the node
 *
 * Creates a new #SwfdecXmlNode.
 *
 * Returns: The new XML node or %NULL on OOM
 **/
SwfdecXmlNode *
swfdec_xml_node_new (SwfdecAsContext *context, SwfdecXmlNodeType type,
    const char* value)
{
  SwfdecXmlNode *node;
  guint size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  size = sizeof (SwfdecXmlNode);
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  node = g_object_new (SWFDEC_TYPE_XML_NODE, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (node), context, size);

  return node;
}

void
swfdec_xml_node_init_context (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context;
  SwfdecAsObject *xml_node, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  context = SWFDEC_AS_CONTEXT (player);
  xml_node = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_XMLNode, 0, swfdec_xml_node_construct, 0));
  if (!xml_node)
    return;
  swfdec_as_native_function_set_construct_type (
      SWFDEC_AS_NATIVE_FUNCTION (xml_node), SWFDEC_TYPE_XML_NODE);

  proto = swfdec_as_object_new (context);

  /* set the right properties on the XmlNode object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (xml_node, SWFDEC_AS_STR_prototype, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
  swfdec_as_object_set_variable (xml_node, SWFDEC_AS_STR_constructor, &val);

  /* set the right properties on the XmlNode.prototype object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR___proto__, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, xml_node);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_cloneNode,
      SWFDEC_TYPE_XML_NODE, swfdec_xml_node_cloneNode, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_removeNode,
      SWFDEC_TYPE_XML_NODE, swfdec_xml_node_do_removeNode, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_insertBefore, 0,
      swfdec_xml_node_insertBefore, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_appendChild, 0,
      swfdec_xml_node_do_appendChild, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_hasChildNodes, 0,
      swfdec_xml_node_hasChildNodes, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toString, 0,
      swfdec_xml_node_toString, 0);
}
