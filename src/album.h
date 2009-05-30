/*
 *      album.h
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

#ifndef __ALBUM_H__
#define __ALBUM_H__

#include <glib-object.h>

#define ALBUM_TYPE (album_get_type ())
#define ALBUM(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ALBUM_TYPE, Album))
#define ALBUM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), ALBUM_TYPE, AlbumClass))
#define IS_ALBUM(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), ALBUM_TYPE))
#define IS_ALBUM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ALBUM_TYPE))
#define ALBUM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), ALBUM_TYPE, AlbumClass))

G_BEGIN_DECLS

typedef struct _Album Album;
typedef struct _AlbumClass AlbumClass;
typedef struct _AlbumPrivate AlbumPrivate;

struct _Album {
    GObject parent;
    
    AlbumPrivate *priv;
};

struct _AlbumClass {
    GObjectClass parent;
};

Album *album_new ();
GType album_get_type (void);

G_END_DECLS

#endif /* __ALBUM_H__ */

