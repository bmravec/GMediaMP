/*
 *      now-playing.h
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

#ifndef __NOW_PLAYING_H__
#define __NOW_PLAYING_H__

#include <glib-object.h>

#define NOW_PLAYING_TYPE (now_playing_get_type ())
#define NOW_PLAYING(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), NOW_PLAYING_TYPE, NowPlaying))
#define NOW_PLAYING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), NOW_PLAYING_TYPE, NowPlayingClass))
#define IS_NOW_PLAYING(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), NOW_PLAYING_TYPE))
#define IS_NOW_PLAYING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NOW_PLAYING_TYPE))
#define NOW_PLAYING_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), NOW_PLAYING_TYPE, NowPlayingClass))

G_BEGIN_DECLS

typedef struct _NowPlaying NowPlaying;
typedef struct _NowPlayingClass NowPlayingClass;
typedef struct _NowPlayingPrivate NowPlayingPrivate;

struct _NowPlaying {
    GObject parent;

    NowPlayingPrivate *priv;
};

struct _NowPlayingClass {
    GObjectClass parent;
};

NowPlaying *now_playing_new ();
GType now_playing_get_type (void);

gboolean now_playing_activate (NowPlaying *self);
gboolean now_playing_deactivate (NowPlaying *self);

G_END_DECLS

#endif /* __NOW_PLAYING_H__ */
