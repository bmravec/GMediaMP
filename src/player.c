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

#include <gdk/gdkx.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/video/gstvideosink.h>

#include "player.h"
#include "shell.h"

G_DEFINE_TYPE(Player, player, G_TYPE_OBJECT)

struct _PlayerPrivate {
    GstElement *pipeline, *vsink;
    GstTagList *songtags;
    gdouble volume;
    guint state;
    gint t_pos;

    Entry *entry;

    Shell *shell;

    StreamData *as, *vs, *ss;

    gint monitor;

    gboolean fullscreen;

    GtkWidget *em_da;

    GtkWidget *fs_win;
    GtkWidget *fs_da;
    GtkWidget *fs_scale;
    GtkWidget *fs_title;
    GtkWidget *fs_vol;
    GtkWidget *fs_time;
    GtkWidget *fs_prev;
    GtkWidget *fs_play;
    GtkWidget *fs_pause;
    GtkWidget *fs_next;
    GtkWidget *fs_vbox;
    GtkWidget *fs_hbox;
};

guint signal_eos;
guint signal_tag;
guint signal_state;
guint signal_pos;
guint signal_volume;

guint signal_play;
guint signal_pause;
guint signal_next;
guint signal_prev;

static gboolean position_update (Player *self);
static gboolean player_button_press (GtkWidget *da, GdkEventButton *event, Player *self);
static void player_set_state (Player *self, guint state);
static gboolean player_bus_call (GstBus *bus, GstMessage *msg, Player *self);
static gboolean on_window_state (GtkWidget *widget, GdkEventWindowState *event, Player *self);
static gboolean handle_expose_cb (GtkWidget *widget, GdkEventExpose *event, Player *self);

static void on_control_prev (GtkWidget *widget, Player *self);
static void on_control_play (GtkWidget *widget, Player *self);
static void on_control_pause (GtkWidget *widget, Player *self);
static void on_control_next (GtkWidget *widget, Player *self);

