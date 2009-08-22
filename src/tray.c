/*
 *      tray.c
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

#include "tray.h"
#include "player.h"

G_DEFINE_TYPE(Tray, tray, G_TYPE_OBJECT)

struct _TrayPrivate {
    GtkStatusIcon *icon;
    GtkWidget *menu;

    GtkWidget *play_item;
    GtkWidget *paused_item;
};

static guint signal_toggle;
static guint signal_play;
static guint signal_next;
static guint signal_prev;
static guint signal_quit;

static void
tray_icon_activate (Tray *self, gpointer user_data)
{
    g_signal_emit (G_OBJECT (self), signal_toggle, 0);
}

static void
tray_popup (Tray *self, guint button, guint activate_time, gpointer user_data)
{
    gtk_menu_popup (GTK_MENU (self->priv->menu), NULL, NULL, NULL, NULL, button, activate_time);
}

static void
tray_play (GtkWidget *widget, Tray *self)
{
    g_signal_emit (G_OBJECT (self), signal_play, 0);
}

static void
tray_quit (GtkWidget *widget, Tray *self)
{
    g_signal_emit (G_OBJECT (self), signal_quit, 0);
}

static void
tray_next (GtkWidget *widget, Tray *self)
{
    g_signal_emit (G_OBJECT (self), signal_next, 0);
}

static void
tray_prev (GtkWidget *widget, Tray *self)
{
    g_signal_emit (G_OBJECT (self), signal_prev, 0);
}

static void
tray_finalize (GObject *object)
{
    Tray *self = TRAY (object);

    G_OBJECT_CLASS (tray_parent_class)->finalize (object);
}

static void
tray_class_init (TrayClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (TrayPrivate));

    object_class->finalize = tray_finalize;

    signal_toggle = g_signal_new ("toggle", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    signal_play = g_signal_new ("play", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    signal_next = g_signal_new ("next", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    signal_prev = g_signal_new ("previous", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    signal_quit = g_signal_new ("quit", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}

static void
tray_init (Tray *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), TRAY_TYPE, TrayPrivate);

    self->priv->icon = gtk_status_icon_new ();
    gtk_status_icon_set_from_stock (GTK_STATUS_ICON (self->priv->icon), GTK_STOCK_MEDIA_PLAY);

    g_signal_connect (G_OBJECT (self->priv->icon), "activate", G_CALLBACK (tray_icon_activate), NULL);
    g_signal_connect (G_OBJECT (self->priv->icon), "popup-menu", G_CALLBACK (tray_popup), NULL);

    self->priv->menu = gtk_menu_new ();

    self->priv->play_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PLAY, NULL);
    g_signal_connect (G_OBJECT (self->priv->play_item), "activate", G_CALLBACK (tray_play), self);
    gtk_menu_append (self->priv->menu, self->priv->play_item);

    self->priv->paused_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PAUSE, NULL);
    g_signal_connect (G_OBJECT (self->priv->paused_item), "activate", G_CALLBACK (tray_play), self);
    gtk_menu_append (self->priv->menu, self->priv->paused_item);

    GtkWidget *item = gtk_separator_menu_item_new ();
    gtk_menu_append (self->priv->menu, item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_NEXT, NULL);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (tray_next), self);
    gtk_menu_append (self->priv->menu, item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, NULL);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (tray_prev), self);
    gtk_menu_append (self->priv->menu, item);

    item = gtk_separator_menu_item_new ();
    gtk_menu_append (self->priv->menu, item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (tray_quit), self);
    gtk_menu_append (self->priv->menu, item);

    gtk_widget_show_all (GTK_WIDGET (self->priv->menu));
    gtk_widget_hide (GTK_WIDGET (self->priv->paused_item));
}

Tray*
tray_new ()
{
    return g_object_new (TRAY_TYPE, NULL);
}

void
tray_set_state (Tray *self, gint state)
{
    if (state == PLAYER_STATE_PLAYING) {
        gtk_widget_show (GTK_WIDGET (self->priv->paused_item));
        gtk_widget_hide (GTK_WIDGET (self->priv->play_item));
    } else {
        gtk_widget_show (GTK_WIDGET (self->priv->play_item));
        gtk_widget_hide (GTK_WIDGET (self->priv->paused_item));
    }
}

void
tray_activate (Tray *self)
{
    gtk_widget_show (GTK_WIDGET (self->priv->icon));
}

void
tray_deactivate (Tray *self)
{
    gtk_widget_hide (GTK_WIDGET (self->priv->icon));
}
