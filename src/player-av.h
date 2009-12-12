/*
 *      player-av.h
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

#include <glib-object.h>

#include "shell.h"
#include "player.h"
#include "entry.h"

#ifndef __PLAYER_AV_H__
#define __PLAYER_AV_H__

#define PLAYER_AV_TYPE (player_av_get_type ())
#define PLAYER_AV(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), PLAYER_AV_TYPE, PlayerAV))
#define PLAYER_AV_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PLAYER_AV_TYPE, PlayerAVClass))
#define IS_PLAYER_AV(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PLAYER_AV_TYPE))
#define IS_PLAYER_AV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAYER_AV_TYPE))
#define PLAYER_AV_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAYER_AV_TYPE, PlayerAVClass))

G_BEGIN_DECLS

typedef struct _PlayerAV PlayerAV;
typedef struct _PlayerAVClass PlayerAVClass;
typedef struct _PlayerAVPrivate PlayerAVPrivate;

struct _PlayerAV {
    GObject parent;

    PlayerAVPrivate *priv;
};

struct _PlayerAVClass {
    GObjectClass parent;
};

Player *player_av_new (Shell *shell);
GType player_av_get_type (void);

G_END_DECLS

#endif /* __PLAYER_AV_H__ */
