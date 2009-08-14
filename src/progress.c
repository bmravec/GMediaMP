/*
 *      progress.c
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

#include <gst/gst.h>

#include "progress.h"

G_DEFINE_TYPE(Progress, progress, G_TYPE_OBJECT)

struct _ProgressPrivate {
    GtkWidget *widget;
    GtkWidget *label;
    GtkWidget *progress_bar;
    GtkWidget *button;
};

guint signal_cancel;

static void
on_button_press (GtkWidget *widget, Progress *self)
{
    g_signal_emit (self, signal_cancel, 0);
}

static void
progress_finalize (GObject *object)
{
    Progress *self = PROGRESS (object);

    G_OBJECT_CLASS (progress_parent_class)->finalize (object);
}

static void
progress_class_init (ProgressClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (ProgressPrivate));

    object_class->finalize = progress_finalize;

    signal_cancel = g_signal_new ("cancel", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}

static void
progress_init (Progress *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), PROGRESS_TYPE, ProgressPrivate);

    GtkWidget *hbox;

    hbox = gtk_hbox_new (FALSE, 0);
    self->priv->widget = gtk_vbox_new (FALSE, 0);

    self->priv->label = gtk_label_new ("");
    self->priv->button = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (self->priv->button),
        gtk_image_new_from_stock (GTK_STOCK_STOP, GTK_ICON_SIZE_MENU));
    self->priv->progress_bar = gtk_progress_bar_new ();

    gtk_box_pack_start (GTK_BOX (hbox), self->priv->progress_bar, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), self->priv->button, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (self->priv->widget), self->priv->label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (self->priv->widget), hbox, FALSE, FALSE, 0);

    gtk_widget_show_all (self->priv->widget);

    g_signal_connect (G_OBJECT (self->priv->button), "clicked",
        G_CALLBACK (on_button_press), self);
}

Progress*
progress_new (const gchar *name)
{
    Progress *self = g_object_new (PROGRESS_TYPE, NULL);

    gtk_label_set_text (GTK_LABEL (self->priv->label), name);

    return self;
}

GtkWidget*
progress_get_widget (Progress *self)
{
    return self->priv->widget;
}

void
progress_set_text (Progress *self, const gchar *text)
{
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (self->priv->progress_bar),
        text);
}

void
progress_set_percent (Progress *self, gdouble frac)
{
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->priv->progress_bar),
        frac);
}
