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
#include "swfdec_audio_event.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"


G_DEFINE_TYPE (SwfdecAudioEvent, swfdec_audio_event, SWFDEC_TYPE_AUDIO)

static guint
swfdec_audio_event_iterate (SwfdecAudio *audio, guint remove)
{
  SwfdecAudioEvent *event = SWFDEC_AUDIO_EVENT (audio);

  event->offset += remove;
  if (event->offset >= event->stop_sample)
    event->offset = event->stop_sample;
  return event->stop_sample - event->offset;
}

static void
swfdec_audio_event_render (SwfdecAudio *audio, gint16* dest,
    guint start, guint n_samples)
{
  SwfdecAudioEvent *event = SWFDEC_AUDIO_EVENT (audio);
  guint offset = event->offset + start;

  if (offset >= event->stop_sample)
    return;
  //n_samples = MIN (n_samples, event->stop_sample - offset);
  swfdec_sound_render (event->sound, dest, offset, n_samples);
}

static void
swfdec_audio_event_dispose (GObject *object)
{
  SwfdecAudioEvent *audio = SWFDEC_AUDIO_EVENT (object);

  g_free (audio->envelope);
  audio->envelope = NULL;
  audio->n_envelopes = 0;

  G_OBJECT_CLASS (swfdec_audio_event_parent_class)->dispose (object);
}

static void
swfdec_audio_event_class_init (SwfdecAudioEventClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAudioClass *audio_class = SWFDEC_AUDIO_CLASS (klass);

  object_class->dispose = swfdec_audio_event_dispose;

  audio_class->iterate = swfdec_audio_event_iterate;
  audio_class->render = swfdec_audio_event_render;
}

static void
swfdec_audio_event_init (SwfdecAudioEvent *audio_event)
{
}

static SwfdecAudio *
swfdec_audio_event_get (SwfdecPlayer *player, SwfdecSound *sound)
{
  GList *walk;

  if (player == NULL)
    return NULL;

  for (walk = player->audio; walk; walk = walk->next) {
    SwfdecAudio *audio = walk->data;
    if (!SWFDEC_IS_AUDIO_EVENT (audio))
      continue;
    if (SWFDEC_AUDIO_EVENT (audio)->sound == sound) {
      return audio;
    }
  }
  return NULL;
}

/**
 * swfdec_audio_event_new:
 * @player: a #SwfdecPlayer or NULL
 * @event: a sound event to start playing back
 *
 * Starts playback of the given sound event (or, when @player is NULL, creates
 * an element for playing back the given sound).
 *
 * Returns: the sound effect or NULL if no new sound was created.
 **/
SwfdecAudio *
swfdec_audio_event_new (SwfdecPlayer *player, SwfdecSoundChunk *chunk)
{
  SwfdecAudioEvent *event;

  g_return_val_if_fail (player == NULL || SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (chunk != NULL, NULL);

  if (chunk->stop) {
    SwfdecAudio *audio = swfdec_audio_event_get (player, chunk->sound);
    if (audio) {
      SWFDEC_LOG ("stopping sound %d", SWFDEC_CHARACTER (chunk->sound)->id);
      swfdec_audio_remove (audio);
    }
    return NULL;
  }
  SWFDEC_LOG ("adding sound %d to playing sounds", SWFDEC_CHARACTER (chunk->sound)->id);
  if (chunk->no_restart &&
      (event = (SwfdecAudioEvent *) swfdec_audio_event_get (player, chunk->sound))) {
    SWFDEC_DEBUG ("sound %d is already playing, reusing it", 
	SWFDEC_CHARACTER (event->sound)->id);
    g_object_ref (event);
    return SWFDEC_AUDIO (event);
  }
  event = g_object_new (SWFDEC_TYPE_AUDIO_EVENT, NULL);
  /* copy chunk data */
  event->sound = chunk->sound;
  event->start_sample = chunk->start_sample;
  event->start_sample = chunk->start_sample;
  event->loop_count = chunk->loop_count;
  event->n_envelopes = chunk->n_envelopes;
  if (event->n_envelopes)
    event->envelope = g_memdup (chunk->envelope, sizeof (SwfdecSoundEnvelope) * event->n_envelopes);
  event->offset = event->start_sample;
  SWFDEC_DEBUG ("playing sound %d from offset %d now", SWFDEC_CHARACTER (event->sound)->id,
      event->start_sample);
  swfdec_audio_add (SWFDEC_AUDIO (event), player);

  return SWFDEC_AUDIO (event);
}

