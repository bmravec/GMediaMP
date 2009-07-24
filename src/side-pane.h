/*
 *      side-pane.h
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
 
#ifndef _SIDE_PANE_H_
#define _SIDE_PANE_H_

#include <gtk/gtkhpaned.h>

#define SIDE_PANE_TYPE (side_pane_get_type ())
#define SIDE_PANE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), SIDE_PANE_TYPE, SidePane))
#define SIDE_PANE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SIDE_PANE_TYPE, SidePaneClass))
#define IS_SIDE_PANE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), SIDE_PANE_TYPE))
#define IS_SIDE_PANE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SIDE_PANE_TYPE))
#define SIDE_PANE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SIDE_PANE_TYPE, SidePaneClass))

G_BEGIN_DECLS

typedef struct _SidePane SidePane;
typedef struct _SidePaneClass SidePaneClass;
typedef struct _SidePanePrivate SidePanePrivate;

struct _SidePane {
    GtkHPaned parent;
    SidePanePrivate *priv;
};

struct _SidePaneClass {
    GtkHPanedClass parent_class;
};

GtkWidget *side_pane_new ();
GType side_pane_get_type (void);
void side_pane_add (SidePane *self, GtkWidget *widget, const char *name);

void side_pane_add_bar_widget (SidePane *self, GtkWidget *widget);

G_END_DECLS

#endif

