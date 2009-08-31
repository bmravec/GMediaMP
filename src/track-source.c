/*
 *      track-source.c
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

#include "track-source.h"

static guint signal_play;

static void
track_source_base_init (gpointer g_iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;

        signal_play = g_signal_new ("entry-play", TRACK_SOURCE_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
            G_TYPE_NONE, 1, G_TYPE_POINTER);
    }
}

GType
track_source_get_type ()
{
    static GType object_type = 0;
    if (!object_type) {
        static const GTypeInfo object_info = {
            sizeof(TrackSourceInterface),
            track_source_base_init, /* base init */
            NULL,                   /* base finalize */
        };

        object_type = g_type_register_static(G_TYPE_INTERFACE,
            "TrackSource", &object_info, 0);
    }

    return object_type;
}

Entry*
track_source_get_next (TrackSource *self)
{
    return TRACK_SOURCE_GET_IFACE (self)->get_next (self);
}

Entry*
track_source_get_prev (TrackSource *self)
{
    return TRACK_SOURCE_GET_IFACE (self)->get_prev (self);
}

void
track_source_emit_play (TrackSource *self, Entry *entry)
{
    g_signal_emit (G_OBJECT (self), signal_play, 0, entry);
}
