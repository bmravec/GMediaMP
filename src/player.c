/*
 *      player.c
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

#include <gst/gst.h>
#include <gtk/gtk.h>

#include "player.h"
#include "shell.h"

G_DEFINE_TYPE(Player, player, G_TYPE_OBJECT)

struct _PlayerPrivate {
    GstElement *pipeline;
    GstTagList *songtags;
    gdouble volume;
    guint state;

    Shell *shell;
    GtkWidget *widget;
};

guint signal_eos;
guint signal_tag;
guint signal_state;
guint signal_ratio;

static gdouble prev_ratio;

static gboolean
position_update (Player *self)
{
    guint len = player_get_length (self);
    guint pos = player_get_position (self);
    gdouble new_ratio = (gdouble) pos / (gdouble) len;

    if (prev_ratio != new_ratio) {
        prev_ratio = new_ratio;
        switch (self->priv->state) {
            case PLAYER_STATE_PLAYING:
            case PLAYER_STATE_PAUSED:
                g_signal_emit (self, signal_ratio, 0, prev_ratio);
                break;
            default:
                g_signal_emit (self, signal_ratio, 0, 0.0);
        }
    }

    if (self->priv->state != PLAYER_STATE_PLAYING) {
        return FALSE;
    }

    return TRUE;
}

static void
player_set_state (Player *self, guint state)
{
    self->priv->state = state;
    g_signal_emit (self, signal_state, 0, state);
}

static gboolean
player_bus_call(GstBus *bus, GstMessage *msg, gpointer user_data)
{
    Player *self = PLAYER (user_data);
    GstTagList *taglist;
    gchar *debug;
    GError *err;

    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_EOS:
            player_stop (self);

            g_signal_emit (self, signal_eos, 0, NULL);
            break;
        case GST_MESSAGE_ERROR:
            player_stop (self);
            gst_message_parse_error (msg, &err, &debug);
            g_free (debug);

            g_print ("Error: %s\n", err->message);
            g_error_free (err);

            player_close (self);
            break;
        case GST_MESSAGE_TAG:
            gst_message_parse_tag (msg,&taglist);
            gst_tag_list_insert(self->priv->songtags, taglist,
                GST_TAG_MERGE_REPLACE);

            g_signal_emit (self, signal_tag, 0, NULL);

            gst_tag_list_free(taglist);
            break;
        case GST_MESSAGE_BUFFERING:

            break;
        case GST_MESSAGE_ELEMENT:

            break;
        default:
            break;
    }

    return TRUE;
}

static void
player_finalize (GObject *object)
{
    Player *self = PLAYER (object);

    G_OBJECT_CLASS (player_parent_class)->finalize (object);
}

static void
player_class_init (PlayerClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (PlayerPrivate));

    object_class->finalize = player_finalize;

    signal_eos = g_signal_new ("eos", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    signal_tag = g_signal_new ("tag", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    signal_state = g_signal_new ("state-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);

    signal_ratio = g_signal_new ("ratio-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__DOUBLE,
        G_TYPE_NONE, 1, G_TYPE_DOUBLE);
}

static void
player_init (Player *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), PLAYER_TYPE, PlayerPrivate);

    self->priv->pipeline = NULL;
    self->priv->songtags = NULL;
    self->priv->state = PLAYER_STATE_NULL;

    self->priv->widget = gtk_drawing_area_new ();
}

Player*
player_new (int argc, char *argv[])
{
    gst_init (&argc, &argv);
    return g_object_new (PLAYER_TYPE, NULL);
}

void
player_load (Player *self, gchar *uri)
{
    if (self->priv->pipeline)
        player_close (self);

    self->priv->pipeline = gst_element_factory_make ("playbin", NULL);

    gchar *ruri = g_strdup_printf ("file://%s", uri);
    g_object_set (G_OBJECT (self->priv->pipeline),
        "uri", ruri,
        "volume", self->priv->volume,
        NULL);
    g_free (ruri);

    gst_object_ref (self->priv->pipeline);

    GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (self->priv->pipeline));
    gst_bus_add_watch (bus, player_bus_call, self);
    gst_object_unref (bus);

    self->priv->songtags = gst_tag_list_new ();
}

void
player_close (Player *self)
{
    player_stop (self);

    if (self->priv->pipeline) {
        gst_element_set_state (self->priv->pipeline, GST_STATE_NULL);
        gst_object_unref (GST_OBJECT (self->priv->pipeline));
        self->priv->pipeline = NULL;
    }

    if (self->priv->songtags) {
        gst_tag_list_free (self->priv->songtags);
        self->priv->songtags = NULL;
    }
}

void
player_play (Player *self)
{
    if (self->priv->pipeline) {
        gst_element_set_state (self->priv->pipeline, GST_STATE_PLAYING);
        player_set_state (self, PLAYER_STATE_PLAYING);

        g_timeout_add (1000, (GSourceFunc) position_update, self);
    }
}

void
player_pause (Player *self)
{
    if (self->priv->pipeline) {
        gst_element_set_state (self->priv->pipeline, GST_STATE_PAUSED);
        player_set_state (self, PLAYER_STATE_PAUSED);
    }
}

void
player_stop (Player *self)
{
    if (self->priv->pipeline) {
        gst_element_set_state (self->priv->pipeline, GST_STATE_NULL);
        player_set_state (self, PLAYER_STATE_STOPPED);
    }

    g_signal_emit (self, signal_ratio, 0, 0.0);
}

guint
player_get_state (Player *self)
{
    return self->priv->state;
}

guint
player_get_length (Player *self)
{
    if (self->priv->pipeline) {
        GstFormat fmt = GST_FORMAT_TIME;
        gint64 len;
        gst_element_query_duration (self->priv->pipeline, &fmt, &len);
        return GST_TIME_AS_SECONDS (len);
    } else {
        return 0;
    }
}

guint
player_get_position (Player *self)
{
    if (self->priv->pipeline) {
        GstFormat fmt = GST_FORMAT_TIME;
        gint64 pos;
        gst_element_query_position (self->priv->pipeline, &fmt, &pos);
        return GST_TIME_AS_SECONDS (pos);
    } else {
        return 0;
    }
}

void
player_set_position (Player *self, guint pos)
{
    if (!self->priv->pipeline) {
        g_signal_emit (self, signal_ratio, 0, 0.0);
        return;
    }

    gst_element_seek_simple (self->priv->pipeline, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, GST_SECOND * pos);

    guint len = player_get_length (self);
    gdouble new_ratio = (gdouble) pos / (gdouble) len;

    g_signal_emit (self, signal_ratio, 0, new_ratio);
}

gdouble
player_get_volume (Player *self)
{
    return self->priv->volume;
}

void
player_set_volume (Player *self, gdouble vol)
{
    self->priv->volume = vol;

    if (vol > 1.0) vol = 1.0;
    if (vol < 0.0) vol = 0.0;

    if (self->priv->pipeline) {
        g_object_set (self->priv->pipeline, "volume", vol, NULL);
    }
}

gboolean
player_activate (Player *self)
{
    self->priv->shell = shell_new ();

    shell_add_widget (self->priv->shell, gtk_label_new ("NOW PLAYING"), "Now Playing", NULL);
//    shell_add_widget (self->priv->shell, self->priv->widget, "Now Playing", NULL);

    gtk_widget_show_all (self->priv->widget);
}

gboolean
player_deactivate (Player *self)
{

}
