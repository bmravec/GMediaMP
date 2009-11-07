/*
 *      tag-dialog.h
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

#ifndef __TAG_DIALOG_H__
#define __TAG_DIALOG_H__

#include <glib-object.h>

#include "entry.h"

#define TAG_DIALOG_TYPE (tag_dialog_get_type ())
#define TAG_DIALOG(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), TAG_DIALOG_TYPE, TagDialog))
#define TAG_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TAG_DIALOG_TYPE, TagDialogClass))
#define IS_TAG_DIALOG(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), TAG_DIALOG_TYPE))
#define IS_TAG_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TAG_DIALOG_TYPE))
#define TAG_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TAG_DIALOG_TYPE, TagDialogClass))

G_BEGIN_DECLS

typedef struct _TagDialog TagDialog;
typedef struct _TagDialogClass TagDialogClass;
typedef struct _TagDialogPrivate TagDialogPrivate;

struct _TagDialog {
    GObject parent;

    TagDialogPrivate *priv;
};

struct _TagDialogClass {
    GObjectClass parent;
};

TagDialog *tag_dialog_new ();
GType tag_dialog_get_type (void);

gboolean tag_dialog_add_tags (TagDialog *self, gchar **keys, gchar **vals);

gboolean tag_dialog_show (TagDialog *self);
gboolean tag_dialog_close (TagDialog *self);

G_END_DECLS

#endif /* __TAG_DIALOG_H__ */
