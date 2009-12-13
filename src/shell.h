/*
 *      shell.h
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

#ifndef __SHELL_H__
#define __SHELL_H__

#include <glib-object.h>

#include "progress.h"
#include "track-source.h"
#include "media-store.h"

#define SHELL_TYPE (shell_get_type ())
#define SHELL(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), SHELL_TYPE, Shell))
#define SHELL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SHELL_TYPE, ShellClass))
#define IS_SHELL(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), SHELL_TYPE))
#define IS_SHELL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SHELL_TYPE))
#define SHELL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SHELL_TYPE, ShellClass))

G_BEGIN_DECLS

typedef struct _Shell Shell;
typedef struct _ShellClass ShellClass;
typedef struct _ShellPrivate ShellPrivate;
struct _Player;

struct _Shell {
    GObject parent;

    ShellPrivate *priv;
};

struct _ShellClass {
    GObjectClass parent;
};

Shell *shell_new ();
GType shell_get_type (void);

gboolean shell_add_widget (Shell *self, GtkWidget *widget, gchar *name, gchar *icon);
gboolean shell_remove_widget (Shell *self, gchar *name);

gboolean shell_select_widget (Shell *self, GtkWidget *widget);
gboolean shell_select_path (Shell *self, const gchar *name);

GtkBuilder *shell_get_builder (Shell *self);

gboolean shell_add_progress (Shell *self, Progress *p);
gboolean shell_remove_progress (Shell *self, Progress *p);

gboolean shell_register_track_source (Shell *self, TrackSource *ts);
gboolean shell_register_media_store (Shell *self, MediaStore *ms);

gboolean shell_import_path (Shell *self, const gchar *path, const gchar *media_type);

struct _Player *shell_get_player (Shell *self);

void shell_run (Shell *self);
void shell_quit (Shell *self);

void shell_toggle_visibility (Shell *self);
void shell_hide (Shell *self);
void shell_show (Shell *self);

gchar **shell_get_media_stores (Shell *self);
gboolean shell_move_to (Shell *self, gchar **e, const gchar *ms_name);

G_END_DECLS

#endif /* __SHELL_H__ */
