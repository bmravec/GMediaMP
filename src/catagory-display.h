/*
 *      catagory-display.h
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

#ifndef __CATAGORY_DISPLAY_H__
#define __CATAGORY_DISPLAY_H__

#include <gtk/gtk.h>

#define CATAGORY_DISPLAY_TYPE (catagory_display_get_type ())
#define CATAGORY_DISPLAY(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), CATAGORY_DISPLAY_TYPE, CatagoryDisplay))
#define CATAGORY_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CATAGORY_DISPLAY_TYPE, CatagoryDisplayClass))
#define IS_CATAGORY_DISPLAY(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), CATAGORY_DISPLAY_TYPE))
#define IS_CATAGORY_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CATAGORY_DISPLAY_TYPE))
#define CATAGORY_DISPLAY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CATAGORY_DISPLAY_TYPE, CatagoryDisplayClass))

G_BEGIN_DECLS

typedef struct _CatagoryDisplay CatagoryDisplay;
typedef struct _CatagoryDisplayClass CatagoryDisplayClass;
typedef struct _CatagoryDisplayPrivate CatagoryDisplayPrivate;

struct _CatagoryDisplay {
    GtkDrawingArea parent;

    CatagoryDisplayPrivate *priv;
};

struct _CatagoryDisplayClass {
    GtkDrawingAreaClass parent;
};

GType catagory_display_get_type (void);

GtkWidget *catagory_display_new ();

void catagory_display_set_capacity (CatagoryDisplay *self, guint64 size);
guint64 catagory_display_get_capacity (CatagoryDisplay *self);

void catagory_display_set_catagory (CatagoryDisplay *self, const gchar *catagory, guint64 size);
guint64 catagory_display_get_catagory (CatagoryDisplay *self, const gchar *catagory);

G_END_DECLS

#endif /* __CATAGORY_DISPLAY_H__ */
