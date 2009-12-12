/*
 *      player-gst.h
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

#ifndef __PLAYER_GST_H__
#define __PLAYER_GST_H__

#include <glib-object.h>

#include "entry.h"
#include "player.h"

#define PLAYER_GST_TYPE (player_gst_get_type ())
#define PLAYER_GST(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), PLAYER_GST_TYPE, PlayerGst))
#define PLAYER_GST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PLAYER_GST_TYPE, PlayerGstClass))
#define IS_PLAYER_GST(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PLAYER_GST_TYPE))
#define IS_PLAYER_GST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAYER_GST_TYPE))
#define PLAYER_GST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAYER_GST_TYPE, PlayerGstClass))

G_BEGIN_DECLS

typedef struct _PlayerGst PlayerGst;
typedef struct _PlayerGstClass PlayerGstClass;
typedef struct _PlayerGstPrivate PlayerGstPrivate;

struct _PlayerGst {
    GObject parent;

    PlayerGstPrivate *priv;
};

struct _PlayerGstClass {
    GObjectClass parent;
};

typedef struct {
    gchar *lang;
    gint index;
    gboolean mute;
} StreamData;

Player *player_gst_new (Shell *shell);
GType player_gst_get_type (void);

G_END_DECLS

#endif /* __PLAYER_GST_H__ */
