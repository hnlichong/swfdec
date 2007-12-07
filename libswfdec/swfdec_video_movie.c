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

#include "swfdec_video_movie.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecVideoMovie, swfdec_video_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_video_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  SwfdecVideoMovie *video = SWFDEC_VIDEO_MOVIE (movie);
  SwfdecRect rect = { 0, 0, 
    SWFDEC_TWIPS_SCALE_FACTOR * video->video->width, 
    SWFDEC_TWIPS_SCALE_FACTOR * video->video->height };
  
  swfdec_rect_union (extents, extents, &rect);
}

static void
swfdec_video_movie_update_image (SwfdecVideoMovie *movie)
{
  if (!movie->needs_update)
    return;

  if (movie->input != NULL) {
    if (movie->input->set_ratio)
      movie->input->set_ratio (movie->input, movie);
    
    if (movie->image)
      cairo_surface_destroy (movie->image);
    movie->image = movie->input->get_image (movie->input);
    if (movie->image)
      cairo_surface_reference (movie->image);
  }
  movie->needs_update = FALSE;
}

static void
swfdec_video_movie_render (SwfdecMovie *mov, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  SwfdecVideoMovie *movie = SWFDEC_VIDEO_MOVIE (mov);

  swfdec_video_movie_update_image (movie);
  if (movie->image == NULL)
    return;

  cairo_scale (cr, 
      (mov->original_extents.x1 - mov->original_extents.x0)
      / cairo_image_surface_get_width (movie->image),
      (mov->original_extents.y1 - mov->original_extents.y0)
      / cairo_image_surface_get_height (movie->image));
  cairo_set_source_surface (cr, movie->image, 0.0, 0.0);
  cairo_paint (cr);
}

static void
swfdec_video_movie_unset_input (SwfdecVideoMovie *movie)
{
  if (movie->input == NULL)
    return;

  if (movie->input->disconnect)
    movie->input->disconnect (movie->input, movie);
  movie->input = NULL;
  swfdec_video_movie_update_image (movie);
}

static void
swfdec_video_movie_dispose (GObject *object)
{
  SwfdecVideoMovie *movie = SWFDEC_VIDEO_MOVIE (object);

  swfdec_video_movie_unset_input (movie);
  g_object_unref (movie->video);
  if (movie->image) {
    cairo_surface_destroy (movie->image);
    movie->image = NULL;
  }

  G_OBJECT_CLASS (swfdec_video_movie_parent_class)->dispose (object);
}

static void
swfdec_video_movie_set_ratio (SwfdecMovie *movie)
{
  SwfdecVideoMovie *video = SWFDEC_VIDEO_MOVIE (movie);

  if (video->input->set_ratio) {
    video->needs_update = TRUE;
    swfdec_movie_invalidate_last (movie);
  }
}

static void
swfdec_video_movie_init_movie (SwfdecMovie *movie)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);

  swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (movie), player->Video);
}

static void
swfdec_video_movie_class_init (SwfdecVideoMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_video_movie_dispose;

  movie_class->update_extents = swfdec_video_movie_update_extents;
  movie_class->render = swfdec_video_movie_render;
  movie_class->init_movie = swfdec_video_movie_init_movie;
  movie_class->set_ratio = swfdec_video_movie_set_ratio;
}

static void
swfdec_video_movie_init (SwfdecVideoMovie * video_movie)
{
}

void
swfdec_video_movie_set_input (SwfdecVideoMovie *movie, SwfdecVideoMovieInput *input)
{
  g_return_if_fail (SWFDEC_IS_VIDEO_MOVIE (movie));

  swfdec_video_movie_unset_input (movie);
  movie->input = input;
  if (input == NULL)
    return;
  if (movie->input->set_ratio) {
    swfdec_movie_invalidate_last (SWFDEC_MOVIE (movie));
  }
  movie->needs_update = TRUE;
  if (input->connect)
    input->connect (input, movie);
}

void
swfdec_video_movie_clear (SwfdecVideoMovie *movie)
{
  g_return_if_fail (SWFDEC_IS_VIDEO_MOVIE (movie));

  if (movie->image) {
    cairo_surface_destroy (movie->image);
    movie->image = NULL;
  }
  swfdec_movie_invalidate_last (SWFDEC_MOVIE (movie));
}

void
swfdec_video_movie_new_image (SwfdecVideoMovie *movie)
{
  g_return_if_fail (SWFDEC_IS_VIDEO_MOVIE (movie));

  if (movie->image) {
    cairo_surface_destroy (movie->image);
    movie->image = NULL;
  }
  movie->needs_update = TRUE;
  swfdec_movie_invalidate_last (SWFDEC_MOVIE (movie));
}

