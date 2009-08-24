/*
 *      movies.h
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

#ifndef __MOVIES_H__
#define __MOVIES_H__

#include <glib-object.h>

#include "entry.h"

#define MOVIES_TYPE (movies_get_type ())
#define MOVIES(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MOVIES_TYPE, Movies))
#define MOVIES_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MOVIES_TYPE, MoviesClass))
#define IS_MOVIES(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOVIES_TYPE))
#define IS_MOVIES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOVIES_TYPE))
#define MOVIES_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MOVIES_TYPE, MoviesClass))

G_BEGIN_DECLS

typedef struct _Movies Movies;
typedef struct _MoviesClass MoviesClass;
typedef struct _MoviesPrivate MoviesPrivate;

struct _Movies {
    GObject parent;

    MoviesPrivate *priv;
};

struct _MoviesClass {
    GObjectClass parent;
};

Movies *movies_new ();
GType movies_get_type (void);

gboolean movies_activate (Movies *self);
gboolean movies_deactivate (Movies *self);

G_END_DECLS

#endif /* __MOVIES_H__ */
