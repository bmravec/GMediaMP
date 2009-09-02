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
#include <libnotify/notify.h>

#include "tray.h"

#include "player.h"
#include "shell.h"

G_DEFINE_TYPE(Tray, tray, G_TYPE_OBJECT)

struct _TrayPrivate {
    Shell *shell;

    GtkStatusIcon *icon;
    GtkWidget *menu;

    GtkWidget *play_item;
    GtkWidget *paused_item;

    NotifyNotification *note;
    GdkPixbuf *img;
};

static guint signal_play;
static guint signal_pause;
static guint signal_next;
static guint signal_prev;
static guint signal_quit;

static void tray_state_changed (Player *player, gint state, Tray *self);
static void tray_pos_changed (Player *player, guint pos, Tray *self);
static gboolean tray_query_tooltip (GtkStatusIcon *icon, gint x, gint y,
    gboolean keyboard_mode, GtkTooltip *tooltip, Tray *self);
static gboolean tray_button_press (GtkStatusIcon *icon, GdkEventButton *event,
    Tray *self);
static gboolean tray_scroll_event (GtkStatusIcon *icon, GdkEventScroll *event,
    Tray *self);

static void
tray_icon_activate (GtkStatusIcon *icon, Tray *self)
{
    shell_toggle_visibility (self->priv->shell);
}

static void
tray_popup (GtkStatusIcon *icon, guint button, guint activate_time, Tray *self)
{
    gtk_menu_popup (GTK_MENU (self->priv->menu), NULL, NULL, NULL, NULL, button, activate_time);
}

static void
tray_play (GtkWidget *widget, Tray *self)
{
    g_signal_emit (G_OBJECT (self), signal_play, 0);
}

static void
tray_pause (GtkWidget *widget, Tray *self)
{
    g_signal_emit (G_OBJECT (self), signal_pause, 0);
}

static void
tray_quit (GtkWidget *widget, Tray *self)
{
    shell_quit (self->priv->shell);
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

    signal_play = g_signal_new ("play", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    signal_pause = g_signal_new ("pause", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    signal_next = g_signal_new ("next", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    signal_prev = g_signal_new ("previous", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    notify_init ("GMediaDB");
}

static void
tray_init (Tray *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), TRAY_TYPE, TrayPrivate);
}

Tray*
tray_new ()
{
    return g_object_new (TRAY_TYPE, NULL);
}

void
on_notify_next (NotifyNotification *note, gchar *action, Tray *self)
{
    g_signal_emit (G_OBJECT (self), signal_next, 0);
}

static void
tray_state_changed (Player *player, gint state, Tray *self)
{
    if (state == PLAYER_STATE_PLAYING) {
        gtk_widget_show (GTK_WIDGET (self->priv->paused_item));
        gtk_widget_hide (GTK_WIDGET (self->priv->play_item));
        gtk_status_icon_set_from_file (self->priv->icon, SHARE_DIR "/imgs/tray-icon-playing.svg");
    } else {
        gtk_widget_show (GTK_WIDGET (self->priv->play_item));
        gtk_widget_hide (GTK_WIDGET (self->priv->paused_item));
        gtk_status_icon_set_from_file (self->priv->icon, SHARE_DIR "/imgs/tray-icon.svg");
    }

    if (state == PLAYER_STATE_PLAYING && self->priv->note == NULL) {
        Entry *e = player_get_entry (player);

        const gchar *title = entry_get_tag_str (e, "title");
        const gchar *artist = entry_get_tag_str (e, "artist");
        const gchar *album = entry_get_tag_str (e, "album");
        const gchar *art = entry_get_art (e);

        gchar *body = g_strdup_printf ("%s\n%s\n%s", title, artist, album);

        self->priv->note = notify_notification_new_with_status_icon (
            "Now Playing", body, NULL, self->priv->icon);

        if (art != NULL) {
            self->priv->img = gdk_pixbuf_new_from_file_at_scale (entry_get_art (e), 50, 50, TRUE, NULL);
        } else {
            self->priv->img = gdk_pixbuf_new_from_file_at_scale (SHARE_DIR "/imgs/rhythmbox-missing-artwork.svg", 50, 50, TRUE, NULL);
        }

        notify_notification_set_icon_from_pixbuf (self->priv->note, self->priv->img);

        notify_notification_add_action (self->priv->note, GTK_STOCK_MEDIA_NEXT, "Next",
            NOTIFY_ACTION_CALLBACK (on_notify_next), self, NULL);

        notify_notification_show (self->priv->note, NULL);
    }

    if ((state == PLAYER_STATE_STOPPED || state == PLAYER_STATE_NULL) &&
        self->priv->note != NULL) {
        notify_notification_close (self->priv->note, NULL);
        g_object_unref (self->priv->note);
        self->priv->note = NULL;

        g_object_unref (self->priv->img);
        self->priv->img = NULL;
    }
/*
    g_print ("STATE CHANGED: ");
    switch (state) {
        case PLAYER_STATE_PLAYING:
            g_print ("PLAYING\n");
            break;
        case PLAYER_STATE_PAUSED:
            g_print ("PAUSED\n");
            break;
        case PLAYER_STATE_STOPPED:
            g_print ("STOPPED\n");
            break;
        case PLAYER_STATE_NULL:
            g_print ("NULL\n");
    }
    */
}

void
tray_activate (Tray *self)
{
    self->priv->shell = shell_new ();

    self->priv->icon = gtk_status_icon_new ();
    gtk_status_icon_set_from_file (self->priv->icon, SHARE_DIR "/imgs/tray-icon.svg");

    gtk_status_icon_set_has_tooltip (self->priv->icon, TRUE);

    g_signal_connect (self->priv->icon, "activate",
        G_CALLBACK (tray_icon_activate), self);
    g_signal_connect (self->priv->icon, "popup-menu",
        G_CALLBACK (tray_popup), self);
    g_signal_connect (self->priv->icon, "scroll-event",
        G_CALLBACK (tray_scroll_event), self);
    g_signal_connect (self->priv->icon, "query-tooltip",
        G_CALLBACK (tray_query_tooltip), self);
    g_signal_connect (self->priv->icon, "button-press-event",
        G_CALLBACK (tray_button_press), self);

    self->priv->menu = gtk_menu_new ();

    self->priv->play_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PLAY, NULL);
    g_signal_connect (G_OBJECT (self->priv->play_item), "activate", G_CALLBACK (tray_play), self);
    gtk_menu_append (self->priv->menu, self->priv->play_item);

    self->priv->paused_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PAUSE, NULL);
    g_signal_connect (G_OBJECT (self->priv->paused_item), "activate", G_CALLBACK (tray_pause), self);
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

    Player *p = shell_get_player (self->priv->shell);
    g_signal_connect (p, "state-changed", G_CALLBACK (tray_state_changed), self);
    g_signal_connect (p, "pos-changed", G_CALLBACK (tray_pos_changed), self);
}

