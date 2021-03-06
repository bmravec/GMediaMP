/*
 *      media-store.h
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

#ifndef __MEDIA_STORE_H__
#define __MEDIA_STORE_H__

#include <glib-object.h>

#include "entry.h"

#define MEDIA_STORE_TYPE (media_store_get_type ())
#define MEDIA_STORE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MEDIA_STORE_TYPE, MediaStore))
#define IS_MEDIA_STORE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MEDIA_STORE_TYPE))
#define MEDIA_STORE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MEDIA_STORE_TYPE, MediaStoreInterface))

G_BEGIN_DECLS

typedef struct _MediaStore MediaStore;
typedef struct _MediaStoreInterface MediaStoreInterface;

struct _MediaStoreInterface {
    GTypeInterface parent;

    void    (*add_entry) (MediaStore *self, gchar **entry);
    void    (*up_entry)  (MediaStore *self, guint id, gchar **entry);
    void    (*rem_entry) (MediaStore *self, Entry *entry);
    guint   (*get_mtype) (MediaStore *self);
    gchar*  (*get_name)  (MediaStore *self);

    Entry** (*get_all_entries) (MediaStore *self);
    Entry*  (*get_entry) (MediaStore *self, guint id);
};

GType media_store_get_type (void);

void media_store_add_entry (MediaStore *self, gchar **entry);
void media_store_update_entry (MediaStore *self, guint id, gchar **entry);
void media_store_remove_entry (MediaStore *self, Entry *entry);

guint media_store_get_media_type (MediaStore *self);
gchar *media_store_get_name (MediaStore *self);

Entry **media_store_get_all_entries (MediaStore *self);
Entry *media_store_get_entry (MediaStore *self, guint id);

void _media_store_emit_add_entry (MediaStore *self, Entry *entry);
void _media_store_emit_remove_entry (MediaStore *self, Entry *entry);

G_END_DECLS

#endif /* __MEDIA_STORE_H__ */
