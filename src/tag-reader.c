/*
 *      tag-reader.c
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

#include "../config.h"

#include "shell.h"
#include "progress.h"

#include "tag-reader.h"
#include "media-store.h"

static void media_store_init (MediaStoreInterface *iface);
gpointer tag_reader_main (TagReader *self);

G_DEFINE_TYPE_WITH_CODE (TagReader, tag_reader, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MEDIA_STORE_TYPE, media_store_init)
)

typedef struct {
    gchar *location;
    gchar *mtype;
} QueueEntry;

struct _TagReaderPrivate {
    Shell *shell;
    Progress *p;

    gint total, done;

    GAsyncQueue *job_queue;
    GThread *job_thread;

    gboolean run;
};

static void
media_store_init (MediaStoreInterface *iface)
{
    iface->add_entry = NULL;
    iface->rem_entry = NULL;
    iface->get_mtype = NULL;
}

static void
tag_reader_finalize (GObject *object)
{
    TagReader *self = TAG_READER (object);

    self->priv->run = FALSE;

    while (g_async_queue_length (self->priv->job_queue) > 0) {
        QueueEntry *entry = g_async_queue_try_pop (self->priv->job_queue);

        if (!entry) {
            continue;
        }

        if (entry->location) {
            g_free (entry->location);
        }
        if (entry->mtype) {
            g_free (entry->mtype);
        }
        g_free (entry);
    }

    g_async_queue_push (self->priv->job_queue, NULL);
    g_thread_join (self->priv->job_thread);

    g_async_queue_unref (self->priv->job_queue);

    G_OBJECT_CLASS (tag_reader_parent_class)->finalize (object);
}

static void
tag_reader_class_init (TagReaderClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (TagReaderPrivate));

    object_class->finalize = tag_reader_finalize;

    av_register_all();
}

static void
tag_reader_init (TagReader *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, TAG_READER_TYPE, TagReaderPrivate);

    self->priv->job_queue = g_async_queue_new ();
    self->priv->run = TRUE;

    self->priv->shell = NULL;
    self->priv->p = NULL;
}

TagReader*
tag_reader_new (Shell *shell) {
    TagReader *self = g_object_new (TAG_READER_TYPE, NULL);

    self->priv->shell = g_object_ref (shell);

    self->priv->job_thread =
        g_thread_create ((GThreadFunc) tag_reader_main, self, TRUE, NULL);

    return self;
}

gpointer
tag_reader_main (TagReader *self)
{
    while (self->priv->run) {
        QueueEntry *entry;

        if (self->priv->p) {
            if (g_async_queue_length (self->priv->job_queue) == 0) {
                shell_remove_progress (self->priv->shell, self->priv->p);
                g_object_unref (self->priv->p);
                self->priv->p = NULL;
                self->priv->total = 0;
                self->priv->done = 0;
            } else {
                gchar *new_str = g_strdup_printf ("%d of %d",
                    self->priv->done, self->priv->total);
                progress_set_text (self->priv->p, new_str);
                progress_set_percent (self->priv->p,
                    (gdouble) self->priv->done / (gdouble) self->priv->total);
                g_free (new_str);
            }
        }

        entry = g_async_queue_pop (self->priv->job_queue);

        if (!entry || !self->priv->run)
            continue;

        if (!self->priv->p) {
            self->priv->p = progress_new ("Importing...");
            shell_add_progress (self->priv->shell, self->priv->p);
            gchar *new_str = g_strdup_printf ("%d of %d", self->priv->done, self->priv->total);
            progress_set_text (self->priv->p, new_str);
            progress_set_percent (self->priv->p,
                (gdouble) self->priv->done / (gdouble) self->priv->total);
            g_free (new_str);
        }

        gboolean has_video;
        gchar **kvs = tag_reader_get_tags (self, entry->location, &has_video);

        if (kvs) {
            if (entry->mtype) {
                shell_move_to (self->priv->shell, kvs, entry->mtype);
            } else {
                if (has_video) {
                    shell_move_to (self->priv->shell, kvs, "Movies");
                } else {
                    shell_move_to (self->priv->shell, kvs, "Music");
                }
            }

            g_strfreev (kvs);
            kvs = NULL;
        }

        if (entry->location) {
            g_free (entry->location);
        }
        if (entry->mtype) {
            g_free (entry->mtype);
        }
        g_free (entry);

        self->priv->done++;
    }
}

void
tag_reader_queue_entry (TagReader *self,
                        const gchar *location,
                        const gchar *media_type)
{
    QueueEntry *qe = g_new0 (QueueEntry, 1);

    qe->location = g_strdup (location);
    if (media_type) {
        qe->mtype = g_strdup (media_type);
    }

    g_async_queue_push (self->priv->job_queue, qe);

    self->priv->total++;
}

#ifdef USE_TAG_READER_AVCODEC
#include <libavformat/avformat.h>
struct AVMetadata {
    int count;
    AVMetadataTag *elems;
};

gchar**
tag_reader_av_get_tags (TagReader *self, const gchar *location, gboolean *has_video)
{
    AVFormatContext *fmt_ctx;
    gint i, j;

    if (av_open_input_file (&fmt_ctx, location, NULL, 0, NULL) != 0) {
        return NULL;
    }

    if (!g_strcmp0 (fmt_ctx->iformat->name, "image2")) {
        av_close_input_file (fmt_ctx);
        return NULL;
    }

    if (av_find_stream_info (fmt_ctx) < 0) {
        av_close_input_file (fmt_ctx);
        return NULL;
    }

    if (has_video) {
        *has_video = FALSE;
        for (i = 0; i < fmt_ctx->nb_streams; i++) {
            if (fmt_ctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
                *has_video = TRUE;
                break;
            }
        }
    }

    AVMetadata *md = fmt_ctx->metadata;
    gint count = md ? 2 * md->count + 7 : 7;

    gchar **tags = g_new0 (gchar*, count);
    gboolean has_title = FALSE;
    gchar *title = NULL;

    i = 0;
    if (md) {
        for (; i < md->count; i++) {
            tags[2*i] = g_ascii_strdown (md->elems[i].key, -1);
            tags[2*i+1] = g_strdup (md->elems[i].value);

            if (!g_strcmp0 (tags[2*i], "title")) {
                has_title = TRUE;
            }
        }
    }

    tags[2*i] = g_strdup ("duration");
    tags[2*i+1] = g_strdup_printf ("%d", (gint) (fmt_ctx->duration / AV_TIME_BASE));
    tags[2*i+2] = g_strdup ("location");
    tags[2*i+3] = g_strdup (location);

    // If the file does not have a title field, lets make one up
    if (!has_title) {
        title = g_path_get_basename (location);
        j = 0;
        while (title[j++]);
        for (; j > 0; j--) {
            if (title[j] == '.') {
                title[j] = '\0';
                break;
            }
        }

        tags[2*i+4] = g_strdup ("title");
        tags[2*i+5] = title;
    }

    return tags;
}
#endif

gchar**
tag_reader_get_tags (TagReader *self, const gchar *location, gboolean *has_video)
{
#ifdef USE_TAG_READER_AVCODEC
    return tag_reader_av_get_tags (self, location, has_video);
#else
    return NULL;
#endif
}
