/*
 *      music.h
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

#ifndef __MUSIC_H__
#define __MUSIC_H__

#include <glib-object.h>

#include "entry.h"

#define MUSIC_TYPE (music_get_type ())
#define MUSIC(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MUSIC_TYPE, Music))
#define MUSIC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MUSIC_TYPE, MusicClass))
#define IS_MUSIC(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUSIC_TYPE))
#define IS_MUSIC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MUSIC_TYPE))
#define MUSIC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MUSIC_TYPE, MusicClass))

G_BEGIN_DECLS

typedef struct _Music Music;
typedef struct _MusicClass MusicClass;
typedef struct _MusicPrivate MusicPrivate;

struct _Music {
    GObject parent;

    MusicPrivate *priv;
};

struct _MusicClass {
    GObjectClass parent;
};

Music *music_new ();
GType music_get_type (void);

gboolean music_activate (Music *self);
gboolean music_deactivate (Music *self);

G_END_DECLS

#endif /* __MUSIC_H__ */
