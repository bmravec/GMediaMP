/*
 *      browser.c
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

#include "browser.h"
#include "title.h"
#include "album.h"
#include "artist.h"

G_DEFINE_TYPE(Browser, browser, GTK_TYPE_VPANED)

struct _BrowserPrivate {
    GtkWidget *artist;
    GtkWidget *album;
    GtkWidget *title;
    GtkWidget *hbox;
    
    gchar *sel_artist;
    gchar *sel_album;
};

static guint signal_add;
static guint signal_replace;

GtkWidget*
add_scroll_bars (GtkWidget *widget)
{
    GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (sw), widget);
    
    return sw;
}

void
artist_select (GtkWidget *widget,
               gchar *artist,
               Browser *self)
{
    g_print ("Artist Select: (%s)\n", artist);
    album_set_filter (ALBUM (self->priv->album), artist);
    title_set_filter (TITLE (self->priv->title), artist, "");
}

void
artist_replace (GtkWidget *widget,
                gchar *artist,
                Browser *self)
{
    g_print ("Artist replace: (%s)\n", artist);
}

void
album_select (GtkWidget *widget,
              gchar *album,
              Browser *self)
{
    g_print ("Album Select: (%s)\n", album);
    title_set_filter (TITLE (self->priv->title), NULL, album);
}

void
album_replace (GtkWidget *widget,
               gchar *album,
               Browser *self)
{
    g_print ("Album replace: (%s)\n", album);
}

void
title_replace (GtkWidget *widget,
               Entry *entry,
               Browser *self)
{
    g_signal_emit (G_OBJECT (self), signal_replace, 0, entry);
}

static void
browser_finalize (GObject *object)
{
    Browser *self = BROWSER (object);
    
    G_OBJECT_CLASS (browser_parent_class)->finalize (object);
}

static void
browser_class_init (BrowserClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (BrowserPrivate));
    
    object_class->finalize = browser_finalize;
    
    signal_add = g_signal_new ("entry-add", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER);
    
    signal_replace = g_signal_new ("entry-replace", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
browser_init (Browser *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), BROWSER_TYPE, BrowserPrivate);
    
    self->priv->artist = artist_new ();
    self->priv->album = album_new ();
    self->priv->title = title_new ();
    
    self->priv->hbox = gtk_hbox_new (TRUE, 5);
    gtk_box_pack_start (GTK_BOX (self->priv->hbox),
        add_scroll_bars (self->priv->artist), TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (self->priv->hbox),
        add_scroll_bars (self->priv->album), TRUE, TRUE, 0);
    
    gtk_paned_add1 (GTK_PANED (self), self->priv->hbox);
    gtk_paned_add2 (GTK_PANED (self), add_scroll_bars (self->priv->title));
    
    self->priv->sel_artist = g_strdup ("");
    self->priv->sel_album = g_strdup ("");
    
    g_signal_connect (G_OBJECT (self->priv->artist), "entry-replace",
        G_CALLBACK (artist_replace), self);
    g_signal_connect (G_OBJECT (self->priv->artist), "select",
        G_CALLBACK (artist_select), self);
    
    g_signal_connect (G_OBJECT (self->priv->album), "entry-replace",
        G_CALLBACK (album_replace), self);
    g_signal_connect (G_OBJECT (self->priv->album), "select",
        G_CALLBACK (album_select), self);
    
    g_signal_connect (G_OBJECT (self->priv->title), "entry-replace",
        G_CALLBACK (title_replace), self);
}

GtkWidget*
browser_new ()
{
    return g_object_new (BROWSER_TYPE, NULL);
}

void
browser_add_entry (Browser *self, Entry *entry)
{
    artist_add_entry (ARTIST (self->priv->artist), entry_get_artist (entry));
    
    album_add_entry (ALBUM (self->priv->album),
        entry_get_album (entry), entry_get_artist (entry));
    
    title_add_entry (TITLE (self->priv->title), entry);
}

