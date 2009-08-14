/*
 *      progress.h
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

#ifndef __PROGRESS_H__
#define __PROGRESS_H__

#include <gtk/gtk.h>

#define PROGRESS_TYPE (progress_get_type ())
#define PROGRESS(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), PROGRESS_TYPE, Progress))
#define PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PROGRESS_TYPE, ProgressClass))
#define IS_PROGRESS(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PROGRESS_TYPE))
#define IS_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PROGRESS_TYPE))
#define PROGRESS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PROGRESS_TYPE, ProgressClass))

G_BEGIN_DECLS

typedef struct _Progress Progress;
typedef struct _ProgressClass ProgressClass;
typedef struct _ProgressPrivate ProgressPrivate;

struct _Progress {
    GObject parent;

    ProgressPrivate *priv;
};

struct _ProgressClass {
    GObjectClass parent;
};

Progress *progress_new (const gchar *label);
GType progress_get_type (void);

void progress_set_text (Progress *self, const gchar *text);
void progress_set_percent (Progress *self, gdouble frac);

GtkWidget *progress_get_widget ();

G_END_DECLS

#endif /* __PROGRESS_H__ */
