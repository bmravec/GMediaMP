/*
 *      browser.h
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

#ifndef __BROWSER_H__
#define __BROWSER_H__

#include <glib-object.h>

#include "entry.h"

#define BROWSER_TYPE (browser_get_type ())
#define BROWSER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), BROWSER_TYPE, Browser))
#define BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), BROWSER_TYPE, BrowserClass))
#define IS_BROWSER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), BROWSER_TYPE))
#define IS_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BROWSER_TYPE))
#define BROWSER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BROWSER_TYPE, BrowserClass))

typedef gint (*BrowserCompareFunc) (gpointer a, gpointer b);

G_BEGIN_DECLS

typedef struct _Browser Browser;
typedef struct _BrowserClass BrowserClass;
typedef struct _BrowserPrivate BrowserPrivate;

struct _Browser {
    GObject parent;

    BrowserPrivate *priv;
};

struct _BrowserClass {
    GObjectClass parent;
};

Browser *browser_new (gchar *media_type,
                      gint mtype,
                      gchar *p1_tag,
                      gchar *p2_tag,
                      gboolean p2_single,
                      BrowserCompareFunc cmp_func,
                      ...);
GType browser_get_type (void);

GtkWidget *browser_get_widget (Browser *self);

G_END_DECLS

#endif /* __BROWSER_H__ */
