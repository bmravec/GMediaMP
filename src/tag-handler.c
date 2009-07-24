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

#include "tag-handler.h"

#include <tag_c.h>

G_DEFINE_TYPE(TagHandler, tag_handler, G_TYPE_OBJECT)

static guint signal_add_entry;

#define TAG_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), TAG_HANDLER_TYPE, TagHandlerPrivate))

struct _TagHandlerPrivate
{
    GAsyncQueue *job_queue;
    GThread *job_thread;

    gboolean run;
};

void tag_handler_emit_add_signal (TagHandler *self, GHashTable *info);

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
    GHashTable *info;
    
    gboolean has_ref = FALSE;
    
    taglib_set_strings_unicode(TRUE);
    
    while (self->priv->run) {
        gchar *entry;
        
        entry = g_async_queue_pop (self->priv->job_queue);
        
        if (!entry || !self->priv->run)
            continue;
        
        info = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
        
        file = taglib_file_new(entry);
        if (!file) continue;
        
        tag = taglib_file_tag(file);
        properties = taglib_file_audioproperties(file);
        
        g_hash_table_insert (info, g_strdup ("artist"), g_strdup0 (taglib_tag_artist (tag)));
        g_hash_table_insert (info, g_strdup ("title"), g_strdup0 (taglib_tag_title (tag)));
        g_hash_table_insert (info, g_strdup ("album"), g_strdup0 (taglib_tag_album (tag)));
        g_hash_table_insert (info, g_strdup ("comment"), g_strdup0 (taglib_tag_comment (tag)));
        g_hash_table_insert (info, g_strdup ("genre"), g_strdup0 (taglib_tag_genre (tag)));
        g_hash_table_insert (info, g_strdup ("year"), g_strdup_printf ("%d",taglib_tag_year (tag))); 
        g_hash_table_insert (info, g_strdup ("track"), g_strdup_printf ("%d",taglib_tag_track (tag))); 
        g_hash_table_insert (info, g_strdup ("duration"), g_strdup_printf ("%d", taglib_audioproperties_length (properties)));
        g_hash_table_insert (info, g_strdup ("location"), g_strdup0 (entry));
        
        tag_handler_emit_add_signal (self, info);
        
        g_hash_table_unref (info);
        info = NULL;
        
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
    
    signal_add_entry = g_signal_new ("add-entry", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__BOXED,
        G_TYPE_NONE, 1, G_TYPE_HASH_TABLE);
}

static void
tag_handler_init (TagHandler *self)
{
    self->priv = TAG_HANDLER_GET_PRIVATE (self);
    
    self->priv->job_queue = g_async_queue_new ();
    self->priv->run = TRUE;
    self->priv->job_thread =
        g_thread_create ((GThreadFunc) tag_handler_main, self, TRUE, NULL);
}

TagHandler *
tag_handler_new ()
{
    TagHandler *self = g_object_new (TAG_HANDLER_TYPE, NULL);
    
    return self;
}

void
tag_handler_emit_add_signal (TagHandler *self, GHashTable *info)
{
    GHashTableIter iter;
    gpointer key, value;
    g_signal_emit (self, signal_add_entry, 0, info);
}

void
tag_handler_add_entry (TagHandler *self, const gchar *location)
{
    g_async_queue_push (self->priv->job_queue, g_strdup (location));
}

