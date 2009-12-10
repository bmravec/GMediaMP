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

#include <gtk/gtk.h>

#include "shell.h"
#include "entry.h"

#define PLAYER_TYPE (player_get_type ())
#define PLAYER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), PLAYER_TYPE, Player))
#define IS_PLAYER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PLAYER_TYPE))
#define PLAYER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), PLAYER_TYPE, PlayerInterface))

typedef enum {
    PLAYER_STATE_NULL = 0,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED,
    PLAYER_STATE_STOPPED,
} PlayerState;

G_BEGIN_DECLS

typedef struct _Player Player;
typedef struct _PlayerInterface PlayerInterface;

struct _PlayerInterface {
    GTypeInterface parent;

    void (*load) (Player *self, Entry *entry);
    void (*close) (Player *self);
    Entry* (*get_entry) (Player *self);

    void (*play) (Player *self);
    void (*pause) (Player *self);
    void (*stop) (Player *self);
    PlayerState (*get_state) (Player *self);

    guint (*get_duration) (Player *self);
    guint (*get_position) (Player *self);
    void  (*set_position) (Player *self, guint pos);

    void (*set_volume) (Player *self, gdouble vol);
    gdouble (*get_volume) (Player *self);

    void (*set_video_destination) (Player *self, GtkWidget *dest);
    GtkWidget* (*get_video_destination) (Player *self);
};

GType player_get_type (void);
Player *player_new (Shell *shell);

void player_load (Player *self, Entry *entry);
void player_close (Player *self);

Entry *player_get_entry (Player *self);

void player_play (Player *self);
void player_pause (Player *self);
void player_stop (Player *self);
PlayerState player_get_state (Player *self);

guint player_get_duration (Player *self);
guint player_get_position (Player *self);
void player_set_position (Player *self, guint pos);

gdouble player_get_volume (Player *self);
void player_set_volume (Player *self, gdouble vol);

void player_set_video_destination (Player *self, GtkWidget *dest);
GtkWidget *player_get_video_destination (Player *self);

gchar *time_to_string (gdouble time);

void _player_emit_eos (Player *self);
void _player_emit_state_changed (Player *self, PlayerState state);
void _player_emit_position_changed (Player *self, guint pos);
void _player_emit_volume_changed (Player *self, gdouble vol);

void _player_emit_play (Player *self);
void _player_emit_pause (Player *self);
void _player_emit_next (Player *self);
void _player_emit_previous (Player *self);

G_END_DECLS

#endif /* __PLAYER_H__ */
