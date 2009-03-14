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

static GtkWidget *window;
static GtkWidget *sidepane;

void
on_destroy (GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
}

int
main (int argc, char *argv[])
{
	gtk_init (&argc, &argv);
	
	g_print ("Hello World: gmediamp!\n");
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	sidepane = side_pane_new ();
	
	GtkWidget *view = gtk_label_new ("Frame 1");
	side_pane_add (SIDE_PANE (sidepane), view, "Frame1");

	view = gtk_label_new ("Frame 2");
	side_pane_add (SIDE_PANE (sidepane), view, "Frame2");

	view = gtk_label_new ("Frame 3");
	side_pane_add (SIDE_PANE (sidepane), view, "Frame3");
	
	gtk_container_add (GTK_CONTAINER (window), sidepane);
	
	gtk_widget_show_all (window);

	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (on_destroy), NULL);
	
	gtk_main ();

	return 0;
}

