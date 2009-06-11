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

#include "gmediadb.h"
#include "browser.h"
#include "player.h"

static GtkWidget *window;
static GtkWidget *browser;

static GtkWidget *play_button;
static GtkWidget *stop_button;
static GtkWidget *next_button;
static GtkWidget *prev_button;
static GtkWidget *pos_scale;
static GtkWidget *play_info;
static GtkWidget *volume_button;
static GtkWidget *play_image;
static GtkWidget *pause_image;

static GMediaDB *mediadb;
static Player *player;

void
on_destroy (GtkWidget *widget, gpointer user_data)
{
    gtk_main_quit ();
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
        Entry *e = g_new0 (Entry, 1);
        gchar **entry = g_ptr_array_index (entries, i);
        
        e->id = atoi (entry[0]);
        e->artist = g_strdup (entry[1]);
        e->album = g_strdup (entry[2]);
        e->title = g_strdup (entry[3]);
        e->duration = atoi (entry[4]);
        e->track = atoi (entry[5]);
        e->location = g_strdup (entry[6]);
        
        browser_add_entry (BROWSER (browser), e);
    }
    
    gtk_widget_show_all (browser);
    
    g_ptr_array_free (entries, TRUE);
}

void
on_add_entry (GObject *obj, Entry *entry, gpointer user_data)
{
    player_load (player, entry->location);
    player_play (player);
    
    gchar *desc = g_strdup_printf ("%s by %s from %s",
        entry->title, entry->artist, entry->album);
    
    gtk_label_set_text (GTK_LABEL (play_info), desc);
    
    g_free (desc);
}

void
on_replace_entry (GObject *obj, Entry *entry, gpointer user_data)
{
    player_load (player, entry->location);
    player_play (player);
    
    gchar *desc = g_strdup_printf ("%s by %s from %s",
        entry->title, entry->artist, entry->album);
    
    gtk_label_set_text (GTK_LABEL (play_info), desc);
    
    g_free (desc);
}

void
on_eos (GObject *obj, gpointer user_data)
{
    g_print ("On EOS\n");
}

void
on_tag (GObject *obj, gpointer user_data)
{
    g_print ("On TAG\n");
}

void
on_state_changed (GObject *obj, guint state, gpointer user_data)
{
    switch (state) {
        case PLAYER_STATE_NULL:
            gtk_button_set_image (GTK_BUTTON (play_button), play_image);
            g_print ("State: NULL\n");
            gtk_label_set_text (GTK_LABEL (play_info), "Not Playing");
            break;
        case PLAYER_STATE_PLAYING:
            gtk_button_set_image (GTK_BUTTON (play_button), pause_image);
            g_print ("State: Playing\n");
            break;
        case PLAYER_STATE_PAUSED:
            gtk_button_set_image (GTK_BUTTON (play_button), play_image);
            g_print ("State: Paused\n");
            break;
        case PLAYER_STATE_STOPPED:
            gtk_button_set_image (GTK_BUTTON (play_button), play_image);
            gtk_label_set_text (GTK_LABEL (play_info), "Not Playing");
            g_print ("State: Stopped\n");
            break;
        default:
            gtk_button_set_image (GTK_BUTTON (play_button), play_image);
            g_print ("State: Invalid\n");
    }
}

void
on_ratio_changed (GObject *obj, gdouble ratio, gpointer user_data)
{
    gtk_range_set_value (GTK_RANGE (pos_scale), 100 * ratio);
}

void
on_play_button (GtkWidget *widget, gpointer user_data)
{
    g_print ("Play Button Pressed\n");
    
    switch (player_get_state (player)) {
        case PLAYER_STATE_NULL:
            break;
        case PLAYER_STATE_PLAYING:
            player_pause (player);
            break;
        case PLAYER_STATE_PAUSED:
            player_play (player);
            break;
        case PLAYER_STATE_STOPPED:
        default:
            player_stop (player);
    }
}

void
on_stop_button (GtkWidget *widget, gpointer user_data)
{
    g_print ("Stop Button Pressed\n");
    player_stop (player);
}

void
on_next_button (GtkWidget *widget, gpointer user_data)
{
    g_print ("Next Button Pressed\n");
}

void
on_prev_button (GtkWidget *widget, gpointer user_data)
{
    g_print ("Previous Button Pressed\n");
}

int
main (int argc, char *argv[])
{
    GtkWidget *hbox;
    GtkWidget *vbox;
    
    gtk_init (&argc, &argv);
    
    mediadb = gmediadb_new ();
    player = player_new (argc, argv);
    
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
    
    play_info = gtk_label_new ("Some Text");
    gtk_box_pack_start (GTK_BOX (hbox), play_info, TRUE, TRUE, 0);
    
    volume_button = gtk_volume_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), volume_button, FALSE, FALSE, 0);
    
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
    g_signal_connect (G_OBJECT (browser), "entry-replace",
        G_CALLBACK (on_add_entry), NULL);
    
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
    
    Entry *e = NULL;
    int i;
    for (i = 0; i < entries->len; i++) {
        e = g_new0 (Entry, 1);
        gchar **entry = g_ptr_array_index (entries, i);
        
        e->id = atoi (entry[0]);
        e->artist = g_strdup (entry[1]);
        e->album = g_strdup (entry[2]);
        e->title = g_strdup (entry[3]);
        e->duration = atoi (entry[4]);
        e->track = atoi (entry[5]);
        e->location = g_strdup (entry[6]);
        
        browser_add_entry (BROWSER (browser), e);
    }
    
    g_ptr_array_free (entries, TRUE);
    
    gtk_main ();
    
    g_object_unref (G_OBJECT (mediadb));
    g_object_unref (G_OBJECT (play_image));
    g_object_unref (G_OBJECT (pause_image));
    
    return 0;
}

