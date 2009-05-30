/*
 *      artist.h
 *      
 *      Copyright 2009 Brett Mravec <brett.mravec@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifndef __ARTIST_H__
#define __ARTIST_H__

#include <glib-object.h>

#define ARTIST_TYPE (artist_get_type ())
#define ARTIST(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ARTIST_TYPE, Artist))
#define ARTIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), ARTIST_TYPE, ArtistClass))
#define IS_ARTIST(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), ARTIST_TYPE))
#define IS_ARTIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ARTIST_TYPE))
#define ARTIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), ARTIST_TYPE, ArtistClass))

G_BEGIN_DECLS

typedef struct _Artist Artist;
typedef struct _ArtistClass ArtistClass;
typedef struct _ArtistPrivate ArtistPrivate;

struct _Artist {
    GObject parent;
    
    ArtistPrivate *priv;
};

struct _ArtistClass {
    GObjectClass parent;
};

Artist *artist_new ();
GType artist_get_type (void);

G_END_DECLS

#endif /* __ARTIST_H__ */

