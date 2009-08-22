/*
 *      entry.h
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

#ifndef __ENTRY_H__
#define __ENTRY_H__

#include <glib-object.h>

#define ENTRY_TYPE (entry_get_type ())
#define ENTRY(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ENTRY_TYPE, Entry))
#define ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), ENTRY_TYPE, EntryClass))
#define IS_ENTRY(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), ENTRY_TYPE))
#define IS_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTRY_TYPE))
#define ENTRY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTRY_TYPE, EntryClass))

enum {
    ENTRY_STATE_NONE = 0,
    ENTRY_STATE_PLAYING,
    ENTRY_STATE_PAUSED,
    ENTRY_STATE_STOPPED,
    ENTRY_STATE_MISSING,
};

enum {
    MEDIA_SONG = 0,
    MEDIA_MUSIC_VIDEO,
    MEDIA_MOVIE,
    MEDIA_TVSHOW,
};

G_BEGIN_DECLS

typedef struct _Entry Entry;
typedef struct _EntryClass EntryClass;
typedef struct _EntryPrivate EntryPrivate;

struct _Entry {
    GObject parent;

    EntryPrivate *priv;
};

struct _EntryClass {
    GObjectClass parent;
};

Entry *entry_new (guint id);
GType entry_get_type (void);

void entry_set_tag_str (Entry *self, const gchar *tag, const gchar *value);
void entry_set_tag_int (Entry *self, const gchar *tag, gint value);

const gchar *entry_get_tag_str (Entry *self, const gchar *tag);
gint entry_get_tag_int (Entry *self, const gchar *tag);

gint entry_cmp (Entry *self, Entry *e);

const gchar *entry_get_artist (Entry *self);
const gchar *entry_get_album (Entry *self);
const gchar *entry_get_title (Entry *self);
const gchar *entry_get_genre (Entry *self);
const gchar *entry_get_location (Entry *self);
gchar *entry_get_art (Entry *self);
guint entry_get_id (Entry *self);
guint entry_get_duration (Entry *self);
guint entry_get_track (Entry *self);

void entry_set_artist (Entry *self, const gchar *artist);
void entry_set_album (Entry *self, const gchar *album);
void entry_set_title (Entry *self, const gchar *title);
void entry_set_genre (Entry *self, const gchar *genre);
void entry_set_location (Entry *self, const gchar *location);
void entry_set_duration (Entry *self, guint duration);
void entry_set_track (Entry *self, guint track);

guint entry_get_state (Entry *self);
void entry_set_state (Entry *self, guint state);
gchar *entry_get_state_image (Entry *self);

guint entry_get_media_type (Entry *self);
void entry_set_media_type (Entry *self, guint type);

G_END_DECLS

#endif /* __ENTRY_H__ */
