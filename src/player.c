/*
 *      player.c
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

#include "player.h"

static guint signal_eos;
static guint signal_state;
static guint signal_pos;
static guint signal_volume;

static guint signal_play;
static guint signal_pause;
static guint signal_next;
static guint signal_prev;

static void
player_base_init (gpointer g_iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;

        signal_eos = g_signal_new ("eos", PLAYER_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

        signal_state = g_signal_new ("state-changed", PLAYER_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
            G_TYPE_NONE, 1, G_TYPE_UINT);

        signal_volume = g_signal_new ("volume-changed", PLAYER_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__DOUBLE,
            G_TYPE_NONE, 1, G_TYPE_DOUBLE);

        signal_pos = g_signal_new ("pos-changed", PLAYER_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
            G_TYPE_NONE, 1, G_TYPE_UINT);

        signal_prev = g_signal_new ("previous", PLAYER_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

        signal_play = g_signal_new ("play", PLAYER_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

        signal_pause = g_signal_new ("pause", PLAYER_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

        signal_next = g_signal_new ("next", PLAYER_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    }
}

GType
player_get_type ()
{
    static GType object_type = 0;
    if (!object_type) {
        static const GTypeInfo object_info = {
            sizeof(PlayerInterface),
            player_base_init,       /* base init */
            NULL,                   /* base finalize */
        };

        object_type = g_type_register_static(G_TYPE_INTERFACE,
            "Player", &object_info, 0);
    }

    return object_type;
}

void
player_load (Player *self, Entry *entry)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->load) {
        iface->load (self, entry);
    }
}

void
player_close (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->close) {
        iface->close (self);
    }
}

Entry*
player_get_entry (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->get_entry) {
        return iface->get_entry (self);
    } else {
        return NULL;
    }
}

void
player_play (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->play) {
        iface->play (self);
    }
}

void
player_pause (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->pause) {
        iface->pause (self);
    }
}

void
player_stop (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->stop) {
        iface->stop (self);
    }
}

PlayerState
player_get_state (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->get_state) {
        return iface->get_state (self);
    } else {
        return PLAYER_STATE_NULL;
    }
}

guint
player_get_duration (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->get_duration) {
        return iface->get_duration (self);
    } else {
        return 0;
    }
}

guint
player_get_position (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->get_position) {
        return iface->get_position (self);
    } else {
        return 0;
    }
}

void
player_set_position (Player *self, guint pos)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->get_position) {
        return iface->set_position (self, pos);
    }
}

gdouble
player_get_volume (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->get_volume) {
        return iface->get_volume (self);
    } else {
        return 0.0;
    }
}

void
player_set_volume (Player *self, gdouble vol)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->set_volume) {
        return iface->set_volume (self, vol);
    }
}

void
player_set_video_destination (Player *self, GtkWidget *dest)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->set_video_destination) {
        return iface->set_video_destination (self, dest);
    }
}

GtkWidget*
player_get_video_destination (Player *self)
{
    PlayerInterface *iface = PLAYER_GET_IFACE (self);

    if (iface->get_video_destination) {
        return iface->get_video_destination (self);
    } else {
        return NULL;
    }
}

gchar*
time_to_string (gdouble time)
{
    gint hr, min, sec;

    hr = time;
    sec = hr % 60;
    hr /= 60;
    min = hr % 60;
    hr /= 60;

    if (hr > 0) {
        return g_strdup_printf ("%d:%02d:%02d", hr, min, sec);
    }

    return g_strdup_printf ("%02d:%02d", min, sec);
}

gpointer
_player_emit_eos_thread (Player *self)
{
    gdk_threads_enter ();
    g_signal_emit (self, signal_eos, 0);
    gdk_threads_leave ();
}

void
_player_emit_eos (Player *self)
{
    g_thread_create (_player_emit_eos_thread, self, FALSE, NULL);
}

void
_player_emit_state_changed (Player *self, PlayerState state)
{
    g_signal_emit (self, signal_state, 0, state);
}

void
_player_emit_position_changed (Player *self, guint pos)
{
    g_signal_emit (self, signal_pos, 0, pos);
}

void
_player_emit_volume_changed (Player *self, gdouble vol)
{
    g_signal_emit (self, signal_volume, 0, vol);
}

void
_player_emit_play (Player *self)
{
    g_signal_emit (self, signal_play, 0);
}

void
_player_emit_pause (Player *self)
{
    g_signal_emit (self, signal_pause, 0);
}

void
_player_emit_next (Player *self)
{
    g_signal_emit (self, signal_next, 0);
}

void
_player_emit_previous (Player *self)
{
    g_signal_emit (self, signal_prev, 0);
}
