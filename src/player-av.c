/*
 *      player-avcodec.c
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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <pulse/simple.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>

#include "player-av.h"
#include "shell.h"

static void player_init (PlayerInterface *iface);
G_DEFINE_TYPE_WITH_CODE (PlayerAV, player_av, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (PLAYER_TYPE, player_init)
)

struct _PlayerAVPrivate {
    AVFormatContext *fctx;

    AVCodecContext *vctx;
    AVCodecContext *actx;

    AVFrame *vframe, *vframe_xv;
    uint8_t *vbuffer_xv;
    struct SwsContext *sws_ctx;
    gboolean frame_ready;

    gint astream, vstream;
    GAsyncQueue *apq, *vpq;
    GMutex *rp_mutex;
    GThread *athread;
    gint vt_id;

    int64_t vpos;

    int64_t start_time;
    int64_t stop_time;

    gdouble volume;
    guint state;

    Entry *entry;

    Shell *shell;

    gint monitor;

    gboolean fullscreen;

    guint win_width, win_height;

    GtkWidget *video_dest;

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

    XvImage *xvimage;
    Window win;
    Window root;
    Display *display;
    XvPortID xv_port_id;
    GC xv_gc;
    XGCValues values;
};

static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

static gboolean player_av_get_video_frame (PlayerAV *self, AVFrame *pFrame, int64_t *pts);
static gint player_av_get_audio_frame (PlayerAV *self, short *dest, int64_t *pts);

static gpointer player_av_audio_loop (PlayerAV *self);
static gboolean on_timeout (PlayerAV *self);

static void player_av_change_gdk_window (PlayerAV *self, GdkWindow *window);

static int player_av_av_get_buffer (struct AVCodecContext *c, AVFrame *pic);
static void player_av_av_release_buffer (struct AVCodecContext *c, AVFrame *pic);

static gboolean position_update (PlayerAV *self);
static gboolean player_av_button_press (GtkWidget *da, GdkEventButton *event, PlayerAV *self);
static void player_av_set_state (PlayerAV *self, guint state);
static gboolean handle_expose_cb (GtkWidget *widget, GdkEventExpose *event, PlayerAV *self);
static gboolean on_alloc_event (GtkWidget *widget, GtkAllocation *allocation, PlayerAV *self);
static void on_vol_changed (GtkWidget *widget, gdouble val, PlayerAV *self);

static void on_control_prev (GtkWidget *widget, PlayerAV *self);
static void on_control_play (GtkWidget *widget, PlayerAV *self);
static void on_control_pause (GtkWidget *widget, PlayerAV *self);
static void on_control_next (GtkWidget *widget, PlayerAV *self);

static gboolean on_pos_change_value (GtkWidget *range, GtkScrollType scroll, gdouble value, PlayerAV *self);

static void
player_av_finalize (GObject *object)
{
    PlayerAV *self = PLAYER_AV (object);

    G_OBJECT_CLASS (player_av_parent_class)->finalize (object);
}

static void
player_init (PlayerInterface *iface)
{
    iface->load = player_av_load;
    iface->close = player_av_close;
    iface->get_entry = player_av_get_entry;

    iface->play = player_av_play;
    iface->pause = player_av_pause;
    iface->stop = player_av_stop;
    iface->get_state = player_av_get_state;

    iface->get_duration = player_av_get_duration;
    iface->get_position = player_av_get_position;
    iface->set_position = player_av_set_position;

    iface->set_volume = player_av_set_volume;
    iface->get_volume = player_av_get_volume;

    iface->get_video_destination = player_av_get_video_destination;
    iface->set_video_destination = player_av_set_video_destination;
}

static void
player_av_class_init (PlayerAVClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (PlayerAVPrivate));

    object_class->finalize = player_av_finalize;

    av_register_all();
}

static void
player_av_init (PlayerAV *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), PLAYER_AV_TYPE, PlayerAVPrivate);

    self->priv->state = PLAYER_STATE_NULL;
    self->priv->volume = 1.0;
    self->priv->fullscreen = FALSE;
    self->priv->monitor = 0;
    self->priv->video_dest = NULL;

    self->priv->apq = g_async_queue_new ();
    self->priv->vpq = g_async_queue_new ();

    self->priv->rp_mutex = g_mutex_new ();
}

PlayerAV*
player_av_new (Shell *shell)
{
    PlayerAV *self = g_object_new (PLAYER_AV_TYPE, NULL);

    self->priv->shell = g_object_ref (shell);

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
        G_CALLBACK (player_av_button_press), self);
    g_signal_connect (self->priv->fs_da, "button-press-event",
        G_CALLBACK (player_av_button_press), self);
    g_signal_connect (self->priv->em_da, "size-allocate", G_CALLBACK (on_alloc_event), self);
    g_signal_connect (self->priv->fs_da, "size-allocate", G_CALLBACK (on_alloc_event), self);

    g_signal_connect (self->priv->em_da, "expose-event", G_CALLBACK (handle_expose_cb), self);
    g_signal_connect (self->priv->fs_da, "expose-event", G_CALLBACK (handle_expose_cb), self);

    g_signal_connect (self->priv->fs_prev, "clicked", G_CALLBACK (on_control_prev), self);
    g_signal_connect (self->priv->fs_play, "clicked", G_CALLBACK (on_control_play), self);
    g_signal_connect (self->priv->fs_pause, "clicked", G_CALLBACK (on_control_pause), self);
    g_signal_connect (self->priv->fs_next, "clicked", G_CALLBACK (on_control_next), self);

    g_signal_connect (self->priv->fs_vol, "value-changed", G_CALLBACK (on_vol_changed), self);
    g_signal_connect (self->priv->fs_scale, "change-value", G_CALLBACK (on_pos_change_value), self);

    shell_add_widget (self->priv->shell, self->priv->em_da, "Now Playing", NULL);

    gtk_widget_show_all (self->priv->em_da);

    return self;
}

static gboolean
position_update (PlayerAV *self)
{
    guint len = player_av_get_duration (self);
    guint pos = player_av_get_position (self);

    switch (self->priv->state) {
        case PLAYER_STATE_PLAYING:
        case PLAYER_STATE_PAUSED:
            _player_emit_position_changed (PLAYER (self), pos);
            gtk_range_set_range (GTK_RANGE (self->priv->fs_scale), 0, len);
            gtk_range_set_value (GTK_RANGE (self->priv->fs_scale), pos);
            break;
        default:
            _player_emit_position_changed (PLAYER (self), pos);
            gtk_range_set_range (GTK_RANGE (self->priv->fs_scale), 0, 1);
            gtk_range_set_value (GTK_RANGE (self->priv->fs_scale), 0);
    }

    if (self->priv->state != PLAYER_STATE_PLAYING) {
        return FALSE;
    }

    return TRUE;
}

static void
player_av_set_state (PlayerAV *self, guint state)
{
    self->priv->state = state;
    _player_emit_state_changed (PLAYER (self), state);
}

Entry*
player_av_get_entry (PlayerAV *self)
{
    return self->priv->entry;
}

enum PixelFormat
avcodec_codec_tag_to_pix_fmt (unsigned int codec_tag)
{
    enum PixelFormat fmt;

    gint i;
    for (i = 0; i < PIX_FMT_NB; i++) {
        if (codec_tag == avcodec_pix_fmt_to_codec_tag (i)) {
            return i;
        }
    }

    return PIX_FMT_NONE;
}

void
player_av_load (PlayerAV *self, Entry *entry)
{
    gint i;

    player_av_close (self);

    if (av_open_input_file (&self->priv->fctx, entry_get_location (entry), NULL, 0, NULL) != 0)
        return;

    if (av_find_stream_info (self->priv->fctx) < 0)
        return;

    self->priv->fctx->flags = AVFMT_FLAG_GENPTS;

    dump_format(self->priv->fctx, 0, entry_get_location (entry), 0);

    self->priv->astream = self->priv->vstream = -1;
    for (i = 0; i < self->priv->fctx->nb_streams; i++) {
        if (self->priv->fctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
            self->priv->vstream = i;
        }

        if (self->priv->fctx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
            self->priv->astream = i;
        }

        if (self->priv->vstream != -1 && self->priv->astream != -1)
            break;
    }

    // Setup Audio Stream
    if (self->priv->astream != -1) {
        self->priv->actx = self->priv->fctx->streams[self->priv->astream]->codec;
        AVCodec *acodec = avcodec_find_decoder (self->priv->actx->codec_id);
        if (acodec && avcodec_open (self->priv->actx, acodec) < 0) {
            g_print ("Error opening audio stream\n");
            return;
        }
    } else {
        self->priv->actx = NULL;
    }

    // Setup Video Stream
    if (self->priv->vstream != -1) {
        self->priv->vctx = self->priv->fctx->streams[self->priv->vstream]->codec;
        AVCodec *vcodec = avcodec_find_decoder (self->priv->vctx->codec_id);
        if(vcodec && avcodec_open (self->priv->vctx, vcodec) < 0) {
            g_print ("Error opening video stream\n");
            return;
        }
    } else {
        self->priv->vctx = NULL;
    }

    if (self->priv->vctx) {
        self->priv->vctx->get_buffer = player_av_av_get_buffer;
        self->priv->vctx->release_buffer = player_av_av_release_buffer;

        self->priv->display = gdk_x11_display_get_xdisplay (gdk_display_get_default ());
        self->priv->root = DefaultRootWindow (self->priv->display);

        self->priv->win = GDK_WINDOW_XID (self->priv->em_da->window);
        XSetWindowBackgroundPixmap (self->priv->display, self->priv->win, None);

        int nb_adaptors;
        XvAdaptorInfo *adaptors;
        XvQueryAdaptors (self->priv->display, self->priv->root, &nb_adaptors, &adaptors);
        int adaptor_no = 0, j, res;

        self->priv->xv_port_id = 0;
        for (i = 0; i < nb_adaptors && !self->priv->xv_port_id; i++) {
            adaptor_no = i;
            for (j = 0; j < adaptors[adaptor_no].num_ports && !self->priv->xv_port_id; j++) {
                res = XvGrabPort (self->priv->display, adaptors[adaptor_no].base_id + j, 0);
                if (Success == res) {
                    self->priv->xv_port_id = adaptors[adaptor_no].base_id + j;
                }
            }
        }

        XvFreeAdaptorInfo (adaptors);

        int nb_formats;
        XvImageFormatValues *formats = XvListImageFormats (self->priv->display,
            self->priv->xv_port_id, &nb_formats);

        unsigned int vfmt = avcodec_pix_fmt_to_codec_tag (self->priv->vctx->pix_fmt);
        for (i = 0; i < nb_formats; i++) {
            if (vfmt == formats[i].id) {
                break;
            }
        }

        enum PixelFormat ofmt = PIX_FMT_NONE;

        self->priv->vframe = avcodec_alloc_frame ();
        self->priv->vframe_xv = avcodec_alloc_frame();

        if (i < nb_formats) {
            ofmt = self->priv->vctx->pix_fmt;
        } else {
            for (i = 0; i < nb_formats; i++) {
                ofmt = avcodec_codec_tag_to_pix_fmt (formats[i].id);
                if (ofmt != PIX_FMT_NONE) {
                    break;
                }
            }
        }

        int num_bytes = avpicture_get_size (ofmt,
            self->priv->vctx->width + self->priv->vctx->width % 4, self->priv->vctx->height);
        self->priv->vbuffer_xv = (uint8_t*) av_malloc (num_bytes * sizeof (uint8_t));

        avpicture_fill ((AVPicture*) self->priv->vframe_xv,
            self->priv->vbuffer_xv, ofmt,
            self->priv->vctx->width + self->priv->vctx->width % 4, self->priv->vctx->height);

        self->priv->sws_ctx = sws_getContext (
            self->priv->vctx->width, self->priv->vctx->height, self->priv->vctx->pix_fmt,
            self->priv->vctx->width, self->priv->vctx->height, ofmt,
            SWS_POINT, NULL, NULL, NULL);

        self->priv->xvimage = XvCreateImage (
            self->priv->display, self->priv->xv_port_id,
            formats[i].id, self->priv->vbuffer_xv,
            self->priv->vctx->width, self->priv->vctx->height);

        XFree (formats);

        self->priv->xv_gc = XCreateGC (self->priv->display, self->priv->win, 0, &self->priv->values);
    }

    self->priv->entry = entry;
    g_object_ref (entry);

    self->priv->vpos = 0;

    self->priv->start_time = -1;
    self->priv->stop_time = -1;
}

void
player_av_close (PlayerAV *self)
{
    player_av_stop (self);

    if (self->priv->video_dest) {
        gdk_window_invalidate_rect (self->priv->video_dest->window, NULL, TRUE);
    }

    if (self->priv->vctx) {
        avcodec_close (self->priv->vctx);
        self->priv->vctx = NULL;
    }

    if (self->priv->actx) {
        avcodec_close (self->priv->actx);
        self->priv->actx = NULL;
    }

    if (self->priv->fctx) {
        av_close_input_file (self->priv->fctx);
        self->priv->fctx = NULL;
    }

    if (self->priv->vbuffer_xv) {
        av_free (self->priv->vbuffer_xv);
        self->priv->vbuffer_xv = NULL;
    }

    if (self->priv->vframe) {
        av_free (self->priv->vframe);
        self->priv->vframe = NULL;
    }

    if (self->priv->vframe_xv) {
        av_free (self->priv->vframe_xv);
        self->priv->vframe_xv = NULL;
    }

    if (self->priv->xv_gc) {
        XFreeGC (self->priv->display, self->priv->xv_gc);
        self->priv->xv_gc = NULL;
    }

    if (self->priv->sws_ctx) {
        sws_freeContext (self->priv->sws_ctx);
        self->priv->sws_ctx = NULL;
    }

    if (self->priv->xvimage) {
        XFree (self->priv->xvimage);
        self->priv->xvimage = NULL;
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
player_av_play (PlayerAV *self)
{
    if (self->priv->stop_time != -1) {
        self->priv->start_time += av_gettime () - self->priv->stop_time;
        self->priv->stop_time = -1;
    } else {
        self->priv->start_time = av_gettime ();
    }

    self->priv->athread = g_thread_create ((GThreadFunc) player_av_audio_loop, self, TRUE, NULL);

    if (self->priv->vctx) {
        self->priv->frame_ready = FALSE;
        self->priv->vt_id = g_timeout_add_full (
            G_PRIORITY_HIGH,
            10,
            (GSourceFunc) on_timeout,
            self,
            NULL);
    }

    player_av_set_state (self, PLAYER_STATE_PLAYING);

    entry_set_state (self->priv->entry, ENTRY_STATE_PLAYING);

    g_timeout_add (500, (GSourceFunc) position_update, self);
}

void
player_av_pause (PlayerAV *self)
{
    if (!self->priv->entry) {
        return;
    }

    player_av_set_state (self, PLAYER_STATE_PAUSED);
    entry_set_state (self->priv->entry, ENTRY_STATE_PAUSED);

    self->priv->stop_time = av_gettime ();
}

void
player_av_stop (PlayerAV *self)
{
    if (!self->priv->entry) {
        return;
    }

    player_av_set_state (self, PLAYER_STATE_STOPPED);
    if (entry_get_state (self->priv->entry) != ENTRY_STATE_MISSING) {
        entry_set_state (self->priv->entry, ENTRY_STATE_NONE);
    }

    if (self->priv->athread) {
        g_thread_join (self->priv->athread);
        self->priv->athread = NULL;
    }

    self->priv->vt_id = -1;

    self->priv->start_time = self->priv->stop_time = -1;

    while (g_async_queue_length (self->priv->apq) > 0) {
        AVPacket *packet = g_async_queue_pop (self->priv->apq);
        if (packet->data) {
            av_free_packet (packet);
        }

        av_free (packet);
    }

    while (g_async_queue_length (self->priv->vpq) > 0) {
        AVPacket *packet = g_async_queue_pop (self->priv->vpq);
        if (packet->data) {
            av_free_packet (packet);
        }

        av_free (packet);
    }

//    g_signal_emit (self, signal_pos, 0, 0);
    _player_emit_position_changed (PLAYER (self), 0);
}

guint
player_av_get_state (PlayerAV *self)
{
    return self->priv->state;
}

guint
player_av_get_duration (PlayerAV *self)
{
    if (self->priv->fctx) {
        return self->priv->fctx->duration / AV_TIME_BASE;
    } else {
        return 0;
    }
}

guint
player_av_get_position (PlayerAV *self)
{
    if (self->priv->start_time != -1) {
        if (self->priv->stop_time != -1) {
            return (self->priv->stop_time - self->priv->start_time) / 1000000;
        } else {
            return (av_gettime () - self->priv->start_time) / 1000000;
        }
    } else {
        return 0;
    }
}

void
player_av_set_position (PlayerAV *self, guint pos)
{
    int stream = self->priv->astream;
    int64_t seek_target = av_rescale_q (AV_TIME_BASE * pos, AV_TIME_BASE_Q,
        self->priv->fctx->streams[stream]->time_base);

    if (!av_seek_frame (self->priv->fctx, stream, seek_target, 0)) {
        g_print ("It Failed\n");
    } else {
        g_print ("Seek OK?\n");
    }

    g_mutex_lock (self->priv->rp_mutex);
    g_async_queue_lock (self->priv->apq);
    g_async_queue_lock (self->priv->vpq);

    while (g_async_queue_length_unlocked (self->priv->apq) > 0) {
        AVPacket *packet = g_async_queue_pop_unlocked (self->priv->apq);
        av_free_packet (packet);
        av_free (packet);
    }

    while (g_async_queue_length_unlocked (self->priv->vpq) > 0) {
        AVPacket *packet = g_async_queue_pop_unlocked (self->priv->vpq);
        av_free_packet (packet);
        av_free (packet);
    }

    avcodec_flush_buffers (self->priv->actx);
    if (self->priv->vctx) {
        avcodec_flush_buffers (self->priv->vctx);
    }

    g_async_queue_unlock (self->priv->apq);
    g_async_queue_unlock (self->priv->vpq);
    g_mutex_unlock (self->priv->rp_mutex);

    switch (self->priv->state) {
        case PLAYER_STATE_PLAYING:
            break;
        case PLAYER_STATE_PAUSED:
        case PLAYER_STATE_STOPPED:
            break;
        default:
            g_print ("Can not change position in current state\n");
    };

//    g_signal_emit (self, signal_pos, 0, player_av_get_position (self));
    _player_emit_position_changed (PLAYER (self),
        player_av_get_position (self));
}

gdouble
player_av_get_volume (PlayerAV *self)
{
    return self->priv->volume;
}

void
player_av_set_volume (PlayerAV *self, gdouble vol)
{
    if (self->priv->volume != vol) {
        if (vol > 1.0) vol = 1.0;
        if (vol < 0.0) vol = 0.0;

        self->priv->volume = vol;

        gtk_scale_button_set_value (GTK_SCALE_BUTTON (self->priv->fs_vol), self->priv->volume);

//        g_signal_emit (self, signal_volume, 0, vol);
        _player_emit_volume_changed (PLAYER (self), self->priv->volume);
    }
}

static void
on_vol_changed (GtkWidget *widget, gdouble val, PlayerAV *self)
{
    player_av_set_volume (self, val);
}

static gboolean
on_alloc_event (GtkWidget *widget, GtkAllocation *allocation, PlayerAV *self)
{
    if (widget == self->priv->video_dest) {
        self->priv->win_width = allocation->width;
        self->priv->win_height = allocation->height;
    }
}

static void
on_pick_screen (GtkWidget *item, PlayerAV *self)
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
toggle_fullscreen (GtkWidget *item, PlayerAV *self)
{
    GdkRectangle rect;
    GdkScreen *screen = gdk_screen_get_default ();
    gint num = gdk_screen_get_n_monitors (screen);

    if (self->priv->monitor >= num) {
        self->priv->monitor = num-1;
    }

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

    player_av_set_video_destination (self, NULL);
}

static gboolean
player_av_button_press (GtkWidget *da, GdkEventButton *event, PlayerAV *self)
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

        gtk_widget_show_all (menu);

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                        event->button, event->time);
    } else if (event->type == GDK_2BUTTON_PRESS) {
        toggle_fullscreen (NULL, self);
    }

    return FALSE;
}

static gboolean
handle_expose_cb (GtkWidget *widget, GdkEventExpose *event, PlayerAV *self)
{
    gdk_draw_rectangle (widget->window, widget->style->black_gc, TRUE,
        0, 0, widget->allocation.width, widget->allocation.height);

    return FALSE;
}

static void
on_control_prev (GtkWidget *widget, PlayerAV *self)
{
//    g_signal_emit (self, signal_prev, 0);
    _player_emit_previous (PLAYER (self));
}

static void
on_control_play (GtkWidget *widget, PlayerAV *self)
{
//    g_signal_emit (self, signal_play, 0);
    _player_emit_play (PLAYER (self));
}

static void
on_control_pause (GtkWidget *widget, PlayerAV *self)
{
//    g_signal_emit (self, signal_pause, 0);
    _player_emit_pause (PLAYER (self));
}

static void
on_control_next (GtkWidget *widget, PlayerAV *self)
{
//    g_signal_emit (self, signal_next, 0);
    _player_emit_next (PLAYER (self));
}

static gboolean
on_pos_change_value (GtkWidget *range,
                     GtkScrollType scroll,
                     gdouble value,
                     PlayerAV *self)
{
    gint len = player_av_get_duration (self);

    if (value > len) value = len;
    if (value < 0.0) value = 0.0;

    player_av_set_position (self,  value);

    return FALSE;
}

static gboolean
player_av_get_video_frame (PlayerAV *self, AVFrame *pFrame, int64_t *pts)
{
    static AVPacket *packet = NULL;
    static int       bytesRemaining = 0;
    static uint8_t  *rawData;
    int              bytesDecoded;
    int              frameFinished;

    // Decode packets until we have decoded a complete frame
    while (TRUE) {
        // Work on the current packet until we have decoded all of it
        while (bytesRemaining > 0) {
            // Decode the next chunk of data
            bytesDecoded = avcodec_decode_video (self->priv->vctx, pFrame,
                &frameFinished, rawData, bytesRemaining);

            if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque && *(uint64_t*) pFrame->opaque != AV_NOPTS_VALUE) {
                *pts = *(uint64_t*) pFrame->opaque;
            } else if (packet->dts != AV_NOPTS_VALUE) {
                *pts = packet->dts;
            } else {
                *pts = 0;
            }

            // Was there an error?
            if (bytesDecoded <= 0) {
                fprintf (stderr, "Error while decoding frame %d\n", frameFinished);
                bytesRemaining = 0;
                rawData = NULL;
                return FALSE;
            }

            bytesRemaining -= bytesDecoded;
            rawData += bytesDecoded;

            // Did we finish the current frame? Then we can return
            if (frameFinished)
                return TRUE;
        }

        // Read the next packet, skipping all packets that aren't for this
        // stream
        while (TRUE) {
            // Free old packet
            if (packet) {
                if (packet->data != NULL)
                    av_free_packet (packet);
                av_free (packet);
                packet = NULL;
            }

            if (g_async_queue_length (self->priv->vpq) > 0) {
                packet = (AVPacket*) g_async_queue_pop (self->priv->vpq);
                break;
            } else {
                packet = (AVPacket*) av_mallocz (sizeof (AVPacket));
                // Read new packet

                g_mutex_lock (self->priv->rp_mutex);

                if(av_read_frame (self->priv->fctx, packet) < 0) {
                    g_mutex_unlock (self->priv->rp_mutex);
                    goto loop_exit;
                }

                if (packet->stream_index == self->priv->vstream) {
                    g_mutex_unlock (self->priv->rp_mutex);
                    break;
                } else if (packet->stream_index == self->priv->astream) {
                    g_async_queue_push (self->priv->apq, packet);
                    packet = NULL;
                }

                g_mutex_unlock (self->priv->rp_mutex);
            }
        }

        bytesRemaining = packet->size;
        rawData = packet->data;
        global_video_pkt_pts = packet->pts;
    }

loop_exit:
    // Decode the rest of the last frame
    bytesDecoded = avcodec_decode_video (self->priv->vctx, pFrame,
        &frameFinished, rawData, bytesRemaining);

    // Free last packet
    if (packet != NULL)
        if (packet->data != NULL) {
            av_free_packet (packet);
        av_free (packet);
        packet = NULL;
    }

    return frameFinished != 0;
}

static gint
player_av_get_audio_frame (PlayerAV *self, short *dest, int64_t *pts)
{
    static AVPacket *packet = NULL;
    static uint8_t *pkt_data = NULL;
    static int pkt_size = 0;

    int len1, data_size;

    for (;;) {
        while (pkt_size > 0) {
            data_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * self->priv->actx->channels;

            len1 = avcodec_decode_audio2 (self->priv->actx, dest,
                &data_size, pkt_data, pkt_size);

            if (len1 < 0) {
                pkt_size = 0;
                break;
            }

            *pts = packet->pts;

            pkt_data += len1;
            pkt_size -= len1;

            if (data_size <= 0)
                continue;

            return data_size;
        }

        for (;;) {
            // Free old packet
            if (packet) {
                if (packet->data != NULL)
                    av_free_packet (packet);
                av_free (packet);
                packet = NULL;
            }

            if (g_async_queue_length (self->priv->apq) > 0) {
                packet = (AVPacket*) g_async_queue_pop (self->priv->apq);
                break;
            } else {
                packet = (AVPacket*) av_mallocz (sizeof (AVPacket));
                // Read new packet

                g_mutex_lock (self->priv->rp_mutex);

                if(av_read_frame (self->priv->fctx, packet) < 0) {
                    g_mutex_unlock (self->priv->rp_mutex);
                    return -1;
                }

                if (packet->stream_index == self->priv->astream) {
                    g_mutex_unlock (self->priv->rp_mutex);
                    break;
                } else if (packet->stream_index == self->priv->vstream) {
                    g_async_queue_push (self->priv->vpq, packet);
                    packet = NULL;
                }

                g_mutex_unlock (self->priv->rp_mutex);
            }
        }

        pkt_data = packet->data;
        pkt_size = packet->size;
    }
}

gpointer
player_av_audio_loop (PlayerAV *self)
{
    pa_simple *s;
    pa_sample_spec ss;

    ss.format = PA_SAMPLE_S16LE;
    ss.rate = self->priv->actx->sample_rate;
    ss.channels = self->priv->actx->channels;

    s = pa_simple_new (NULL, "GMediaMP", PA_STREAM_PLAYBACK, NULL, "Music", &ss,
        NULL, NULL, NULL);

    int64_t pts;

    gint len, lcv;
    short *abuffer = (short*) av_malloc (AVCODEC_MAX_AUDIO_FRAME_SIZE * self->priv->actx->channels * sizeof (uint8_t));

    double atime, ctime;

    while ((len = player_av_get_audio_frame (self, abuffer, &pts)) > 0) {
        if (pts != AV_NOPTS_VALUE) {
            atime = pts * av_q2d (self->priv->fctx->streams[self->priv->astream]->time_base);
            ctime = (av_gettime () - self->priv->start_time) / 1000000.0;

            self->priv->start_time += (1000000 * (ctime - atime));
        }

        if (len > 0) {
            gint i;
            for (i = 0; i < len / sizeof (short); i++) {
                gdouble val = self->priv->volume * abuffer[i];
                if (val > 32767) {
                    abuffer[i] = 32767;
                } else if (val < -32768) {
                    abuffer[i] = -32768;
                } else {
                    abuffer[i] = val;
                }
            }

            pa_simple_write (s, abuffer, len, NULL);
        }

        if (atime >= self->priv->fctx->duration) {
            break;
        }

        if (self->priv->state != PLAYER_STATE_PLAYING) {
            break;
        }
    }

    if (self->priv->state == PLAYER_STATE_PLAYING) {
        self->priv->state = PLAYER_STATE_STOPPED;
        _player_emit_eos (PLAYER (self));
    }

    av_free (abuffer);

    pa_simple_flush (s, NULL);
    pa_simple_free (s);
}

gboolean
on_timeout (PlayerAV *self)
{
    if (self->priv->vt_id < 0) {
        self->priv->frame_ready = FALSE;
        return FALSE;
    }

    gint res;
    int64_t pts;
    int64_t ctime;
    gint sw, sh, sx, sy;
    double ratio = 1.0 * self->priv->vctx->width / self->priv->vctx->height;

    if (self->priv->frame_ready) {
        sws_scale (self->priv->sws_ctx,
            self->priv->vframe->data, self->priv->vframe->linesize,
            0, self->priv->vctx->height,
            self->priv->vframe_xv->data, self->priv->vframe_xv->linesize);

        if (self->priv->win_height * ratio > self->priv->win_width) {
            sw = self->priv->win_width;
            sh = self->priv->win_width / ratio;
            sx = 0;
            sy = (self->priv->win_height - sh) / 2;
        } else {
            sw = self->priv->win_height * ratio;
            sh = self->priv->win_height;
            sx = (self->priv->win_width - sw) / 2;
            sy = 0;
        }

        XvPutImage (self->priv->display, self->priv->xv_port_id,
            self->priv->win, self->priv->xv_gc, self->priv->xvimage,
            0, 0, self->priv->vctx->width, self->priv->vctx->height,
            sx, sy, sw, sh);

        self->priv->frame_ready = FALSE;
    }

    if (self->priv->state != PLAYER_STATE_PLAYING) {
        self->priv->frame_ready = FALSE;
        self->priv->vt_id = -1;
        return FALSE;
    }

    res = player_av_get_video_frame (self, self->priv->vframe, &pts);

    self->priv->frame_ready = TRUE;

//    double delta = av_q2d (self->priv->fctx->streams[self->priv->vstream]->time_base);
    double delta = av_q2d (self->priv->vctx->time_base);

//    if (delta < 0.005) {
//        delta = av_q2d (self->priv->vctx->time_base);
//    }

    if (self->priv->vctx->time_base.num > 1) {
        delta *= 2;
    }

    double mult = 1;
    if (abs (pts - self->priv->vpos) < 5) {
        mult = delta * 1000;
    }

    self->priv->vpos = pts;

    ctime = (av_gettime () - self->priv->start_time) / 1000;

    double delay = pts * mult - ctime;

    if (delay > 5000 * delta) {
        self->priv->vt_id = g_timeout_add_full (
            G_PRIORITY_HIGH,
            delta * 5000,
            (GSourceFunc) on_timeout,
            self,
            NULL);
    } else if (delay > 0) {
        self->priv->vt_id = g_timeout_add_full (
            G_PRIORITY_HIGH,
            delay,
            (GSourceFunc) on_timeout,
            self,
            NULL);
    } else {
        self->priv->vt_id = g_timeout_add_full (
            G_PRIORITY_HIGH,
            1,
            (GSourceFunc) on_timeout,
            self,
            NULL);
    }

//    g_print ("VP(%d) PTS(%d) DIFF(%f) TIME(%d) TB(%d,%d) M(%f) D(%f) STB(%d,%d)\n", self->priv->vpos, pts, delta, ctime,
//        self->priv->vctx->time_base.num, self->priv->vctx->time_base.den, mult, delay,
//        self->priv->fctx->streams[self->priv->vstream]->time_base.num,
//        self->priv->fctx->streams[self->priv->vstream]->time_base.den);

    return FALSE;
}

static void
player_av_change_gdk_window (PlayerAV *self, GdkWindow *window)
{
    if (self->priv->fctx) {
        self->priv->win = GDK_WINDOW_XID (window);
    }

    if (self->priv->vctx) {
        self->priv->xv_gc = XCreateGC (self->priv->display, self->priv->win, 0, &self->priv->values);
    }
}

static int
player_av_av_get_buffer (struct AVCodecContext *c, AVFrame *pic)
{
    int ret = avcodec_default_get_buffer (c, pic);
    uint64_t *pts = av_malloc (sizeof (uint64_t));
    *pts = global_video_pkt_pts;
    pic->opaque = pts;
    return ret;
}

static void
player_av_av_release_buffer (struct AVCodecContext *c, AVFrame *pic)
{
    if (pic)
        av_freep (&pic->opaque);
    avcodec_default_release_buffer (c, pic);
}

void
player_av_set_video_destination (PlayerAV *self, GtkWidget *dest)
{
    if (self->priv->video_dest) {
        gdk_window_invalidate_rect (self->priv->video_dest->window, NULL, TRUE);
    }

    self->priv->video_dest = dest;

    if (dest) {
        player_av_change_gdk_window (self, dest->window);
    } else {
        if (self->priv->fullscreen) {
            player_av_change_gdk_window (self, self->priv->fs_da->window);
            self->priv->video_dest = self->priv->fs_da;
        } else {
            player_av_change_gdk_window (self, self->priv->em_da->window);
            self->priv->video_dest = self->priv->em_da;
        }
    }

    g_signal_connect (self->priv->video_dest, "size-allocate", G_CALLBACK (on_alloc_event), self);

    gdk_drawable_get_size (GDK_DRAWABLE (self->priv->video_dest->window),
        &self->priv->win_width, &self->priv->win_height);
}

GtkWidget*
player_av_get_video_destination (PlayerAV *self)
{
    return self->priv->video_dest;
}
