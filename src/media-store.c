/*
 *      media-store.c
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

#include "media-store.h"

static guint signal_add;
static guint signal_remove;

static void
media_store_base_init (gpointer g_iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;

        signal_add = g_signal_new ("add-entry", MEDIA_STORE_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
            G_TYPE_NONE, 1, G_TYPE_POINTER);

        signal_remove = g_signal_new ("remove-entry", MEDIA_STORE_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
            G_TYPE_NONE, 1, G_TYPE_POINTER);
    }
}

GType
media_store_get_type ()
{
    static GType object_type = 0;
    if (!object_type) {
        static const GTypeInfo object_info = {
            sizeof(MediaStoreInterface),
            media_store_base_init,  /* base init */
            NULL,                   /* base finalize */
        };

        object_type = g_type_register_static(G_TYPE_INTERFACE,
            "MediaStore", &object_info, 0);
    }

    return object_type;
}

void
media_store_add_entry (MediaStore *self, gchar **entry)
{
    MediaStoreInterface *iface = MEDIA_STORE_GET_IFACE (self);

    if (iface->add_entry) {
        iface->add_entry (self, entry);
    }
}

void
media_store_update_entry (MediaStore *self, guint id, gchar **entry)
{
    MediaStoreInterface *iface = MEDIA_STORE_GET_IFACE (self);

    if (iface->up_entry) {
        iface->up_entry (self, id, entry);
    }
}

void
media_store_remove_entry (MediaStore *self, Entry *entry)
{
    MediaStoreInterface *iface = MEDIA_STORE_GET_IFACE (self);

    if (iface->rem_entry) {
        iface->rem_entry (self, entry);
    }
}

guint
media_store_get_media_type (MediaStore *self)
{
    MediaStoreInterface *iface = MEDIA_STORE_GET_IFACE (self);

    if (iface->get_mtype) {
        return iface->get_mtype (self);
    } else {
        return MEDIA_NONE;
    }
}

gchar*
media_store_get_name (MediaStore *self)
{
    MediaStoreInterface *iface = MEDIA_STORE_GET_IFACE (self);

    if (iface->get_name) {
        return iface->get_name (self);
    } else {
        return NULL;
    }
}

Entry**
media_store_get_all_entries (MediaStore *self)
{
    MediaStoreInterface *iface = MEDIA_STORE_GET_IFACE (self);

    if (iface->get_all_entries) {
        return iface->get_all_entries (self);
    } else {
        return NULL;
    }
}

Entry*
media_store_get_entry (MediaStore *self, guint id)
{
    MediaStoreInterface *iface = MEDIA_STORE_GET_IFACE (self);

    if (iface->get_name) {
        return iface->get_entry (self, id);
    } else {
        return NULL;
    }
}

void
_media_store_emit_add_entry (MediaStore *self, Entry *entry)
{
    g_signal_emit (self, signal_add, 0, entry);
}

void
_media_store_emit_remove_entry (MediaStore *self, Entry *entry)
{
    g_signal_emit (self, signal_remove, 0, entry);
}
