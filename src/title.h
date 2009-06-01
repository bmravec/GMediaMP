/*
 *      title.h
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

#ifndef __TITLE_H__
#define __TITLE_H__

#include <gtk/gtktreeview.h>

#define TITLE_TYPE (title_get_type ())
#define TITLE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), TITLE_TYPE, Title))
#define TITLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TITLE_TYPE, TitleClass))
#define IS_TITLE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), TITLE_TYPE))
#define IS_TITLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TITLE_TYPE))
#define TITLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TITLE_TYPE, TitleClass))

G_BEGIN_DECLS

typedef struct _Title Title;
typedef struct _TitleClass TitleClass;
typedef struct _TitlePrivate TitlePrivate;

struct _Title {
    GtkTreeView parent;
    
    TitlePrivate *priv;
};

struct _TitleClass {
    GtkTreeViewClass parent;
};

GtkWidget *title_new ();
GType title_get_type (void);

G_END_DECLS

#endif /* __TITLE_H__ */

