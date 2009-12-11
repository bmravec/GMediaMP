/*
 *      mini-pane.h
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

#ifndef __MINI_PANE_H__
#define __MINI_PANE_H__

#include <gtk/gtkdrawingarea.h>
#include "shell.h"

#define MINI_PANE_TYPE (mini_pane_get_type ())
#define MINI_PANE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MINI_PANE_TYPE, MiniPane))
#define MINI_PANE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MINI_PANE_TYPE, MiniPaneClass))
#define IS_MINI_PANE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MINI_PANE_TYPE))
#define IS_MINI_PANE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MINI_PANE_TYPE))
#define MINI_PANE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MINI_PANE_TYPE, MiniPaneClass))

G_BEGIN_DECLS

typedef struct _MiniPane MiniPane;
typedef struct _MiniPaneClass MiniPaneClass;
typedef struct _MiniPanePrivate MiniPanePrivate;

struct _MiniPane {
    GtkDrawingArea parent;

    MiniPanePrivate *priv;
};

struct _MiniPaneClass {
    GtkDrawingAreaClass parent;
};

GtkWidget *mini_pane_new (Shell *shell);
GType mini_pane_get_type (void);

G_END_DECLS

#endif /* __MINI_PANE_H__ */
