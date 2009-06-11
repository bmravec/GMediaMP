/*
 *      player.h
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

#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <glib-object.h>

#define PLAYER_TYPE (player_get_type ())
#define PLAYER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), PLAYER_TYPE, Player))
#define PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PLAYER_TYPE, PlayerClass))
#define IS_PLAYER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PLAYER_TYPE))
#define IS_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAYER_TYPE))
#define PLAYER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAYER_TYPE, PlayerClass))

enum {
    PLAYER_STATE_NULL = 0,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED,
    PLAYER_STATE_STOPPED,
};

G_BEGIN_DECLS

typedef struct _Player Player;
typedef struct _PlayerClass PlayerClass;
typedef struct _PlayerPrivate PlayerPrivate;

struct _Player {
    GObject parent;
    
    PlayerPrivate *priv;
};

struct _PlayerClass {
    GObjectClass parent;
};

Player *player_new ();
GType player_get_type (void);
void player_load (Player *self, gchar *uri);
void player_close (Player *self);
void player_play (Player *self);
void player_pause (Player *self);
void player_stop (Player *self);
guint player_get_state (Player *self);

guint player_get_length (Player *self);
guint player_get_position (Player *self);

G_END_DECLS

#endif /* __PLAYER_H__ */