static gboolean on_pos_change_value (GtkWidget *range, GtkScrollType scroll, gdouble value, Player *self);

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

    signal_volume = g_signal_new ("volume-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__DOUBLE,
        G_TYPE_NONE, 1, G_TYPE_DOUBLE);

    signal_pos = g_signal_new ("pos-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);

    signal_prev = g_signal_new ("previous", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    signal_play = g_signal_new ("play", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    signal_pause = g_signal_new ("pause", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    signal_next = g_signal_new ("next", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}

static void
player_init (Player *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), PLAYER_TYPE, PlayerPrivate);

    self->priv->pipeline = NULL;
    self->priv->songtags = NULL;
    self->priv->state = PLAYER_STATE_NULL;
    self->priv->volume = 1.0;
    self->priv->fullscreen = FALSE;
    self->priv->monitor = 0;
}

Player*
player_new (int argc, char *argv[])
{
    gst_init (&argc, &argv);
    return g_object_new (PLAYER_TYPE, NULL);
}

static gboolean
position_update (Player *self)
{
    guint len = player_get_length (self);
    guint pos = player_get_position (self);

    switch (self->priv->state) {
        case PLAYER_STATE_PLAYING:
        case PLAYER_STATE_PAUSED:
            g_signal_emit (self, signal_pos, 0, pos);
            gtk_range_set_range (GTK_RANGE (self->priv->fs_scale), 0, len);
            gtk_range_set_value (GTK_RANGE (self->priv->fs_scale), pos);
            break;
        default:
            g_signal_emit (self, signal_pos, 0, 0);
            gtk_range_set_range (GTK_RANGE (self->priv->fs_scale), 0, 1);
            gtk_range_set_value (GTK_RANGE (self->priv->fs_scale), 0);
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

Entry*
player_get_entry (Player *self)
{
    return self->priv->entry;
}

static GstBusSyncReply
bus_sync_handler (GstBus *bus, GstMessage *msg, Player *self)
{
    if ((GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ELEMENT) &&
        gst_structure_has_name (msg->structure, "prepare-xwindow-id")) {
        if (self->priv->fullscreen) {
            gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (GST_MESSAGE_SRC (msg)),
                GDK_WINDOW_XID (self->priv->fs_da->window));
        } else {
            gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (GST_MESSAGE_SRC (msg)),
                GDK_WINDOW_XID (self->priv->em_da->window));
        }
    }

    return GST_BUS_PASS;
}

static gboolean
player_bus_call(GstBus *bus, GstMessage *msg, Player *self)
{
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

void
player_load (Player *self, Entry *entry)
{
    if (self->priv->pipeline)
        player_close (self);

    if (self->priv->entry) {
        g_object_unref (self->priv->entry);
        self->priv->entry = NULL;
    }

    self->priv->entry = entry;
    g_object_ref (entry);

    self->priv->pipeline = gst_element_factory_make ("playbin", NULL);
    self->priv->vsink = gst_element_factory_make ("xvimagesink", NULL);

    g_object_set (self->priv->vsink, "force-aspect-ratio", TRUE, NULL);

    gchar *ruri = g_strdup_printf ("file://%s", entry_get_tag_str (entry, "location"));
    g_object_set (G_OBJECT (self->priv->pipeline),
        "video-sink", self->priv->vsink,
        "uri", ruri,
        "volume", self->priv->volume,
        "subtitle-font-desc", "Sans 32",
        NULL);
    g_free (ruri);

    gst_object_ref (self->priv->pipeline);

    GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (self->priv->pipeline));
    gst_bus_add_watch (bus, (GstBusFunc) player_bus_call, self);
    gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, self);
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

    if (self->priv->entry) {
        if (entry_get_state (self->priv->entry) != ENTRY_STATE_MISSING) {
            entry_set_state (self->priv->entry, ENTRY_STATE_NONE);
        }

        g_object_unref (self->priv->entry);
        self->priv->entry = NULL;
    }
}

void
insert_stream_data (StreamData *sd, gint index, const gchar *lang)
{
    gint i = 0;

    for (;;i++) {
        if (sd[i].index == -1) {
            sd[i].index = index;
            sd[i].lang = g_strdup (lang);
            break;
        }
    }
}

void
player_play (Player *self)
{
    if (self->priv->pipeline) {
        gst_element_set_state (self->priv->pipeline, GST_STATE_PLAYING);
        player_set_state (self, PLAYER_STATE_PLAYING);

        entry_set_state (self->priv->entry, ENTRY_STATE_PLAYING);

        g_timeout_add (1000, (GSourceFunc) position_update, self);

        gst_element_get_state (self->priv->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        if (self->priv->as) {
            g_free (self->priv->as);
            self->priv->as = NULL;
        }

        if (self->priv->vs) {
            g_free (self->priv->vs);
            self->priv->vs = NULL;
        }

        if (self->priv->ss) {
            g_free (self->priv->ss);
            self->priv->ss = NULL;
        }

        gint num, i;

        GList *si, *iter;
        g_object_get (self->priv->pipeline,
                      "stream-info", &si,
                      "nstreams", &num, NULL);

        self->priv->as = g_new0 (StreamData, num);
        self->priv->vs = g_new0 (StreamData, num);
        self->priv->ss = g_new0 (StreamData, num);

        for (i = 0; i < num; i++) {
            self->priv->as[i].index = -1;
            self->priv->vs[i].index = -1;
            self->priv->ss[i].index = -1;
        }

        for (i = 0;si; si = si->next, i++) {
            GObject *obj = G_OBJECT (si->data);

            gchar *lang;
            gint stype;

            g_object_get (obj, "language-code", &lang, "type", &stype, NULL);

            switch (stype) {
                case 1:
                    insert_stream_data (self->priv->as, i, lang);
                    break;
                case 2:
                    insert_stream_data (self->priv->vs, i, lang);
                    break;
                case 3:
                    insert_stream_data (self->priv->ss, i, lang);
                    break;
            };
        }
    }
}

void
player_pause (Player *self)
{
    if (self->priv->pipeline) {
        gst_element_set_state (self->priv->pipeline, GST_STATE_PAUSED);
        player_set_state (self, PLAYER_STATE_PAUSED);

        entry_set_state (self->priv->entry, ENTRY_STATE_PAUSED);
    }
}

void
player_stop (Player *self)
{
    if (self->priv->pipeline) {
        gst_element_set_state (self->priv->pipeline, GST_STATE_NULL);
        player_set_state (self, PLAYER_STATE_STOPPED);

        if (entry_get_state (self->priv->entry) != ENTRY_STATE_MISSING) {
            entry_set_state (self->priv->entry, ENTRY_STATE_NONE);
        }
    }

    g_signal_emit (self, signal_pos, 0, 0);
}

StreamData*
player_get_audio_streams (Player *self)
{
    return self->priv->as;
}

StreamData*
player_get_video_streams (Player *self)
{
    return self->priv->vs;
}

StreamData*
player_get_subtitle_streams (Player *self)
{
    return self->priv->ss;
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
        g_signal_emit (self, signal_pos, 0, 0);
        return;
    }

    gst_element_seek_simple (self->priv->pipeline, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, GST_SECOND * pos);

    guint new_pos = player_get_position (self);

    g_signal_emit (self, signal_pos, 0, new_pos);
}

gdouble
player_get_volume (Player *self)
{
    return self->priv->volume;
}

void
player_set_volume (Player *self, gdouble vol)
{
    if (self->priv->volume != vol) {
        if (vol > 1.0) vol = 1.0;
        if (vol < 0.0) vol = 0.0;

        self->priv->volume = vol;

        if (self->priv->pipeline) {
            g_object_set (self->priv->pipeline, "volume", vol, NULL);
        }

        gtk_scale_button_set_value (GTK_SCALE_BUTTON (self->priv->fs_vol), self->priv->volume);

        g_signal_emit (self, signal_volume, 0, vol);
    }
}

void
on_vol_changed (GtkWidget *widget, gdouble val, Player *self)
{
    player_set_volume (self, val);
}

gboolean
player_activate (Player *self)
{
    self->priv->shell = shell_new ();

    self->priv->em_da = gtk_drawing_area_new ();

    GtkBuilder *builder = shell_get_builder (self->priv->shell);

    GError *err = NULL;
    gtk_builder_add_from_file (builder, SHARE_DIR "/ui/player.ui", &err);

    if (err) {
        g_print ("ERROR ADDING: %s", err->message);
        g_error_free (err);
    }

    // Get objects from gtkbuilder
    self->priv->fs_win = GTK_WIDGET (gtk_builder_get_object (builder, "player_win"));
    self->priv->fs_da = GTK_WIDGET (gtk_builder_get_object (builder, "player_da"));
    self->priv->fs_title = GTK_WIDGET (gtk_builder_get_object (builder, "player_title"));
    self->priv->fs_scale = GTK_WIDGET (gtk_builder_get_object (builder, "player_scale"));
    self->priv->fs_vol = GTK_WIDGET (gtk_builder_get_object (builder, "player_vol"));
    self->priv->fs_time = GTK_WIDGET (gtk_builder_get_object (builder, "player_time"));
    self->priv->fs_prev = GTK_WIDGET (gtk_builder_get_object (builder, "player_prev"));
    self->priv->fs_play = GTK_WIDGET (gtk_builder_get_object (builder, "player_play"));
    self->priv->fs_pause = GTK_WIDGET (gtk_builder_get_object (builder, "player_pause"));
    self->priv->fs_next = GTK_WIDGET (gtk_builder_get_object (builder, "player_next"));
    self->priv->fs_vbox = GTK_WIDGET (gtk_builder_get_object (builder, "player_vbox"));
    self->priv->fs_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "player_hbox"));

    // Set initial state
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (self->priv->fs_vol), self->priv->volume);

    gtk_widget_add_events (self->priv->em_da, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events (self->priv->fs_da, GDK_BUTTON_PRESS_MASK);

    g_signal_connect (self->priv->em_da, "button-press-event",
        G_CALLBACK (player_button_press), self);
    g_signal_connect (self->priv->fs_da, "button-press-event",
        G_CALLBACK (player_button_press), self);

    g_signal_connect (self->priv->em_da, "expose-event", G_CALLBACK (handle_expose_cb), self);
    g_signal_connect (self->priv->fs_da, "expose-event", G_CALLBACK (handle_expose_cb), self);

    g_signal_connect (self->priv->fs_prev, "clicked", G_CALLBACK (on_control_prev), self);
    g_signal_connect (self->priv->fs_play, "clicked", G_CALLBACK (on_control_play), self);
    g_signal_connect (self->priv->fs_pause, "clicked", G_CALLBACK (on_control_pause), self);
    g_signal_connect (self->priv->fs_next, "clicked", G_CALLBACK (on_control_next), self);

    g_signal_connect (self->priv->fs_vol, "value-changed", G_CALLBACK (on_vol_changed), self);
    g_signal_connect (self->priv->fs_scale, "change-value", G_CALLBACK (on_pos_change_value), self);

    g_signal_connect (self->priv->fs_win, "window-state-event",
        G_CALLBACK (on_window_state), self);

    shell_add_widget (self->priv->shell, self->priv->em_da, "Now Playing", NULL);

    gtk_widget_show_all (self->priv->em_da);
}

