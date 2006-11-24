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

#ifndef _SWFDEC_AUDIO_INTERNAL_H_
#define _SWFDEC_AUDIO_INTERNAL_H_

#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_audio.h>
#include <libswfdec/swfdec_types.h>

G_BEGIN_DECLS

#define SWFDEC_AUDIO_OUT_STEREO 256
#define SWFDEC_AUDIO_OUT_GET(channels, rate) \
  (((channels) == 2 ? SWFDEC_AUDIO_OUT_STEREO : 0) | 44100 / (rate))
#define SWFDEC_AUDIO_OUT_IS_STEREO(out) ((out) & SWFDEC_AUDIO_OUT_STEREO)
#define SWFDEC_AUDIO_OUT_N_CHANNELS(out) (SWFDEC_AUDIO_OUT_IS_STEREO (out) ? 2 : 1)
#define SWFDEC_AUDIO_OUT_GRANULARITY(out) ((out) & 0xFF)
#define SWFDEC_AUDIO_OUT_RATE(out) (44100 / SWFDEC_AUDIO_OUT_GRANULARITY (out))
typedef enum {
  SWFDEC_AUDIO_OUT_MONO_44100 = 1,
  SWFDEC_AUDIO_OUT_MONO_22050 = 2,
  SWFDEC_AUDIO_OUT_MONO_11025 = 4,
  SWFDEC_AUDIO_OUT_MONO_5512 = 8,
  SWFDEC_AUDIO_OUT_STEREO_44100 = SWFDEC_AUDIO_OUT_MONO_44100 | SWFDEC_AUDIO_OUT_STEREO,
  SWFDEC_AUDIO_OUT_STEREO_22050 = SWFDEC_AUDIO_OUT_MONO_22050 | SWFDEC_AUDIO_OUT_STEREO,
  SWFDEC_AUDIO_OUT_STEREO_11025 = SWFDEC_AUDIO_OUT_MONO_11025 | SWFDEC_AUDIO_OUT_STEREO,
  SWFDEC_AUDIO_OUT_STEREO_5512 = SWFDEC_AUDIO_OUT_MONO_5512 | SWFDEC_AUDIO_OUT_STEREO,
} SwfdecAudioOut;

struct _SwfdecAudio {
  GObject		object;

  SwfdecPlayer *	player;		/* the player that plays us */
  guint			start_offset;	/* offset from player in number of samples */
};

struct _SwfdecAudioClass {
  GObjectClass		object_class;

  guint			(* iterate)		(SwfdecAudio *	audio,
						 guint		n_samples);
  void			(* render)		(SwfdecAudio *	audio,
						 gint16 *	dest,
						 guint		start, 
						 guint		n_samples);
};

SwfdecAudio *	swfdec_audio_new		(SwfdecPlayer *	player,
						 GType		type);
void		swfdec_audio_remove		(SwfdecAudio *	audio);

guint		swfdec_audio_iterate		(SwfdecAudio *	audio,
						 guint		n_samples);

void		swfdec_player_iterate_audio   	(SwfdecPlayer *	player);

G_END_DECLS
#endif
