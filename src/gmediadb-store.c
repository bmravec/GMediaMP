/*
 *      gmediadb-store.c
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

#include <gmediadb.h>

#include "shell.h"
#include "media-store.h"
#include "gmediadb-store.h"

static void media_store_init (MediaStoreInterface *iface);
G_DEFINE_TYPE_WITH_CODE (GMediaDBStore, gmediadb_store, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MEDIA_STORE_TYPE, media_store_init)
)

struct _GMediaDBStorePrivate {
    Shell *shell;
    GMediaDB *db;

    gint mtype;
    gchar *media_type;

    GHashTable *entries;
};

// Interface methods
static void gmediadb_store_add_entry (MediaStore *self, gchar **entry);
static void gmediadb_store_remove_entry (MediaStore *self, Entry *entry);
static guint gmediadb_store_get_mtype (MediaStore *self);
static gchar *gmediadb_store_get_name (MediaStore *self);
static Entry **gmediadb_store_get_all_entries (MediaStore *self);
static Entry *gmediadb_store_get_entry (MediaStore *self, guint id);

// Signals from GMediaDB
static void gmediadb_store_gmediadb_add (GMediaDB *db, guint id, GMediaDBStore *self);
static void gmediadb_store_gmediadb_remove (GMediaDB *db, guint id, GMediaDBStore *self);
static void gmediadb_store_gmediadb_update (GMediaDB *db, guint id, GMediaDBStore *self);

static void
media_store_init (MediaStoreInterface *iface)
{
    iface->add_entry = gmediadb_store_add_entry;
    iface->rem_entry = gmediadb_store_remove_entry;
    iface->get_mtype = gmediadb_store_get_mtype;
    iface->get_name  = gmediadb_store_get_name;

    iface->get_all_entries = gmediadb_store_get_all_entries;
    iface->get_entry = gmediadb_store_get_entry;
}

static void
gmediadb_store_finalize (GObject *object)
{
    GMediaDBStore *self = GMEDIADB_STORE (object);

    if (self->priv->db) {
        g_object_unref (self->priv->db);
        self->priv->db = NULL;
    }

    if (self->priv->entries) {
        g_hash_table_unref (self->priv->entries);
        self->priv->entries = NULL;
    }

    G_OBJECT_CLASS (gmediadb_store_parent_class)->finalize (object);
}

static void
gmediadb_store_class_init (GMediaDBStoreClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (GMediaDBStorePrivate));

    object_class->finalize = gmediadb_store_finalize;
}

static void
gmediadb_store_init (GMediaDBStore *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), GMEDIADB_STORE_TYPE, GMediaDBStorePrivate);

    self->priv->entries = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, g_object_unref);
}

GMediaDBStore*
gmediadb_store_new (gchar *media_type, gint mtype)
{
    GMediaDBStore *self = g_object_new (GMEDIADB_STORE_TYPE, NULL);

    self->priv->mtype = mtype;
    self->priv->media_type = g_strdup (media_type);

    self->priv->db = gmediadb_new (media_type);

    GPtrArray *entries = gmediadb_get_all_entries (self->priv->db, NULL);

    int i, j;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);

        guint *nid = g_new0 (guint, 1);

        *nid = entry[1] ? atoi (entry[1]) : 0;

        Entry *e = entry_new (*nid);
        entry_set_media_type (e, self->priv->mtype);

        for (j = 2; entry[j]; j += 2) {
            entry_set_tag_str (e, entry[j], entry[j + 1]);
        }

        g_hash_table_insert (self->priv->entries, nid, e);
    }

    return self;
}

static void
gmediadb_store_add_entry (MediaStore *self, gchar **entry)
{
    GMediaDBStorePrivate *priv = GMEDIADB_STORE (self)->priv;

    gmediadb_add_entry (priv->db, entry);
}

static void
gmediadb_store_remove_entry (MediaStore *self, Entry *entry)
{
    gmediadb_remove_entry (GMEDIADB_STORE (self)->priv->db, entry_get_id (entry));
}

static guint
gmediadb_store_get_mtype (MediaStore *self)
{
    return GMEDIADB_STORE (self)->priv->mtype;
}

static gchar*
gmediadb_store_get_name (MediaStore *self)
{
    return GMEDIADB_STORE (self)->priv->media_type;
}

static Entry*
gmediadb_store_get_entry (MediaStore *self, guint id)
{
    GMediaDBStorePrivate *priv = GMEDIADB_STORE (self)->priv;

    return g_object_ref (g_hash_table_lookup (priv->entries, &id));
}

static Entry**
gmediadb_store_get_all_entries (MediaStore *self)
{
    GMediaDBStorePrivate *priv = GMEDIADB_STORE (self)->priv;

    Entry **le = g_new0 (Entry*, g_hash_table_size (priv->entries) + 1);

    GHashTableIter iter;
    Entry *e;
    gint i = 0;

    g_hash_table_iter_init (&iter, priv->entries);
    while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &e)) {
        le[i++] = e;
    }

    return le;
}