gboolean
player_deactivate (Player *self)
{

}

static void
on_pick_screen (GtkWidget *item, Player *self)
{
    GdkRectangle rect;
    gint num;

    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item))) {
        num = atoi (gtk_menu_item_get_label (GTK_MENU_ITEM (item))) - 1;

        self->priv->monitor = num;

        if (self->priv->fs_win) {
            GdkScreen *screen = gdk_screen_get_default ();
            gdk_screen_get_monitor_geometry (screen, num, &rect);

            gtk_window_move (GTK_WINDOW (self->priv->fs_win), rect.x, rect.y);
        }
    }
}

static void
toggle_fullscreen (GtkWidget *item, Player *self)
{
    GdkRectangle rect;
    GdkScreen *screen = gdk_screen_get_default ();
    gint num = gdk_screen_get_n_monitors (screen);

    if (self->priv->monitor >= num) {
        self->priv->monitor = num-1;
    }

    self->priv->t_pos = player_get_position (self);
    gst_element_set_state (self->priv->pipeline, GST_STATE_NULL);

    if (!self->priv->fullscreen) {
        gdk_screen_get_monitor_geometry (screen, self->priv->monitor, &rect);

        gtk_widget_show_all (self->priv->fs_win);
        self->priv->fullscreen = TRUE;
        gtk_window_fullscreen (GTK_WINDOW (self->priv->fs_win));
        gtk_window_move (GTK_WINDOW (self->priv->fs_win), rect.x, rect.y);
    } else {
        gtk_widget_hide (self->priv->fs_win);
        self->priv->fullscreen = FALSE;
    }
}

