/*
 *      artist.c
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

#include "artist.h"

G_DEFINE_TYPE(Artist, artist, G_TYPE_OBJECT)

struct _ArtistPrivate {
    gint i;
};

static void
artist_finalize (GObject *object)
{
    Artist *self = ARTIST (object);
    
    G_OBJECT_CLASS (artist_parent_class)->finalize (object);
}

static void
artist_class_init (ArtistClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (ArtistPrivate));
    
    object_class->finalize = artist_finalize;
}

static void
artist_init (Artist *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), ARTIST_TYPE, ArtistPrivate);
    
    
}

Artist*
artist_new ()
{
    return g_object_new (ARTIST_TYPE, NULL);
}

