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

#define SWFDEC_AS_CONSTANT_STRING(str) "\2" str "\0"
const char swfdec_as_strings[] = 
  SWFDEC_AS_CONSTANT_STRING ("")
  SWFDEC_AS_CONSTANT_STRING ("__proto__")
  SWFDEC_AS_CONSTANT_STRING ("this")
  SWFDEC_AS_CONSTANT_STRING ("code")
  SWFDEC_AS_CONSTANT_STRING ("level")
  SWFDEC_AS_CONSTANT_STRING ("description")
  SWFDEC_AS_CONSTANT_STRING ("status")
  SWFDEC_AS_CONSTANT_STRING ("success")
  SWFDEC_AS_CONSTANT_STRING ("NetConnection.Connect.Success")
  SWFDEC_AS_CONSTANT_STRING ("onLoad")
  SWFDEC_AS_CONSTANT_STRING ("onEnterFrame")
  SWFDEC_AS_CONSTANT_STRING ("onUnload")
  SWFDEC_AS_CONSTANT_STRING ("onMouseMove")
  SWFDEC_AS_CONSTANT_STRING ("onMouseDown")
  SWFDEC_AS_CONSTANT_STRING ("onMouseUp")
  SWFDEC_AS_CONSTANT_STRING ("onKeyUp")
  SWFDEC_AS_CONSTANT_STRING ("onKeyDown")
  SWFDEC_AS_CONSTANT_STRING ("onData")
  SWFDEC_AS_CONSTANT_STRING ("onPress")
  SWFDEC_AS_CONSTANT_STRING ("onRelease")
  SWFDEC_AS_CONSTANT_STRING ("onReleaseOutside")
  SWFDEC_AS_CONSTANT_STRING ("onRollOver")
  SWFDEC_AS_CONSTANT_STRING ("onRollOut")
  SWFDEC_AS_CONSTANT_STRING ("onDragOver")
  SWFDEC_AS_CONSTANT_STRING ("onDragOut")
  SWFDEC_AS_CONSTANT_STRING ("onConstruct")
  SWFDEC_AS_CONSTANT_STRING ("onStatus")
  SWFDEC_AS_CONSTANT_STRING ("error")
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Buffer.Empty")
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Buffer.Full")
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Buffer.Flush")
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Play.Start")
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Play.Stop")
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Play.StreamNotFound")
  SWFDEC_AS_CONSTANT_STRING ("undefined")
  SWFDEC_AS_CONSTANT_STRING ("null")
  SWFDEC_AS_CONSTANT_STRING ("[object Object]")
  SWFDEC_AS_CONSTANT_STRING ("true")
  SWFDEC_AS_CONSTANT_STRING ("false")
  SWFDEC_AS_CONSTANT_STRING ("_x")
  SWFDEC_AS_CONSTANT_STRING ("_y")
  SWFDEC_AS_CONSTANT_STRING ("_xscale")
  SWFDEC_AS_CONSTANT_STRING ("_yscale")
  SWFDEC_AS_CONSTANT_STRING ("_currentframe")
  SWFDEC_AS_CONSTANT_STRING ("_totalframes")
  SWFDEC_AS_CONSTANT_STRING ("_alpha")
  SWFDEC_AS_CONSTANT_STRING ("_visible")
  SWFDEC_AS_CONSTANT_STRING ("_width")
  SWFDEC_AS_CONSTANT_STRING ("_height")
  SWFDEC_AS_CONSTANT_STRING ("_rotation") 
  SWFDEC_AS_CONSTANT_STRING ("_target")
  SWFDEC_AS_CONSTANT_STRING ("_framesloaded")
  SWFDEC_AS_CONSTANT_STRING ("_name") 
  SWFDEC_AS_CONSTANT_STRING ("_droptarget")
  SWFDEC_AS_CONSTANT_STRING ("_url") 
  SWFDEC_AS_CONSTANT_STRING ("_highquality") 
  SWFDEC_AS_CONSTANT_STRING ("_focusrect") 
  SWFDEC_AS_CONSTANT_STRING ("_soundbuftime") 
  SWFDEC_AS_CONSTANT_STRING ("_quality")
  SWFDEC_AS_CONSTANT_STRING ("_xmouse") 
  SWFDEC_AS_CONSTANT_STRING ("_ymouse")
  SWFDEC_AS_CONSTANT_STRING ("_parent")
  SWFDEC_AS_CONSTANT_STRING ("_root")
  SWFDEC_AS_CONSTANT_STRING ("#ERROR#")
  SWFDEC_AS_CONSTANT_STRING ("number")
  SWFDEC_AS_CONSTANT_STRING ("boolean")
  SWFDEC_AS_CONSTANT_STRING ("string")
  SWFDEC_AS_CONSTANT_STRING ("movieclip")
  SWFDEC_AS_CONSTANT_STRING ("function")
  SWFDEC_AS_CONSTANT_STRING ("object")
  SWFDEC_AS_CONSTANT_STRING ("toString")
  SWFDEC_AS_CONSTANT_STRING ("valueOf")
  SWFDEC_AS_CONSTANT_STRING ("Function")
  SWFDEC_AS_CONSTANT_STRING ("prototype")
  SWFDEC_AS_CONSTANT_STRING ("constructor")
  SWFDEC_AS_CONSTANT_STRING ("Object")
  SWFDEC_AS_CONSTANT_STRING ("hasOwnProperty")
  SWFDEC_AS_CONSTANT_STRING ("NUMERIC")
  SWFDEC_AS_CONSTANT_STRING ("RETURNINDEXEDARRAY")
  SWFDEC_AS_CONSTANT_STRING ("UNIQUESORT")
  SWFDEC_AS_CONSTANT_STRING ("DESCENDING")
  SWFDEC_AS_CONSTANT_STRING ("CASEINSENSITIVE")
  SWFDEC_AS_CONSTANT_STRING ("Array")
  SWFDEC_AS_CONSTANT_STRING ("ASSetPropFlags")
  SWFDEC_AS_CONSTANT_STRING ("0")
  SWFDEC_AS_CONSTANT_STRING ("-Infinity")
  SWFDEC_AS_CONSTANT_STRING ("Infinity")
  SWFDEC_AS_CONSTANT_STRING ("NaN")
  SWFDEC_AS_CONSTANT_STRING ("Number")
  SWFDEC_AS_CONSTANT_STRING ("NAN")
  SWFDEC_AS_CONSTANT_STRING ("MAX_VALUE")
  SWFDEC_AS_CONSTANT_STRING ("MIN_VALUE")
  SWFDEC_AS_CONSTANT_STRING ("NEGATIVE_INFINITY")
  SWFDEC_AS_CONSTANT_STRING ("POSITIVE_INFINITY")
  SWFDEC_AS_CONSTANT_STRING ("[type Object]")
  SWFDEC_AS_CONSTANT_STRING ("startDrag")
  SWFDEC_AS_CONSTANT_STRING ("Mouse")
  SWFDEC_AS_CONSTANT_STRING ("hide")
  SWFDEC_AS_CONSTANT_STRING ("show")
  SWFDEC_AS_CONSTANT_STRING ("addListener")
  SWFDEC_AS_CONSTANT_STRING ("removeListener")
  SWFDEC_AS_CONSTANT_STRING ("MovieClip")
  SWFDEC_AS_CONSTANT_STRING ("attachMovie")
  SWFDEC_AS_CONSTANT_STRING ("duplicateMovieClip")
  SWFDEC_AS_CONSTANT_STRING ("getBytesLoaded")
  SWFDEC_AS_CONSTANT_STRING ("getBytesTotal")
  SWFDEC_AS_CONSTANT_STRING ("getDepth")
  SWFDEC_AS_CONSTANT_STRING ("getNextHighestDepth")
  SWFDEC_AS_CONSTANT_STRING ("getURL")
  SWFDEC_AS_CONSTANT_STRING ("gotoAndPlay")
  SWFDEC_AS_CONSTANT_STRING ("gotoAndStop")
  SWFDEC_AS_CONSTANT_STRING ("hitTest")
  SWFDEC_AS_CONSTANT_STRING ("nextFrame")
  SWFDEC_AS_CONSTANT_STRING ("play")
  SWFDEC_AS_CONSTANT_STRING ("prevFrame")
  SWFDEC_AS_CONSTANT_STRING ("removeMovieClip")
  SWFDEC_AS_CONSTANT_STRING ("stop")
  SWFDEC_AS_CONSTANT_STRING ("stopDrag")
  SWFDEC_AS_CONSTANT_STRING ("swapDepths")
  SWFDEC_AS_CONSTANT_STRING ("super")
  SWFDEC_AS_CONSTANT_STRING ("length")
  SWFDEC_AS_CONSTANT_STRING ("[type Function]")
  SWFDEC_AS_CONSTANT_STRING ("arguments")
  SWFDEC_AS_CONSTANT_STRING (",")
  SWFDEC_AS_CONSTANT_STRING ("registerClass")
  SWFDEC_AS_CONSTANT_STRING ("__constructor__")
  SWFDEC_AS_CONSTANT_STRING ("_global")
  SWFDEC_AS_CONSTANT_STRING ("aa")
  SWFDEC_AS_CONSTANT_STRING ("ab")
  SWFDEC_AS_CONSTANT_STRING ("ba")
  SWFDEC_AS_CONSTANT_STRING ("bb")
  SWFDEC_AS_CONSTANT_STRING ("ga")
  SWFDEC_AS_CONSTANT_STRING ("gb")
  SWFDEC_AS_CONSTANT_STRING ("ra")
  SWFDEC_AS_CONSTANT_STRING ("rb")
  SWFDEC_AS_CONSTANT_STRING ("getRGB")
  SWFDEC_AS_CONSTANT_STRING ("getTransform")
  SWFDEC_AS_CONSTANT_STRING ("setRGB")
  SWFDEC_AS_CONSTANT_STRING ("setTransform")
  SWFDEC_AS_CONSTANT_STRING ("Color")
  SWFDEC_AS_CONSTANT_STRING ("push")
  SWFDEC_AS_CONSTANT_STRING ("parseInt")
  SWFDEC_AS_CONSTANT_STRING ("Math")
  SWFDEC_AS_CONSTANT_STRING ("ceil")
  /* add more here */
