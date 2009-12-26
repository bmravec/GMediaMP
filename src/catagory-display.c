/*
 *      catagory-display.c
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

#include "catagory-display.h"
#include <math.h>

G_DEFINE_TYPE (CatagoryDisplay, catagory_display, GTK_TYPE_DRAWING_AREA)

typedef struct _CatagoryEntry CatagoryEntry;
struct _CatagoryEntry {
    gchar *name;
    guint64 size;

    CatagoryEntry *next;
};

struct _CatagoryDisplayPrivate {
    guint64 capacity;

    CatagoryEntry *list;
};

static gdouble reds[10]   = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
static gdouble greens[10] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
static gdouble blues[10] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

static gboolean on_expose_event (CatagoryDisplay *self, GdkEventExpose *event, GtkWidget *widget);

static void
catagory_display_finalize (GObject *object)
{
    CatagoryDisplay *self = CATAGORY_DISPLAY (object);

    G_OBJECT_CLASS (catagory_display_parent_class)->finalize (object);
}

static void
catagory_display_class_init (CatagoryDisplayClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (CatagoryDisplayPrivate));

    object_class->finalize = catagory_display_finalize;
}

static void
catagory_display_init (CatagoryDisplay *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), CATAGORY_DISPLAY_TYPE, CatagoryDisplayPrivate);

    self->priv->capacity = 0;

    self->priv->list = NULL;
}

GtkWidget*
catagory_display_new ()
{
    CatagoryDisplay *self = g_object_new (CATAGORY_DISPLAY_TYPE, NULL);

    g_signal_connect_swapped (self, "expose-event", G_CALLBACK (on_expose_event), self);

    return GTK_WIDGET (self);
}

static gchar*
size_to_string (guint64 size)
{
    static gchar *suf[4] = { "KB", "MB", "GB", "TB" };

    if (size < 1024) {
        return g_strdup_printf ("%ld Bytes", size);
    }

    gint i;
    for (i = 0; i < 4; i++) {
        if (size < 1024 * 1024) {
            return g_strdup_printf ("%d.%02d %s", size / 1024, (size % 1024) / 10, suf[i]);
        }

        size /= 1024;
    }
}

static gboolean
on_expose_event (CatagoryDisplay *self, GdkEventExpose *event, GtkWidget *widget)
{
    cairo_t *cr = gdk_cairo_create (widget->window);

    gdouble radius = 15;
    gint xmid = widget->allocation.width / 2;
    gint ymid = widget->allocation.height / 4;

    cairo_move_to (cr, xmid - 200, ymid - radius);
    cairo_line_to (cr, xmid + 200, ymid - radius);
    cairo_arc (cr, xmid + 200, ymid, radius, -M_PI / 2, M_PI / 2);
    cairo_line_to (cr, xmid - 200, ymid + radius);
    cairo_arc (cr, xmid - 200, ymid, radius, M_PI / 2, -M_PI / 2);
    cairo_clip (cr);

    gint width = 400 + 2 * radius;

    gint i, xpos;
    CatagoryEntry *iter;
    gint lw = width;
    for (i = 0, xpos = xmid - 200 - radius, iter = self->priv->list; iter; i++, iter = iter->next) {
        if (iter->size > 0) {
            gdouble percent = 1.0 * iter->size / self->priv->capacity;
            cairo_set_source_rgba (cr, reds[i], greens[i], blues[i], 1.0);
            cairo_rectangle (cr, xpos, ymid - radius, ceil (lw * percent), 2 * radius);
            cairo_fill (cr);

            xpos += ceil (width * percent);
            lw = lw - ceil (width * percent);
        }
    }

    cairo_reset_clip (cr);

    cairo_pattern_t *grad = cairo_pattern_create_linear (xmid, ymid - radius, xmid, ymid + radius);
    cairo_pattern_add_color_stop_rgba (grad, 0.0, 0.0, 0.0, 0.0, 0.05);
    cairo_pattern_add_color_stop_rgba (grad, 0.4, 0.0, 0.0, 0.0, 0.00);
    cairo_pattern_add_color_stop_rgba (grad, 0.5, 0.0, 0.0, 0.0, 0.00);
    cairo_pattern_add_color_stop_rgba (grad, 1.0, 0.0, 0.0, 0.0, 0.10);

    cairo_set_source (cr, grad);
    cairo_move_to (cr, xmid - 200, ymid - radius);
    cairo_line_to (cr, xmid + 200, ymid - radius);
    cairo_arc (cr, xmid + 200, ymid, radius, -M_PI / 2, M_PI / 2);
    cairo_line_to (cr, xmid - 200, ymid + radius);
    cairo_arc (cr, xmid - 200, ymid, radius, M_PI / 2, -M_PI / 2);
    cairo_fill (cr);

    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.25);
    cairo_set_line_width (cr, 1.0);
    cairo_move_to (cr, xmid - 200, ymid - radius);
    cairo_line_to (cr, xmid + 200, ymid - radius);
    cairo_arc (cr, xmid + 200, ymid, radius, -M_PI / 2, M_PI / 2);
    cairo_line_to (cr, xmid - 200, ymid + radius);
    cairo_arc (cr, xmid - 200, ymid, radius, M_PI / 2, -M_PI / 2);
    cairo_stroke (cr);

    gint x;
    for (x = xmid - 190; x <= xmid + 200; x += 25) {
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.25);
        cairo_move_to (cr, x - 1, ymid - radius);
        cairo_line_to (cr, x - 1, ymid + radius);
        cairo_stroke (cr);

        cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.25);
        cairo_move_to (cr, x, ymid - radius);
        cairo_line_to (cr, x, ymid + radius);
        cairo_stroke (cr);
    }

    cairo_pattern_destroy (grad);
    guint64 tot_size = 0;
    gint ypos = ymid + 2 * radius;

    for (i = 0, xpos = xmid - 200 - radius, iter = self->priv->list; iter; i++, iter = iter->next) {
        if (iter->size > 0) {
            tot_size += iter->size;
            cairo_set_source_rgba (cr, reds[i], greens[i], blues[i], 1.0);
            cairo_rectangle (cr, xpos, ypos, 10, 10);
            cairo_fill (cr);

            cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
            cairo_rectangle (cr, xpos, ypos, 10, 10);
            cairo_stroke (cr);

            xpos += 15;

            gchar *num_str = size_to_string (iter->size);
            gchar *str = g_strdup_printf ("%s (%s)", iter->name, num_str);
            g_free (num_str);

            cairo_move_to (cr, xpos, ypos + 10);
            cairo_show_text (cr, str);

            cairo_text_extents_t extents;
            cairo_text_extents (cr, str, &extents);
            g_free (str);

            xpos += extents.width + 10;

            if (xpos > xmid + 200) {
                ypos += radius;
                xpos = xmid - 200 - radius;
            }
        }
    }

    if (tot_size < self->priv->capacity) {
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
        cairo_rectangle (cr, xpos, ypos, 10, 10);
        cairo_fill (cr);

        cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
        cairo_rectangle (cr, xpos, ypos, 10, 10);
        cairo_stroke (cr);

        xpos += 15;

        gchar *num_str = size_to_string (self->priv->capacity - tot_size);
        gchar *str = g_strdup_printf ("%s (%s)", "Free Space", num_str);
        g_free (num_str);

        cairo_move_to (cr, xpos, ypos + 10);
        cairo_show_text (cr, str);
        g_free (str);
    }

    cairo_destroy (cr);

    return TRUE;
}

void
catagory_display_set_capacity (CatagoryDisplay *self, guint64 size)
{
    self->priv->capacity = size;

    if (self->priv->list) {
        if (GTK_WIDGET_VISIBLE (GTK_WIDGET (self))) {
            gdk_window_invalidate_rect (GTK_WIDGET (self)->window, NULL, TRUE);
        }
    }
}

guint64
catagory_display_get_capacity (CatagoryDisplay *self)
{
    return self->priv->capacity;
}

void
catagory_display_set_catagory (CatagoryDisplay *self, const gchar *catagory, guint64 size)
{
    if (self->priv->list == NULL) {
        self->priv->list = g_new0 (CatagoryEntry, 1);

        self->priv->list->name = g_strdup (catagory);
        self->priv->list->size = size;
    } else {
        CatagoryEntry *iter, *prev;
        for (iter = self->priv->list; iter; prev = iter, iter = iter->next) {
            if (!g_strcmp0 (iter->name, catagory)) {
                iter->size = size;
            }
        }

        if (iter == NULL) {
            prev->next = g_new0 (CatagoryEntry, 1);

            prev->next->name = g_strdup (catagory);
            prev->next->size = size;
        }
    }

    if (GTK_WIDGET_VISIBLE (GTK_WIDGET (self))) {
        gdk_window_invalidate_rect (GTK_WIDGET (self)->window, NULL, TRUE);
    }
}

guint64
catagory_display_get_catagory (CatagoryDisplay *self, const gchar *catagory)
{
    CatagoryEntry *iter;
    for (iter = self->priv->list; iter; iter = iter->next) {
        if (!g_strcmp0 (iter->name, catagory)) {
            return iter->size;
        }
    }

    return 0;
}
