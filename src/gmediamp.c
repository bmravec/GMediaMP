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

#include "side-pane.h"
#include "gmediadb.h"
#include "browser.h"

static GtkWidget *window;
static GtkWidget *sidepane;
static GtkWidget *browser;

static GtkWidget *play_button;
static GtkWidget *stop_button;
static GtkWidget *next_button;
static GtkWidget *prev_button;
static GtkWidget *pos_scale;

static GMediaDB *mediadb;

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
    
    Entry e;
    guint i;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);
        
        e.id = atoi (entry[0]);
        e.artist = g_strdup (entry[1]);
        e.album = g_strdup (entry[2]);
        e.title = g_strdup (entry[3]);
        e.duration = atoi (entry[4]);
        e.track = atoi (entry[5]);
        e.location = g_strdup (entry[6]);
        
        browser_add_entry (BROWSER (browser), &e);
    }
    
    g_ptr_array_free (entries, TRUE);
}

int
main (int argc, char *argv[])
{
    GtkWidget *hbox;
    GtkWidget *vbox;
    
    gtk_init (&argc, &argv);
    
    mediadb = gmediadb_new ();
    
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    
    hbox = gtk_hbox_new (FALSE, 0);
    play_button = gtk_button_new ();
    stop_button = gtk_button_new ();
    next_button = gtk_button_new ();
    prev_button = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (play_button),
        gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON));
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
    
    pos_scale = gtk_hscale_new_with_range (0.0, 100.0, 0.5);
    gtk_scale_set_draw_value (GTK_SCALE (pos_scale), FALSE);
    
//    sidepane = side_pane_new ();
    
    browser = browser_new ();
    /*
    side_pane_add (SIDE_PANE (sidepane), browser, "Browser");
    
    GtkWidget *view = gtk_label_new ("Frame 1");
    side_pane_add (SIDE_PANE (sidepane), view, "Frame1");
    
    view = gtk_label_new ("Frame 2");
    side_pane_add (SIDE_PANE (sidepane), view, "Frame2");
    
    view = gtk_label_new ("Frame 3");
    side_pane_add (SIDE_PANE (sidepane), view, "Frame3");
    */
    
    vbox = gtk_vbox_new (FALSE, 0);
    
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), pos_scale, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), browser, TRUE, TRUE, 0);
//    gtk_box_pack_start (GTK_BOX (vbox), sidepane, TRUE, TRUE, 0);
    
    gtk_container_add (GTK_CONTAINER (window), vbox);
    
    gtk_widget_show_all (window);
    
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (on_destroy), NULL);
    g_signal_connect (G_OBJECT (mediadb), "add-entry", G_CALLBACK (on_add), NULL);
    
    gchar *tags[] = { "id", "artist", "album", "title", "duration", "track", "location", NULL };
    GPtrArray *entries = gmediadb_get_all_entries (mediadb, tags);
    
    Entry e;
    int i;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);
        
        e.id = atoi (entry[0]);
        e.artist = entry[1];
        e.album = entry[2];
        e.title = entry[3];
        e.duration = atoi (entry[4]);
        e.track = atoi (entry[5]);
        e.location = entry[6];
        
        browser_add_entry (BROWSER (browser), &e);
    }
    
    g_ptr_array_free (entries, TRUE);
    
    gtk_main ();
    
    g_object_unref (G_OBJECT (mediadb));
    mediadb = NULL;
    
    return 0;
}

