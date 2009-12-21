/*
 *      ipod-store.c
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

#include "ipod-store.h"
#include "media-store.h"
#include "shell.h"

static void media_store_init (MediaStoreInterface *iface);
G_DEFINE_TYPE_WITH_CODE (IPodStore, ipod_store, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MEDIA_STORE_TYPE, media_store_init)
)

struct _IPodStorePrivate {
    Shell *shell;

    Itdb_iTunesDB *db;

    gint mtype;

    GHashTable *entries;
};

static void ipod_store_class_init (IPodStoreClass *klass);
static void ipod_store_init (IPodStore *self);
static void ipod_store_finalize (GObject *object);

// Interface methods
static guint ipod_store_get_mtype (MediaStore *self);
static gchar *ipod_store_get_name (MediaStore *self);
static Entry **ipod_store_get_all_entries (MediaStore *self);
static Entry *ipod_store_get_entry (MediaStore *self, guint id);

static void
ipod_store_class_init (IPodStoreClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (IPodStorePrivate));

    object_class->finalize = ipod_store_finalize;
}

static void
media_store_init (MediaStoreInterface *iface)
{
    iface->get_mtype = ipod_store_get_mtype;
    iface->get_name  = ipod_store_get_name;

    iface->get_all_entries = ipod_store_get_all_entries;
    iface->get_entry = ipod_store_get_entry;
}

static void
ipod_store_init (IPodStore *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), IPOD_STORE_TYPE, IPodStorePrivate);

    self->priv->entries = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, g_object_unref);
}

static void
ipod_store_finalize (GObject *object)
{
    IPodStore *self = IPOD_STORE (object);

    if (self->priv->entries) {
        g_hash_table_unref (self->priv->entries);
        self->priv->entries = NULL;
    }

    G_OBJECT_CLASS (ipod_store_parent_class)->finalize (object);
}

IPodStore*
ipod_store_new (Itdb_iTunesDB *db, gint mtype)
{
    IPodStore *self = g_object_new (IPOD_STORE_TYPE, NULL);

    self->priv->db = db;
    self->priv->mtype = mtype;

    GList *iter;

    for (iter = self->priv->db->tracks; iter; iter = iter->next) {
        Itdb_Track *track = (Itdb_Track*) iter->data;

        if (track->mediatype != mtype) {
            continue;
        }

        guint *nid = g_new0 (guint, 1);

        *nid = track->id;

        Entry *e = _entry_new (*nid);
        _entry_set_media_type (e, self->priv->mtype);
        _entry_set_tag_str (e, "title", track->title);
        _entry_set_tag_int (e, "duration", track->tracklen / 1000);

        gchar *file = itdb_filename_on_ipod (track);
//        itdb_filename_ipod2fs (file);
//        gchar *loc = g_strdup_printf ("%s%s", itdb_get_mountpoint (db), file);
//        g_free (file);

        _entry_set_tag_str (e, "location", file);
//        g_free (loc);

        g_hash_table_insert (self->priv->entries, nid, e);
    }

    return self;
}

static void
ipod_store_add_entry (MediaStore *self, gchar **entry)
{
    IPodStorePrivate *priv = IPOD_STORE (self)->priv;

    gmediadb_add_entry (priv->db, entry);
}

static void
ipod_store_update_entry (MediaStore *self, guint id, gchar **entry)
{
    IPodStorePrivate *priv = IPOD_STORE (self)->priv;

    gmediadb_update_entry (priv->db, id, entry);
}

static void
ipod_store_remove_entry (MediaStore *self, Entry *entry)
{
    gmediadb_remove_entry (IPOD_STORE (self)->priv->db, entry_get_id (entry));
}

static guint
ipod_store_get_mtype (MediaStore *self)
{
    return IPOD_STORE (self)->priv->mtype;
}

static gchar*
ipod_store_get_name (MediaStore *self)
{
    return "IPod";
}

static Entry*
ipod_store_get_entry (MediaStore *self, guint id)
{
    IPodStorePrivate *priv = IPOD_STORE (self)->priv;

    return g_object_ref (g_hash_table_lookup (priv->entries, &id));
}

static Entry**
ipod_store_get_all_entries (MediaStore *self)
{
    IPodStorePrivate *priv = IPOD_STORE (self)->priv;

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