static gboolean
on_window_state (GtkWidget *widget,
                 GdkEventWindowState *event,
                 Player *self)
{
    gst_element_set_state (self->priv->pipeline, GST_STATE_PLAYING);
    gst_element_get_state (self->priv->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    player_set_position (self, self->priv->t_pos);
}

static gboolean
player_on_as_change (Player *self, GtkWidget *item)
{
    gint ni = atoi (gtk_menu_item_get_label (GTK_MENU_ITEM (item))) - 1;

    gst_element_set_state (self->priv->pipeline, GST_STATE_NULL);

//    g_print ("set current-audio to %d\n", self->priv->as[ni].index);
//    g_object_set (self->priv->pipeline, "current-audio", self->priv->as[ni].index, NULL);
    g_print ("set current-audio to %d\n", ni);
    g_object_set (self->priv->pipeline, "current-audio", ni, NULL);

//    gst_element_set_state (self->priv->pipeline, GST_STATE_NULL);

//    gst_element_set_state (self->priv->pipeline, GST_STATE_PLAYING);
}

static gboolean
player_on_vs_change (Player *self, GtkWidget *item)
{
    gint ni = atoi (gtk_menu_item_get_label (GTK_MENU_ITEM (item))) - 1;

//    gst_element_set_state (self->priv->pipeline, GST_STATE_NULL);

    g_print ("set current-video to %d\n", ni);
    g_object_set (self->priv->pipeline, "current-video", ni, NULL);

//    gst_element_set_state (self->priv->pipeline, GST_STATE_PLAYING);
}

static gboolean
player_on_ss_change (Player *self, GtkWidget *item)
{
    gint ni = atoi (gtk_menu_item_get_label (GTK_MENU_ITEM (item))) - 1;

//    gst_element_set_state (self->priv->pipeline, GST_STATE_NULL);

    g_print ("set current-text to %d\n", ni);
    g_object_set (self->priv->pipeline, "current-text", ni, NULL);

//    gst_element_set_state (self->priv->pipeline, GST_STATE_PLAYING);
}

static GtkWidget*
browser_get_stream_submenu (Player *self, StreamData *sd, GCallback func)
{
    GtkWidget *menu = gtk_menu_new ();
    gint i;

    if (!sd) {
        return menu;
    }

    for (i = 0;; i++) {
        if (sd[i].index == -1) {
            break;
        }

        gchar *str = g_strdup_printf ("%d: %s", i+1, sd[i].lang);
        GtkWidget *item = gtk_menu_item_new_with_label (str);
        g_free (str);
        gtk_menu_append (GTK_MENU (menu), item);
        g_signal_connect_swapped (item, "activate", func, self);
    }

    return menu;
}

static gboolean
player_button_press (GtkWidget *da, GdkEventButton *event, Player *self)
{
    GdkRectangle rect;
    GdkScreen *screen = gdk_screen_get_default ();
    gint num = gdk_screen_get_n_monitors (screen);

    if (self->priv->monitor >= num) {
        self->priv->monitor = num-1;
    }

    if (event->button == 3) {
        GtkWidget *item;
        GtkWidget *menu = gtk_menu_new ();

        item = gtk_menu_item_new_with_label ("Toggle Fullscreen");
        gtk_menu_append (GTK_MENU (menu), item);
        g_signal_connect (item, "activate", G_CALLBACK (toggle_fullscreen), self);

        gtk_menu_append (GTK_MENU (menu), gtk_separator_menu_item_new ());

        GSList *group = NULL;
        gint i;

        for (i = 0; i < num; i++) {
            gdk_screen_get_monitor_geometry (screen, i, &rect);
            gchar *str = g_strdup_printf ("%d: %dx%d", i+1, rect.width, rect.height);
            item = gtk_radio_menu_item_new_with_label (group, str);
            g_free (str);
            group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));

            if (i == self->priv->monitor) {
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
            }

            gtk_menu_append (GTK_MENU (menu), item);
            g_signal_connect (item, "activate", G_CALLBACK (on_pick_screen), self);
        }

        gtk_menu_append (GTK_MENU (menu), gtk_separator_menu_item_new ());

/*
        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CONVERT, NULL);
        gtk_menu_item_set_label (GTK_MENU_ITEM (item), "Video");
        gtk_menu_append (GTK_MENU (menu), item);

        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item),
            browser_get_stream_submenu (self, self->priv->vs, G_CALLBACK (player_on_vs_change)));

        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CONVERT, NULL);
        gtk_menu_item_set_label (GTK_MENU_ITEM (item), "Audio");
        gtk_menu_append (GTK_MENU (menu), item);

        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item),
            browser_get_stream_submenu (self, self->priv->as, G_CALLBACK (player_on_as_change)));

        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CONVERT, NULL);
        gtk_menu_item_set_label (GTK_MENU_ITEM (item), "Subtitles");
        gtk_menu_append (GTK_MENU (menu), item);

        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item),
            browser_get_stream_submenu (self, self->priv->ss, G_CALLBACK (player_on_ss_change)));
*/
        gtk_widget_show_all (menu);

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                        event->button, event->time);
    } else if (event->type == GDK_2BUTTON_PRESS) {
        toggle_fullscreen (NULL, self);
    }

    return FALSE;
}

