/*
 *      track-source.h
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

#ifndef __TRACK_SOURCE_H__
#define __TRACK_SOURCE_H__

#include <glib-object.h>

#include "entry.h"

#define TRACK_SOURCE_TYPE (track_source_get_type ())
#define TRACK_SOURCE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), TRACK_SOURCE_TYPE, TrackSource))
#define IS_TRACK_SOURCE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), TRACK_SOURCE_TYPE))
#define TRACK_SOURCE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TRACK_SOURCE_TYPE, TrackSourceInterface))

G_BEGIN_DECLS

typedef struct _TrackSource TrackSource;
typedef struct _TrackSourceInterface TrackSourceInterface;

struct _TrackSourceInterface {
    GTypeInterface parent;

    Entry* (*get_prev) (TrackSource *self);
    Entry* (*get_next) (TrackSource *self);
};

GType track_source_get_type (void);

Entry *track_source_get_next (TrackSource *self);
Entry *track_source_get_prev (TrackSource *self);

void track_source_emit_play (TrackSource *self, Entry *entry);

G_END_DECLS

#endif /* __TRACK_SOURCE_H__ */
