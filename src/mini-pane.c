/*
 *      mini-pane.c
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

#include "mini-pane.h"

#include <gtk/gtk.h>

#include "shell.h"
#include "player.h"

G_DEFINE_TYPE(MiniPane, mini_pane, GTK_TYPE_DRAWING_AREA)

struct _MiniPanePrivate {
    Shell *shell;

    gint width;

    GdkPixbuf *img;
    gchar *art_file;
    gint img_width, img_height;
};

static void on_size_allocate (GtkWidget *widget, GtkAllocation *alloc, MiniPane *self);
static void on_size_requisition (GtkWidget *widget, GtkRequisition *req, MiniPane *self);
static void on_player_state (Player *player, gint state, MiniPane *self);
static gboolean on_expose (GtkWidget *widget, GdkEventExpose *event, MiniPane *self);

static guint signal_changed;

static void
mini_pane_finalize (GObject *object)
{
    MiniPane *self = MINI_PANE (object);

    G_OBJECT_CLASS (mini_pane_parent_class)->finalize (object);
}

static void
mini_pane_class_init (MiniPaneClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (MiniPanePrivate));

    object_class->finalize = mini_pane_finalize;

    signal_changed = g_signal_new ("mini_pane-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}

static void
mini_pane_init (MiniPane *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), MINI_PANE_TYPE, MiniPanePrivate);

    self->priv->width = -1;
}

GtkWidget*
mini_pane_new ()
{
    return g_object_new (MINI_PANE_TYPE, NULL);
}

gboolean
mini_pane_activate (MiniPane *self)
{
    self->priv->shell = shell_new ();

    g_signal_connect (self, "size-allocate", G_CALLBACK (on_size_allocate), self);
    g_signal_connect (self, "size-request", G_CALLBACK (on_size_requisition), self);
    g_signal_connect (self, "expose-event", G_CALLBACK (on_expose), self);

    Player *p = shell_get_player (self->priv->shell);

    g_signal_connect (p, "state-changed", G_CALLBACK (on_player_state), self);
}

gboolean
mini_pane_deactivate (MiniPane *self)
{

}

static void
on_size_allocate (GtkWidget *widget, GtkAllocation *alloc, MiniPane *self)
{
//    g_print ("Size Allocate (%d,%d)\n", alloc->width, alloc->height);

    if (alloc->width != alloc->height) {
        self->priv->width = alloc->width;
        gtk_widget_set_size_request (widget, alloc->width, alloc->width);
        gtk_widget_queue_resize (widget);

        if (self->priv->img) {
            g_object_unref (self->priv->img);

            self->priv->img = gdk_pixbuf_new_from_file_at_scale (
                self->priv->art_file,
                self->priv->width, self->priv->width, TRUE, NULL);

            g_object_get (self->priv->img,
                "width", &self->priv->img_width,
                "height", &self->priv->img_height,
                NULL);
        }
    }
}

static void
on_size_requisition (GtkWidget *widget, GtkRequisition *req, MiniPane *self)
{
//    g_print ("Size Requisition (%d,%d)\n", req->width, req->height);

    if (self->priv->width != -1) {
        req->height = req->width = self->priv->width;
    } else {
        self->priv->width = req->height = req->width;
    }
}

static void
on_player_state (Player *player, gint state, MiniPane *self)
{
    if (state == PLAYER_STATE_PLAYING && self->priv->img == NULL) {
        Entry *e = player_get_entry (player);

        if (entry_get_media_type (e) == MEDIA_SONG) {

            self->priv->art_file = entry_get_art (e);

            self->priv->img = gdk_pixbuf_new_from_file_at_scale (
                self->priv->art_file,
                self->priv->width, self->priv->width, TRUE, NULL);

            g_object_get (self->priv->img,
                "width", &self->priv->img_width,
                "height", &self->priv->img_height,
                NULL);

            gtk_widget_queue_draw (GTK_WIDGET (self));
        }
    }

    if ((state == PLAYER_STATE_STOPPED || state == PLAYER_STATE_NULL) &&
        self->priv->img) {
        g_object_unref (self->priv->img);
        self->priv->img = NULL;

        g_free (self->priv->art_file);

        gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

static gboolean
on_expose (GtkWidget *widget, GdkEventExpose *event, MiniPane *self)
{
    if (self->priv->img) {
        gdk_draw_pixbuf (widget->window, widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
            self->priv->img, 0, 0, 0, 0, self->priv->img_width, self->priv->img_height,
            GDK_RGB_DITHER_NONE, 0, 0);
        return TRUE;
    }

    return FALSE;
}
