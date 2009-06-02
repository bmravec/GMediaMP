/*
 *      gmediadb.c
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

#include "gmediadb.h"

#include <dbus/dbus-glib.h>

G_DEFINE_TYPE(GMediaDB, gmediadb, G_TYPE_OBJECT)

struct _GMediaDBPrivate {
    DBusGProxy *proxy;
};

static guint signal_add;
static guint signal_remove;

static void media_removed_cb (DBusGProxy *proxy, guint id, gpointer user_data);
static void media_added_cb (DBusGProxy *proxy, guint id, gpointer user_data);

static void
gmediadb_finalize (GObject *object)
{
    GMediaDB *self = GMEDIADB (object);
    
    if (self->priv->proxy) {
        dbus_g_proxy_call (self->priv->proxy, "unref", NULL,
            G_TYPE_INVALID, G_TYPE_INVALID);
        
        self->priv->proxy = NULL;
    }
    
    G_OBJECT_CLASS (gmediadb_parent_class)->finalize (object);
}

static void
gmediadb_class_init (GMediaDBClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (GMediaDBPrivate));
    
    object_class->finalize = gmediadb_finalize;
    
    signal_add = g_signal_new ("add-entry", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);
    
    signal_remove = g_signal_new ("remove-entry", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
gmediadb_init (GMediaDB *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), GMEDIADB_TYPE, GMediaDBPrivate);
    
    DBusGConnection *conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
    if (!conn) {
        g_printerr ("Failed to open connection to bus\n");
        return;
    }
    
    self->priv->proxy = dbus_g_proxy_new_for_name (conn,
        "org.gnome.GMediaDB",
        "/org/gnome/GMediaDB/Music",
        "org.gnome.GMediaDB.MediaObject");
    
    if (!dbus_g_proxy_call (self->priv->proxy, "ref", NULL,
        G_TYPE_INVALID, G_TYPE_INVALID)) {
        g_printerr ("Unable to ref MediaObject\n");
        self->priv->proxy = NULL;
        return;
    }
    
    dbus_g_proxy_add_signal (self->priv->proxy, "media_added",
        G_TYPE_UINT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (self->priv->proxy, "media_removed",
        G_TYPE_UINT, G_TYPE_INVALID);
    
    dbus_g_proxy_connect_signal (self->priv->proxy, "media_added",
        G_CALLBACK (media_added_cb), self, NULL);
    dbus_g_proxy_connect_signal (self->priv->proxy, "media_removed",
        G_CALLBACK (media_removed_cb), self, NULL);
}

GMediaDB*
gmediadb_new ()
{
    return g_object_new (GMEDIADB_TYPE, NULL);
}

GPtrArray*
gmediadb_get_entries (GMediaDB *self, GArray *ids, gchar *tags[])
{
    GPtrArray *entries;
    GError *error = NULL;
    
    g_print ("gmediadb_get_entries\n");
    
    if (!dbus_g_proxy_call (self->priv->proxy, "get_entries", &error,
        DBUS_TYPE_G_UINT_ARRAY, ids, G_TYPE_STRV, tags, G_TYPE_INVALID,
        dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRV), &entries,
        G_TYPE_INVALID)) {
        if (error->domain == DBUS_GERROR &&
            error->code == DBUS_GERROR_REMOTE_EXCEPTION) {
            g_printerr ("Caught remote method exception %s: %s",
                dbus_g_error_get_name (error), error->message);
        } else {
            g_printerr ("Error: %s\n", error->message);
        }
        
        g_error_free (error);
        return NULL;
    }
    
    return entries;
}

GPtrArray*
gmediadb_get_all_entries (GMediaDB *self, gchar *tags[])
{
    GPtrArray *entries;
    GError *error = NULL;
    
    g_print ("gmediadb_get_all_entries\n");
    
    if (!dbus_g_proxy_call (self->priv->proxy, "get_all_entries", &error,
        G_TYPE_STRV, tags, G_TYPE_INVALID,
        dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRV), &entries,
        G_TYPE_INVALID)) {
        if (error->domain == DBUS_GERROR &&
            error->code == DBUS_GERROR_REMOTE_EXCEPTION) {
            g_printerr ("Caught remote method exception %s: %s",
                dbus_g_error_get_name (error), error->message);
        } else {
            g_printerr ("Error: %s\n", error->message);
        }
        
        g_error_free (error);
        return NULL;
    }
    
    return entries;
}

void
gmediadb_import_path (GMediaDB *self, gchar *path)
{
    GError *error = NULL;
    
    g_print ("gmediadb_import_path\n");
    
    if (!dbus_g_proxy_call (self->priv->proxy, "import_path", &error,
        G_TYPE_STRING, path, G_TYPE_INVALID, G_TYPE_INVALID)) {
        if (error->domain == DBUS_GERROR &&
            error->code == DBUS_GERROR_REMOTE_EXCEPTION) {
            g_printerr ("Caught remote method exception %s: %s",
               dbus_g_error_get_name (error), error->message);
        } else {
            g_printerr ("Error: %s\n", error->message);
        }
        
        g_error_free (error);
        return;
    }
}

void
media_added_cb (DBusGProxy *proxy, guint id, gpointer user_data)
{
    g_signal_emit (G_OBJECT (user_data), signal_add, 0, id);
}

void
media_removed_cb (DBusGProxy *proxy, guint id, gpointer user_data)
{
    g_signal_emit (G_OBJECT (user_data), signal_remove, 0, id);
}

