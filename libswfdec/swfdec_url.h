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

#include <libswfdec/swfdec_bits.h>
#include <libswfdec/swfdec_as_types.h>
#include <libswfdec/swfdec_player.h>
#include <libswfdec/swfdec_types.h>

#ifndef _SWFDEC_URL_H_
#define _SWFDEC_URL_H_

G_BEGIN_DECLS

typedef struct _SwfdecURL SwfdecURL;

#define SWFDEC_TYPE_URL swfdec_url_get_type()
GType			swfdec_url_get_type		(void) G_GNUC_CONST;

SwfdecURL *		swfdec_url_new			(const char *		string);
SwfdecURL *		swfdec_url_new_relative	      	(const SwfdecURL *	url,
							 const char *		string);
SwfdecURL *		swfdec_url_copy			(const SwfdecURL *      url);
void			swfdec_url_free			(SwfdecURL *		url);

const char *		swfdec_url_get_url		(const SwfdecURL *      url);
const char *		swfdec_url_get_protocol		(const SwfdecURL *      url);
const char *		swfdec_url_get_host		(const SwfdecURL *      url);
const char *		swfdec_url_get_path		(const SwfdecURL *      url);
const char *		swfdec_url_get_query		(const SwfdecURL *      url);
							 

G_END_DECLS

#endif