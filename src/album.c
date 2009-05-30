/*
 *      album.c
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

#include "album.h"

G_DEFINE_TYPE(Album, album, G_TYPE_OBJECT)

struct _AlbumPrivate {
    gint i;
};

static void
album_finalize (GObject *object)
{
    Album *self = ALBUM (object);
    
    G_OBJECT_CLASS (album_parent_class)->finalize (object);
}

static void
album_class_init (AlbumClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (AlbumPrivate));
    
    object_class->finalize = album_finalize;
}

static void
album_init (Album *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), ALBUM_TYPE, AlbumPrivate);
    
    
}

Album*
album_new ()
{
    return g_object_new (ALBUM_TYPE, NULL);
}

