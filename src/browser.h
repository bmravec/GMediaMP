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

#include <gtk/gtk.h>

#include "shell.h"
#include "entry.h"
#include "media-store.h"

#define BROWSER_TYPE (browser_get_type ())
#define BROWSER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), BROWSER_TYPE, Browser))
#define BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), BROWSER_TYPE, BrowserClass))
#define IS_BROWSER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), BROWSER_TYPE))
#define IS_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BROWSER_TYPE))
#define BROWSER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BROWSER_TYPE, BrowserClass))

G_BEGIN_DECLS

typedef struct _Browser Browser;
typedef struct _BrowserClass BrowserClass;
typedef struct _BrowserPrivate BrowserPrivate;

struct _Browser {
    GtkVPaned parent;

    BrowserPrivate *priv;
};

struct _BrowserClass {
    GtkVPanedClass parent;
};

GType browser_get_type (void);

Browser *browser_new (Shell *shell);
Browser *browser_new_with_model (Shell *shell, MediaStore *store);

void browser_set_model (Browser *self, MediaStore *store);
MediaStore *browser_get_model (Browser *self);

void browser_set_pane1_tag (Browser *self, const gchar *label, const gchar *tag);
gchar *browser_get_pane1_tag (Browser *self);

void browser_set_pane2_tag (Browser *self, const gchar *label, const gchar *tag);
gchar *browser_get_pane2_tag (Browser *self);

void browser_set_pane2_single_mode (Browser *self, gboolean single_mode);
gboolean browser_get_pane2_single_mode (Browser *self);

void browser_set_compare_func (Browser *self, EntryCompareFunc func);

void browser_add_column (Browser *self, const gchar *label, const gchar *tag,
    gboolean expand, GtkTreeCellDataFunc col_func);

G_END_DECLS

#endif /* __BROWSER_H__ */
