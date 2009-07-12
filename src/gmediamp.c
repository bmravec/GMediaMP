/*
 *      gmediamp.c
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
#include <gmediadb.h>
#include <libnotify/notify.h>

#include "browser.h"
#include "player.h"
#include "tray.h"

static GtkWidget *window;
static GtkWidget *browser;

static GtkWidget *play_button;
static GtkWidget *stop_button;
static GtkWidget *next_button;
static GtkWidget *prev_button;
static GtkWidget *volume_button;
static GtkWidget *play_image;
static GtkWidget *pause_image;

static GtkWidget *pos_scale;
static GtkWidget *play_info;
static GtkWidget *play_time;

static GMediaDB *mediadb;
static Player *player;
static Entry *s_entry;
static Tray *tray_icon;

static gboolean win_visible;

gchar*
find_album_art (Entry *e)
{
    GFile *location = g_file_new_for_path (entry_get_location (e));
    GFile *parent = g_file_get_parent (location);
    gchar *ppath = g_file_get_path (parent);
    GDir *dir = g_dir_open (ppath, 0, NULL);
    const gchar *file;
    gchar *ret = NULL;

    while (dir && (file = g_dir_read_name (dir))) {
        if (g_str_has_suffix (file, ".png"))
            ret = g_strdup_printf ("%s/%s", ppath, file);
        if (g_str_has_suffix (file, ".bmp"))
            ret = g_strdup_printf ("%s/%s", ppath, file);
        if (g_str_has_suffix (file, ".png"))
            ret = g_strdup_printf ("%s/%s", ppath, file);
    }

    if (dir) {
        g_dir_close (dir);
    }

    g_object_unref (G_OBJECT (location));
    g_object_unref (G_OBJECT (parent));
    g_free (ppath);

    return ret;
}

void
play_entry (Entry *e)
{
    if (s_entry) {
        if (entry_get_state (s_entry) == ENTRY_STATE_PLAYING) {
            entry_set_state (s_entry, ENTRY_STATE_NONE);
        }

        g_object_unref (G_OBJECT (s_entry));
        s_entry = NULL;
    }

    if (e && entry_get_state (e) != ENTRY_STATE_MISSING) {
        g_print ("PlayEntry (%s,%s,%s)\n",
            entry_get_title (e), entry_get_artist (e), entry_get_album (e));

        s_entry = e;
        g_object_ref (G_OBJECT (s_entry));

        entry_set_state (s_entry, ENTRY_STATE_PLAYING);
        player_load (player, entry_get_location (s_entry));
        player_play (player);

        gchar *body = g_strdup_printf ("%s\n%s\n%s",
            entry_get_title (e), entry_get_artist (e), entry_get_album (e));

        gchar *art_path = find_album_art (e);
        NotifyNotification *n = notify_notification_new ("Playing Track", body, 
        art_path, NULL);

        if (art_path)
            g_free (art_path);

        notify_notification_show (n, NULL);
        g_free (body);
    }
}

void
on_destroy (GtkWidget *widget, gpointer user_data)
{
    gtk_main_quit ();
}

void
on_volume_changed (GtkWidget *widget, gdouble value, gpointer user_data)
{
    player_set_volume (player, value);
}

void
on_toggle (Tray *tray, gpointer user_data)
{
    if (win_visible) {
        gtk_widget_hide (window);
        win_visible = FALSE;
    } else {
        gtk_widget_show (window);
        win_visible = TRUE;
    }
}

void
on_add (GObject *obj, guint id, gpointer user_data)
{
    g_print ("Added: %d\n", id);
    
    GArray *ids = g_array_new (TRUE, TRUE, sizeof (guint));
    g_array_append_val (ids, id);
    
    gchar *tags[] = { "id", "artist", "album", "title", "duration", "track", "location", NULL };
    GPtrArray *entries = gmediadb_get_entries (mediadb, ids, tags);
    g_array_free (ids, TRUE);
    
    if (!entries) {
        return;
    }
    
    guint i;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);
        
        Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
        entry_set_artist (e, entry[1]);
        entry_set_album (e, entry[2]);
        entry_set_title (e, entry[3]);
        entry_set_duration (e, entry[4] ? atoi (entry[4]) : 0);
        entry_set_track (e, entry[5] ? atoi (entry[5]) : 0);
        entry_set_location (e, entry[6]);
        
        browser_add_entry (BROWSER (browser), e);
        
        g_object_unref (G_OBJECT (e));
    }
    
    gtk_widget_show_all (browser);
    
    g_ptr_array_free (entries, TRUE);
}

void
on_remove (GObject *obj, guint id, gpointer user_data)
{
    g_print ("Removed: %d\n", id);
    
    browser_remove_entry (BROWSER (browser), id);
}

void
on_add_entry (GObject *obj, Entry *entry, gpointer user_data)
{
    g_print ("onAddEntry\n");
    play_entry (entry);
}

void
on_replace_entry (GObject *obj, Entry *entry, gpointer user_data)
{
    g_print ("onReplaceEntry\n");
    play_entry (entry);
}

void
on_eos (GObject *obj, gpointer user_data)
{
    g_print ("On EOS\n");
    
    Entry *e = browser_get_next (BROWSER (browser));
    play_entry (e);
    
    gtk_range_set_value (GTK_RANGE (pos_scale), 0);
}

void
on_tag (GObject *obj, gpointer user_data)
{
    g_print ("On TAG\n");
}

void
on_state_changed (GObject *obj, guint state, gpointer user_data)
{
    gchar *desc;

    tray_set_state (tray_icon, state);

    switch (state) {
        case PLAYER_STATE_NULL:
            g_print ("State: NULL\n");
            gtk_button_set_image (GTK_BUTTON (play_button), play_image);
            gtk_label_set_text (GTK_LABEL (play_info), "Not Playing");
            break;
        case PLAYER_STATE_PLAYING:
            g_print ("State: Playing\n");
            gtk_button_set_image (GTK_BUTTON (play_button), pause_image);
            desc = g_strdup_printf ("%s by %s from %s",
                entry_get_title (s_entry),
                entry_get_artist (s_entry),
                entry_get_album (s_entry));
            
            gtk_label_set_text (GTK_LABEL (play_info), desc);
            g_free (desc);
            break;
        case PLAYER_STATE_PAUSED:
            g_print ("State: Paused\n");
            
            gtk_button_set_image (GTK_BUTTON (play_button), play_image);
            desc = g_strdup_printf ("%s by %s from %s",
                entry_get_title (s_entry),
                entry_get_artist (s_entry),
                entry_get_album (s_entry));
            
            gtk_label_set_text (GTK_LABEL (play_info), desc);
            g_free (desc);
            break;
        case PLAYER_STATE_STOPPED:
            g_print ("State: Stopped\n");
            gtk_button_set_image (GTK_BUTTON (play_button), play_image);
            gtk_label_set_text (GTK_LABEL (play_info), "Not Playing");
            break;
        default:
            g_print ("State: Invalid\n");
            gtk_button_set_image (GTK_BUTTON (play_button), play_image);
    }
}

gchar*
sec_to_time (guint second)
{
    guint hour = second / 3600;
    second %= 3600;
    
    guint minute = second / 60;
    second %= 60;
    
    if (hour != 0) {
        return g_strdup_printf ("%02d:%02d:%02d", hour, minute, second);
    } else {
        return g_strdup_printf ("%02d:%02d", minute, second);
    }
}

void
on_ratio_changed (GObject *obj, gdouble ratio, gpointer user_data)
{
    gchar *pos, *len, *time;
    
    gtk_range_set_value (GTK_RANGE (pos_scale), 100 * ratio);
    
    switch (player_get_state (player)) {
        case PLAYER_STATE_PLAYING:
        case PLAYER_STATE_PAUSED:
            pos = sec_to_time (player_get_position (player));
            len = sec_to_time (player_get_length (player));
            time = g_strdup_printf ("%s of %s", pos, len);
            
            g_free (pos);
            g_free (len);
            
            gtk_label_set_text (GTK_LABEL (play_time), time);
            g_free (time);
            break;
        default:
            gtk_label_set_text (GTK_LABEL (play_time), "");
    }
}

void
on_play_button (GtkWidget *widget, gpointer user_data)
{
    g_print ("Play Button Pressed\n");
    Entry *e;
    
    switch (player_get_state (player)) {
        case PLAYER_STATE_NULL:
            e = browser_get_next (BROWSER (browser));
            play_entry (e);
            break;
        case PLAYER_STATE_PLAYING:
            player_pause (player);
            break;
        case PLAYER_STATE_PAUSED:
            player_play (player);
            break;
        case PLAYER_STATE_STOPPED:
            browser_get_prev (BROWSER (browser));
            e = browser_get_next (BROWSER (browser));
            play_entry (e);
            break;
        default:
            play_entry (NULL);
    }
}

void
on_stop_button (GtkWidget *widget, gpointer user_data)
{
    g_print ("Stop Button Pressed\n");
    play_entry (NULL);
}

void
on_next_button (GtkWidget *widget, gpointer user_data)
{
    g_print ("Next Button Pressed\n");
    
    Entry *e = browser_get_next (BROWSER (browser));
    play_entry (e);
}

void
on_prev_button (GtkWidget *widget, gpointer user_data)
{
    g_print ("Previous Button Pressed\n");
    
    Entry *e = browser_get_prev (BROWSER (browser));
    play_entry (e);
}

int
main (int argc, char *argv[])
{
    GtkWidget *hbox;
    GtkWidget *vbox;
    
    gtk_init (&argc, &argv);
    
    mediadb = gmediadb_new ("Music");
    player = player_new (argc, argv);
    tray_icon = tray_new ();
    win_visible = TRUE;

    notify_init ("gmediamp");

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    
    hbox = gtk_hbox_new (FALSE, 0);
    play_button = gtk_button_new ();
    stop_button = gtk_button_new ();
    next_button = gtk_button_new ();
    prev_button = gtk_button_new ();
    
    play_image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
    pause_image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON);
    g_object_ref (G_OBJECT (play_image));
    g_object_ref (G_OBJECT (pause_image));
    
    gtk_button_set_image (GTK_BUTTON (play_button), play_image);
    gtk_button_set_image (GTK_BUTTON (stop_button),
        gtk_image_new_from_stock (GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON));
    gtk_button_set_image (GTK_BUTTON (next_button),
        gtk_image_new_from_stock (GTK_STOCK_MEDIA_NEXT, GTK_ICON_SIZE_BUTTON));
    gtk_button_set_image (GTK_BUTTON (prev_button),
        gtk_image_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start (GTK_BOX (hbox), prev_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), play_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), stop_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), next_button, FALSE, FALSE, 0);
    
    play_info = gtk_label_new ("Not Playing");
    gtk_box_pack_start (GTK_BOX (hbox), play_info, TRUE, TRUE, 0);
    
    play_time = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (hbox), play_time, FALSE, FALSE, 0);
    
    volume_button = gtk_volume_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), volume_button, FALSE, FALSE, 0);
    
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (volume_button), 1.0);
    player_set_volume (player, 1.0);

    pos_scale = gtk_hscale_new_with_range (0.0, 100.0, 0.5);
    gtk_scale_set_draw_value (GTK_SCALE (pos_scale), FALSE);
    
    browser = browser_new ();
    
    vbox = gtk_vbox_new (FALSE, 0);
    
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), pos_scale, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), browser, TRUE, TRUE, 0);
    
    gtk_container_add (GTK_CONTAINER (window), vbox);
    
    gtk_widget_show_all (window);
    
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (on_destroy), NULL);
    g_signal_connect (G_OBJECT (mediadb), "add-entry", G_CALLBACK (on_add), NULL);
    g_signal_connect (G_OBJECT (mediadb), "remove-entry", G_CALLBACK (on_remove), NULL);
    g_signal_connect (G_OBJECT (browser), "entry-replace", G_CALLBACK (on_add_entry), NULL);
    g_signal_connect (G_OBJECT (volume_button), "value-changed",
        G_CALLBACK (on_volume_changed), NULL);

    g_signal_connect (G_OBJECT (tray_icon), "toggle", G_CALLBACK (on_toggle), NULL);
    g_signal_connect (G_OBJECT (tray_icon), "quit", G_CALLBACK (on_destroy), NULL);
    g_signal_connect (G_OBJECT (tray_icon), "play", G_CALLBACK (on_play_button), NULL);
    g_signal_connect (G_OBJECT (tray_icon), "next", G_CALLBACK (on_next_button), NULL);
    g_signal_connect (G_OBJECT (tray_icon), "previous", G_CALLBACK (on_prev_button), NULL);
    
    g_signal_connect (G_OBJECT (player), "eos", G_CALLBACK (on_eos), NULL);
    g_signal_connect (G_OBJECT (player), "tag", G_CALLBACK (on_tag), NULL);
    g_signal_connect (G_OBJECT (player), "state-changed",
        G_CALLBACK (on_state_changed), NULL);
    g_signal_connect (G_OBJECT (player), "ratio-changed",
        G_CALLBACK (on_ratio_changed), NULL);
    
    g_signal_connect (G_OBJECT (play_button), "clicked",
        G_CALLBACK (on_play_button), NULL);
    g_signal_connect (G_OBJECT (stop_button), "clicked",
        G_CALLBACK (on_stop_button), NULL);
    g_signal_connect (G_OBJECT (next_button), "clicked",
        G_CALLBACK (on_next_button), NULL);
    g_signal_connect (G_OBJECT (prev_button), "clicked",
        G_CALLBACK (on_prev_button), NULL);
    
    gchar *tags[] = { "id", "artist", "album", "title", "duration", "track", "location", NULL };
    GPtrArray *entries = gmediadb_get_all_entries (mediadb, tags);
    
    int i;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);
        Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
        entry_set_artist (e, entry[1]);
        entry_set_album (e, entry[2]);
        entry_set_title (e, entry[3]);
        entry_set_duration (e, entry[4] ? atoi (entry[4]): 0);
        entry_set_track (e, entry[5] ? atoi (entry[5]) : 0);
        entry_set_location (e, entry[6]);
        
        browser_add_entry (BROWSER (browser), e);
        
        g_object_unref (G_OBJECT (e));
    }
    
    g_ptr_array_free (entries, TRUE);
    
    gtk_main ();
    
    g_object_unref (G_OBJECT (mediadb));
    g_object_unref (G_OBJECT (play_image));
    g_object_unref (G_OBJECT (pause_image));
    
    return 0;
}

