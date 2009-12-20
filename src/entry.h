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

typedef enum {
    ENTRY_STATE_NONE = 0,
    ENTRY_STATE_PLAYING,
    ENTRY_STATE_PAUSED,
    ENTRY_STATE_MISSING,
} EntryState;

typedef enum {
    MEDIA_NONE = 0,
    MEDIA_SONG,
    MEDIA_MUSIC_VIDEO,
    MEDIA_MOVIE,
    MEDIA_TVSHOW,
} EntryType;

G_BEGIN_DECLS

typedef struct _Entry Entry;
typedef struct _EntryClass EntryClass;
typedef struct _EntryPrivate EntryPrivate;
typedef gint (*EntryCompareFunc) (Entry *e1, Entry *e2);

struct _Entry {
    GObject parent;

    EntryPrivate *priv;
};

struct _EntryClass {
    GObjectClass parent;
};

GType entry_get_type (void);

guint entry_get_id (Entry *self);

const gchar *entry_get_tag_str (Entry *self, const gchar *tag);
gint entry_get_tag_int (Entry *self, const gchar *tag);

const gchar *entry_get_location (Entry *self);

gchar *entry_get_art (Entry *self);

EntryState entry_get_state (Entry *self);
void entry_set_state (Entry *self, EntryState state);
gchar *entry_get_state_image (Entry *self);

EntryType entry_get_media_type (Entry *self);

guint entry_get_key_value_pairs (Entry *self, gchar ***keys, gchar ***vals);

// These functions should only be used inside the MediaStore object
Entry *_entry_new (guint id);
void   _entry_set_id (Entry *self, guint id);
void   _entry_set_tag_str (Entry *self, const gchar *tag, const gchar *value);
void   _entry_set_tag_int (Entry *self, const gchar *tag, gint value);
void   _entry_set_location (Entry *self, const gchar *location);
void   _entry_set_media_type (Entry *self, guint type);

G_END_DECLS

#endif /* __ENTRY_H__ */
