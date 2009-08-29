/*
 *      shows.h
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

#ifndef __SHOWS_H__
#define __SHOWS_H__

#include <glib-object.h>

#include "entry.h"

#define SHOWS_TYPE (shows_get_type ())
#define SHOWS(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), SHOWS_TYPE, Shows))
#define SHOWS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SHOWS_TYPE, ShowsClass))
#define IS_SHOWS(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), SHOWS_TYPE))
#define IS_SHOWS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SHOWS_TYPE))
#define SHOWS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SHOWS_TYPE, ShowsClass))

G_BEGIN_DECLS

typedef struct _Shows Shows;
typedef struct _ShowsClass ShowsClass;
typedef struct _ShowsPrivate ShowsPrivate;

struct _Shows {
    GObject parent;

    ShowsPrivate *priv;
};

struct _ShowsClass {
    GObjectClass parent;
};

Shows *shows_new ();
GType shows_get_type (void);

gboolean shows_activate (Shows *self);
gboolean shows_deactivate (Shows *self);

G_END_DECLS

#endif /* __SHOWS_H__ */