;

#ifdef COMPUTE_SWFDEC_AS_STRINGS_H
/* This code creates the offset */
#include <stdio.h>
#include <string.h>

int
main (int argc, char **argv)
{
  const char *cur = swfdec_as_strings;
  char *name, *s;

  printf ("/* This is a generated file. See swfdec_as_strings.c for details. */\n");
  printf ("#ifndef _SWFDEC_AS_STRINGS_H_\n");
  printf ("#define _SWFDEC_AS_STRINGS_H_\n");
  printf ("\n");
  printf ("extern const char swfdec_as_strings[];\n");
  printf ("\n");
  while (*cur != 0) {
    cur++;
    if (!strcmp (cur, "")) {
      name = strdup ("EMPTY");
    } else if (!strcmp (cur, ",")) {
      name = strdup ("COMMA");
    } else {
      s = name = strdup (cur);
      while (*s) {
	if (!isalnum (*s))
	  *s = '_';
	s++;
      }
    }
    printf ("#define SWFDEC_AS_STR_%s &swfdec_as_strings[%u]\n", name, 
	(unsigned int) (cur - swfdec_as_strings));
    free (name);
    cur += strlen (cur) + 1;
  }
  printf ("\n");
  printf ("#endif /* _SWFDEC_AS_STRINGS_H_ */\n");
  return 0;
}
#endif
