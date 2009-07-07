/*
 *      entry.c
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

#include <gtk/gtk.h>
#include <string.h>

#include "entry.h"

G_DEFINE_TYPE(Entry, entry, G_TYPE_OBJECT)

struct _EntryPrivate {
    gchar *artist, *album, *title, *genre, *location;
    guint track, id, duration;
    guint state;
};

static guint signal_changed;

static void
entry_finalize (GObject *object)
{
    Entry *self = ENTRY (object);
    
    G_OBJECT_CLASS (entry_parent_class)->finalize (object);
}

static void
entry_class_init (EntryClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (EntryPrivate));
    
    object_class->finalize = entry_finalize;
    
    signal_changed = g_signal_new ("entry-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}

static void
entry_init (Entry *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), ENTRY_TYPE, EntryPrivate);
    
    self->priv->artist = "";
    self->priv->album = "";
    self->priv->title = "";
    self->priv->genre = "";
    self->priv->location = "";
    
    self->priv->track = 0;
    self->priv->id = 0;
    self->priv->duration = 0;
    self->priv->state = ENTRY_STATE_NONE;
}

Entry*
entry_new (guint id)
{
    Entry *self = g_object_new (ENTRY_TYPE, NULL);
    
    self->priv->id = id;
    
    return self;
}

gchar*
entry_get_artist (Entry *self)
{
    return self->priv->artist;
}

gchar*
entry_get_album (Entry *self)
{
    return self->priv->album;
}

gchar*
entry_get_title (Entry *self)
{
    return self->priv->title;
}

gchar*
entry_get_genre (Entry *self)
{
    return self->priv->genre;
}

gchar*
entry_get_location (Entry *self)
{
    return self->priv->location;
}

void
entry_set_artist (Entry *self, const gchar *artist)
{
    if (self->priv->artist && strlen (self->priv->artist) != 0) {
        g_free (self->priv->artist);
    }
    
    if (!artist || strlen (artist) == 0) {
        self->priv->artist = "";
    } else {
        self->priv->artist = g_strdup (artist);
    }
    
    g_signal_emit (G_OBJECT (self), signal_changed, 0);
}

void
entry_set_album (Entry *self, const gchar *album)
{
    if (self->priv->album && strlen (self->priv->album) != 0) {
        g_free (self->priv->album);
    }
    
    if (!album || strlen (album) == 0) {
        self->priv->album = "";
    } else {
        self->priv->album = g_strdup (album);
    }
    
    g_signal_emit (G_OBJECT (self), signal_changed, 0);
}

void
entry_set_title (Entry *self, const gchar *title)
{
    if (self->priv->title && strlen (self->priv->title) != 0) {
        g_free (self->priv->title);
    }
    
    if (!title || strlen (title) == 0) {
        self->priv->title = "";
    } else {
        self->priv->title = g_strdup (title);
    }
    
    g_signal_emit (G_OBJECT (self), signal_changed, 0);
}

void
entry_set_genre (Entry *self, const gchar *genre)
{
    if (self->priv->genre && strlen (self->priv->genre) != 0) {
        g_free (self->priv->genre);
    }
    
    if (!genre || strlen (genre) == 0) {
        self->priv->genre = "";
    } else {
        self->priv->genre = g_strdup (genre);
    }
    
    g_signal_emit (G_OBJECT (self), signal_changed, 0);
}

void
entry_set_location (Entry *self, const gchar *location)
{
    if (self->priv->location && strlen (self->priv->location) != 0) {
        g_free (self->priv->location);
    }
    
    if (!location || strlen (location) == 0) {
        self->priv->location = "";
    } else {
        self->priv->location = g_strdup (location);
    }
    
    g_signal_emit (G_OBJECT (self), signal_changed, 0);
}

guint
entry_get_id (Entry *self)
{
    return self->priv->id;
}

guint
entry_get_duration (Entry *self)
{
    return self->priv->duration;
}

guint
entry_get_track (Entry *self)
{
    return self->priv->track;
}

void
entry_set_duration (Entry *self, guint duration)
{
    self->priv->duration = duration;
    
    g_signal_emit (G_OBJECT (self), signal_changed, 0);
}

void
entry_set_track (Entry *self, guint track)
{
    self->priv->track = track;
    
    g_signal_emit (G_OBJECT (self), signal_changed, 0);
}

guint
entry_get_state (Entry *self)
{
    return self->priv->state;
}

void
entry_set_state (Entry *self, guint state)
{
    self->priv->state = state;
    
    g_signal_emit (G_OBJECT (self), signal_changed, 0);
}

gchar*
entry_get_state_string (Entry *self)
{
    switch (self->priv->state) {
        case ENTRY_STATE_PLAYING:
            return GTK_STOCK_MEDIA_PLAY;
        case ENTRY_STATE_MISSING:
            return GTK_STOCK_DIALOG_ERROR;
        default:
            return NULL;
    }
}

gint
entry_cmp (Entry *self, Entry *e)
{
    gint res;
    if (self->priv->id == e->priv->id)
        return 0;
    
    res = g_strcmp0 (entry_get_artist (self), entry_get_artist (e));
    if (res != 0)
        return res;
    
    res = g_strcmp0 (entry_get_album (self), entry_get_album (e));
    if (res != 0)
        return res;
    
    if (entry_get_track (e) != entry_get_track (self))
        return entry_get_track (self) - entry_get_track (e);
    
    res = g_strcmp0 (entry_get_title (self), entry_get_title (e));
    if (res != 0)
        return res;
    
    return -1;
}

