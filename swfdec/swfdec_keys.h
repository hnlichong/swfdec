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

#ifndef _SWFDEC_KEYS_H_
#define _SWFDEC_KEYS_H_

G_BEGIN_DECLS

typedef enum {
  SWFDEC_KEY_BACKSPACE = 8,
  SWFDEC_KEY_TAB = 9,
  SWFDEC_KEY_CLEAR = 12,
  SWFDEC_KEY_ENTER = 13,
  SWFDEC_KEY_SHIFT = 16,
  SWFDEC_KEY_CONTROL = 17,
  SWFDEC_KEY_ALT = 18,
  SWFDEC_KEY_CAPS_LOCK = 20,
  SWFDEC_KEY_ESCAPE = 27,
  SWFDEC_KEY_SPACE = 32,
  SWFDEC_KEY_PAGE_UP = 33,
  SWFDEC_KEY_PAGE_DOWN = 34,
  SWFDEC_KEY_END = 35,
  SWFDEC_KEY_HOME = 36,
  SWFDEC_KEY_LEFT = 37,
  SWFDEC_KEY_UP = 38,
  SWFDEC_KEY_RIGHT = 39,
  SWFDEC_KEY_DOWN = 40,
  SWFDEC_KEY_INSERT = 45,
  SWFDEC_KEY_DELETE = 46,
  SWFDEC_KEY_HELP = 47,
  SWFDEC_KEY_0 = 48,
  SWFDEC_KEY_1 = 49,
  SWFDEC_KEY_2 = 50,
  SWFDEC_KEY_3 = 51,
  SWFDEC_KEY_4 = 52,
  SWFDEC_KEY_5 = 53,
  SWFDEC_KEY_6 = 54,
  SWFDEC_KEY_7 = 55,
  SWFDEC_KEY_8 = 56,
  SWFDEC_KEY_9 = 57,
  SWFDEC_KEY_A = 65,
  SWFDEC_KEY_B = 66,
  SWFDEC_KEY_C = 67,
  SWFDEC_KEY_D = 68,
  SWFDEC_KEY_E = 69,
  SWFDEC_KEY_F = 70,
  SWFDEC_KEY_G = 71,
  SWFDEC_KEY_H = 72,
  SWFDEC_KEY_I = 73,
  SWFDEC_KEY_J = 74,
  SWFDEC_KEY_K = 75,
  SWFDEC_KEY_L = 76,
  SWFDEC_KEY_M = 77,
  SWFDEC_KEY_N = 78,
  SWFDEC_KEY_O = 79,
  SWFDEC_KEY_P = 80,
  SWFDEC_KEY_Q = 81,
  SWFDEC_KEY_R = 82,
  SWFDEC_KEY_S = 83,
  SWFDEC_KEY_T = 84,
  SWFDEC_KEY_U = 85,
  SWFDEC_KEY_V = 86,
  SWFDEC_KEY_W = 87,
  SWFDEC_KEY_X = 88,
  SWFDEC_KEY_Y = 89,
  SWFDEC_KEY_Z = 90,
  SWFDEC_KEY_NUMPAD_0 = 96,
  SWFDEC_KEY_NUMPAD_1 = 97,
  SWFDEC_KEY_NUMPAD_2 = 98,
  SWFDEC_KEY_NUMPAD_3 = 99,
  SWFDEC_KEY_NUMPAD_4 = 100,
  SWFDEC_KEY_NUMPAD_5 = 101,
  SWFDEC_KEY_NUMPAD_6 = 102,
  SWFDEC_KEY_NUMPAD_7 = 103,
  SWFDEC_KEY_NUMPAD_8 = 104,
  SWFDEC_KEY_NUMPAD_9 = 105,
  SWFDEC_KEY_NUMPAD_MULTIPLY = 106,
  SWFDEC_KEY_NUMPAD_ADD = 107,
  SWFDEC_KEY_NUMPAD_SUBTRACT = 109,
  SWFDEC_KEY_NUMPAD_DECIMAL = 110,
  SWFDEC_KEY_NUMPAD_DIVIDE = 111,
  SWFDEC_KEY_F1 = 112,
  SWFDEC_KEY_F2 = 113,
  SWFDEC_KEY_F3 = 114,
  SWFDEC_KEY_F4 = 115,
  SWFDEC_KEY_F5 = 116,
  SWFDEC_KEY_F6 = 117,
  SWFDEC_KEY_F7 = 118,
  SWFDEC_KEY_F8 = 119,
  SWFDEC_KEY_F9 = 120,
  SWFDEC_KEY_F10 = 121,
  SWFDEC_KEY_F11 = 122,
  SWFDEC_KEY_F12 = 123,
  SWFDEC_KEY_F13 = 124,
  SWFDEC_KEY_F14 = 125,
  SWFDEC_KEY_F15 = 126,
  SWFDEC_KEY_NUM_LOCK = 144,
  SWFDEC_KEY_SEMICOLON = 186,
  SWFDEC_KEY_EQUAL = 187,
  SWFDEC_KEY_COMMA = 188,
  SWFDEC_KEY_MINUS = 189,
  SWFDEC_KEY_DOT = 190,
  SWFDEC_KEY_SLASH = 191,
  SWFDEC_KEY_GRAVE = 192,
  SWFDEC_KEY_LEFT_BRACKET = 219,
  SWFDEC_KEY_BACKSLASH = 220,
  SWFDEC_KEY_RIGHT_BRACKET = 221,
  SWFDEC_KEY_APOSTROPHE = 222,
} SwfdecKey;

G_END_DECLS
#endif