static gboolean
handle_expose_cb (GtkWidget *widget, GdkEventExpose *event, Player *self)
{
    gdk_draw_rectangle (widget->window, widget->style->black_gc, TRUE,
        0, 0, widget->allocation.width, widget->allocation.height);

    return FALSE;
}

static void
on_control_prev (GtkWidget *widget, Player *self)
{
    g_signal_emit (self, signal_prev, 0);
}

static void
on_control_play (GtkWidget *widget, Player *self)
{
    g_signal_emit (self, signal_play, 0);
}

static void
on_control_pause (GtkWidget *widget, Player *self)
{
    g_signal_emit (self, signal_pause, 0);
}

static void
on_control_next (GtkWidget *widget, Player *self)
{
    g_signal_emit (self, signal_next, 0);
}

static gboolean
on_pos_change_value (GtkWidget *range,
                     GtkScrollType scroll,
                     gdouble value,
                     Player *self)
{
    gint len = player_get_length (self);

    if (value > len) value = len;
    if (value < 0.0) value = 0.0;

    player_set_position (self,  value);

    return FALSE;
}

gchar*
time_to_string (gdouble time)
{
    gint hr, min, sec;

    hr = time;
    sec = hr % 60;
    hr /= 60;
    min = hr % 60;
    hr /= 60;

    if (hr > 0) {
        return g_strdup_printf ("%d:%02d:%02d", hr, min, sec);
    }

    return g_strdup_printf ("%02d:%02d", min, sec);
}
