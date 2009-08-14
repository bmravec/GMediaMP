/*
 *      playlist.h
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

#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__

#include <glib-object.h>

#define PLAYLIST_TYPE (playlist_get_type ())
#define PLAYLIST(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), PLAYLIST_TYPE, Playlist))
#define PLAYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PLAYLIST_TYPE, PlaylistClass))
#define IS_PLAYLIST(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PLAYLIST_TYPE))
#define IS_PLAYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAYLIST_TYPE))
#define PLAYLIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAYLIST_TYPE, PlaylistClass))

enum {
    PLAYLIST_STATE_NONE = 0,
    PLAYLIST_STATE_PLAYING,
    PLAYLIST_STATE_MISSING
};

G_BEGIN_DECLS

typedef struct _Playlist Playlist;
typedef struct _PlaylistClass PlaylistClass;
typedef struct _PlaylistPrivate PlaylistPrivate;

struct _Playlist {
    GObject parent;

    PlaylistPrivate *priv;
};

struct _PlaylistClass {
    GObjectClass parent;
};

Playlist *playlist_new ();
GType playlist_get_type (void);

gboolean playlist_activate (Playlist *self);
gboolean playlist_deactivate (Playlist *self);

G_END_DECLS

#endif /* __PLAYLIST_H__ */
