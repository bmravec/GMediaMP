/*
 *      tag-handler.c
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

#include "shell.h"
#include "progress.h"

#include "tag-handler.h"
#include "media-store.h"
#include "entry.h"

#include <tag_c.h>

static void media_store_init (MediaStoreInterface *iface);
G_DEFINE_TYPE_WITH_CODE (TagHandler, tag_handler, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MEDIA_STORE_TYPE, media_store_init)
)

#define TAG_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), TAG_HANDLER_TYPE, TagHandlerPrivate))

struct _TagHandlerPrivate
{
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

gchar *
g_strdup0 (gchar *str)
{
    return str ? g_strdup (str) : g_strdup ("");
}

gpointer
tag_handler_main (TagHandler *self)
{
    const TagLib_AudioProperties *properties;
    TagLib_File *file;
    TagLib_Tag *tag;
    Entry *ne;

    gboolean has_ref = FALSE;

    taglib_set_strings_unicode(TRUE);

    while (self->priv->run) {
        gchar *entry;

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

        ne = entry_new (-1);

        file = taglib_file_new(entry);
        if (!file) {
            entry_set_tag_str (ne, "location", entry);
            gchar *title = g_path_get_basename (entry);
            gint i = 0;
            while (title[i++]);
            for (;i > 0;i--) {
                if (title[i] == '.') {
                    title[i] = '\0';
                    break;
                }
            }

            if (i == 0) {
                g_free (title);
                g_object_unref (ne);
                continue;
            }

            entry_set_tag_str (ne, "title", title);

            gchar *ext = &title[i+1];

            if (!g_strcmp0 (ext, "ogg") || !g_strcmp0 (ext, "ogv") ||
                !g_strcmp0 (ext, "avi") || !g_strcmp0 (ext, "mov") ||
                !g_strcmp0 (ext, "mp4") || !g_strcmp0 (ext, "m4v") ||
                !g_strcmp0 (ext, "mkv") || !g_strcmp0 (ext, "ogm")) {
                entry_set_media_type (ne, MEDIA_MOVIE);
                media_store_emit_move (MEDIA_STORE (self), ne);
            }

            self->priv->done++;

            g_object_unref (ne);
            ne = NULL;
            continue;
        }

        tag = taglib_file_tag(file);
        properties = taglib_file_audioproperties(file);

        entry_set_tag_str (ne, "artist", taglib_tag_artist (tag));
        entry_set_tag_str (ne, "title", taglib_tag_title (tag));
        entry_set_tag_str (ne, "album", taglib_tag_album (tag));
        entry_set_tag_str (ne, "comment", taglib_tag_comment (tag));
        entry_set_tag_str (ne, "genre", taglib_tag_genre (tag));

        gchar *year = g_strdup_printf ("%d", taglib_tag_year (tag));
        entry_set_tag_str (ne, "year", year);
        g_free (year);

        gchar *track = g_strdup_printf ("%d",taglib_tag_track (tag));
        entry_set_tag_str (ne, "track", track);
        g_free (track);

        gchar *duration = g_strdup_printf ("%d", taglib_audioproperties_length (properties));
        entry_set_tag_str (ne, "duration", duration);
        g_free (duration);

        entry_set_tag_str (ne, "location", entry);
        entry_set_media_type (ne, MEDIA_SONG);

        media_store_emit_move (MEDIA_STORE (self), ne);

        g_object_unref (ne);
        ne = NULL;

        self->priv->done++;

        taglib_tag_free_strings();
        taglib_file_free(file);
    }
}

static void
tag_handler_finalize (GObject *object)
{
    TagHandler *self = TAG_HANDLER (object);

    self->priv->run = FALSE;

    while (g_async_queue_length (self->priv->job_queue) > 0) {
        gchar *str = g_async_queue_try_pop (self->priv->job_queue);
        if (str) {
            //TODO: DO SOMETHING WITH QUEUED ITEMS
            g_free (str);
        }
    }

    g_async_queue_push (self->priv->job_queue, "");

    g_thread_join (self->priv->job_thread);

    g_async_queue_unref (self->priv->job_queue);

    G_OBJECT_CLASS (tag_handler_parent_class)->finalize (object);
}

static void
tag_handler_class_init (TagHandlerClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (TagHandlerPrivate));

    object_class->finalize = tag_handler_finalize;
}

static void
tag_handler_init (TagHandler *self)
{
    self->priv = TAG_HANDLER_GET_PRIVATE (self);

    self->priv->job_queue = g_async_queue_new ();
    self->priv->run = TRUE;

    self->priv->shell = NULL;
    self->priv->p = NULL;
}

TagHandler *
tag_handler_new ()
{
    TagHandler *self = g_object_new (TAG_HANDLER_TYPE, NULL);

    return self;
}

gboolean
tag_handler_activate (TagHandler *self)
{
    self->priv->shell = shell_new ();

    self->priv->job_thread =
        g_thread_create ((GThreadFunc) tag_handler_main, self, TRUE, NULL);
}

gboolean
tag_handler_deactivate (TagHandler *self)
{

}

void
tag_handler_add_entry (TagHandler *self, const gchar *location)
{
    g_async_queue_push (self->priv->job_queue, g_strdup (location));

    self->priv->total++;
}
