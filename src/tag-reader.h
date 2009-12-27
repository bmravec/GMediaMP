/*
 *      tag-reader.h
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

#ifndef __TAG_READER_H__
#define __TAG_READER_H__

#include <glib-object.h>

#define TAG_READER_TYPE (tag_reader_get_type ())
#define TAG_READER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), TAG_READER_TYPE, TagReader))
#define TAG_READER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TAG_READER_TYPE, TagReaderClass))
#define IS_TAG_READER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), TAG_READER_TYPE))
#define IS_TAG_READER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TAG_READER_TYPE))
#define TAG_READER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TAG_READER_TYPE, TagReaderClass))

G_BEGIN_DECLS

typedef struct _TagReader TagReader;
typedef struct _TagReaderClass TagReaderClass;
typedef struct _TagReaderPrivate TagReaderPrivate;

struct _TagReader {
    GObject parent;

    TagReaderPrivate *priv;
};

struct _TagReaderClass {
    GObjectClass parent;
};

GType tag_reader_get_type (void);
TagReader *tag_reader_new (Shell *shell);

gchar **tag_reader_get_tags (TagReader *self, const gchar *location, gboolean *has_video);
void tag_reader_queue_entry (TagReader *self, const gchar *location, const gchar *media_type);

G_END_DECLS

#endif /* __TAG_READER_H__ */