void
tray_deactivate (Tray *self)
{
    gtk_widget_hide (GTK_WIDGET (self->priv->icon));
}

static gboolean
tray_query_tooltip (GtkStatusIcon *icon,
                    gint x,
                    gint y,
                    gboolean keyboard_mode,
                    GtkTooltip *tooltip,
                    Tray *self)
{
    Player *p = shell_get_player (self->priv->shell);
    guint state = player_get_state (p);
    if (state == PLAYER_STATE_PLAYING || state == PLAYER_STATE_PAUSED) {
        Entry *e = player_get_entry (p);

//        GdkPixbuf *img = gdk_pixbuf_new_from_file_at_scale (entry_get_art (e), 50, 50, TRUE, NULL);
//        if (img == NULL) {
//            img = gdk_pixbuf_new_from_file_at_scale (SHARE_DIR "/imgs/rhythmbox-missing-artwork.svg", 50, 50, TRUE, NULL);
//        }

        gtk_tooltip_set_icon (tooltip, self->priv->img);
//        g_object_unref (img);

        const gchar *title = entry_get_tag_str (e, "title");
        const gchar *artist = entry_get_tag_str (e, "artist");
        const gchar *album = entry_get_tag_str (e, "album");
        gchar *str;

        gchar *pos = time_to_string (player_get_position (p));
        gchar *len = time_to_string (player_get_length (p));

        if (artist && album) {
            str = g_strdup_printf ("%s\n\nby %s from %s\n%s of %s", title, artist, album, pos, len);
        } else if (artist) {
            str = g_strdup_printf ("%s\n\nby %s\n%s of %s", title, artist, pos, len);
        } else if (album) {
            str = g_strdup_printf ("%s\n\nfrom %s\n%s of %s", title, album, pos, len);
        } else {
            str = g_strdup_printf ("%s\n\n%s of %s", title, pos, len);
        }

        gtk_tooltip_set_text (tooltip, str);

        g_free (pos);
        g_free (len);
        g_free (str);

        return TRUE;
    } else {
        return FALSE;
    }
}

static gboolean
tray_button_press (GtkStatusIcon *icon,
                   GdkEventButton *event,
                   Tray *self)
{
    if (event->button == 2) {
        Player *p = shell_get_player (self->priv->shell);

        guint state = player_get_state (p);

        if (state == PLAYER_STATE_PLAYING) {
            g_signal_emit (G_OBJECT (self), signal_pause, 0);
        } else {
            g_signal_emit (G_OBJECT (self), signal_play, 0);
        }

        return TRUE;
    }

    return FALSE;
}

static gboolean
tray_scroll_event (GtkStatusIcon *icon,
                   GdkEventScroll *event,
                   Tray *self)
{
    Player *p = shell_get_player (self->priv->shell);

    gdouble vol = player_get_volume (p);

    if (event->direction == GDK_SCROLL_UP) {
        player_set_volume (p, vol + 0.05);
    } else if (event->direction == GDK_SCROLL_DOWN) {
        player_set_volume (p, vol - 0.05);
    }
}

static void
tray_pos_changed (Player *player, guint pos, Tray *self)
{
    gtk_tooltip_trigger_tooltip_query (gdk_display_get_default ());
}
