/*
 *      side-pane.c
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

G_DEFINE_TYPE(SidePane, side_pane, GTK_TYPE_HPANED)

#define SIDE_PANE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), SIDE_PANE_TYPE, SidePanePrivate))

struct _SidePanePrivate {
    GtkTreeStore *store;
    
    GtkWidget *view;
    GtkWidget *notebook;
    GtkWidget *vbox;
};

static void selector_changed_cb (GtkTreeSelection*,gpointer);

static void
side_pane_finalize (GObject *object)
{
    SidePane *self = SIDE_PANE (object);

    G_OBJECT_CLASS (side_pane_parent_class)->finalize (object);
}

static void
side_pane_class_init (SidePaneClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (SidePanePrivate));
    object_class->finalize = side_pane_finalize;
}

static void
side_pane_init (SidePane *self)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    self->priv = SIDE_PANE_GET_PRIVATE (self);
    
    self->priv->vbox = gtk_vbox_new (FALSE, 0);
    gtk_paned_add1 (GTK_PANED (self), self->priv->vbox);
    
    self->priv->store = gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    
    self->priv->view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (self->priv->store));
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->view), FALSE);
    gtk_box_pack_start (GTK_BOX (self->priv->vbox), self->priv->view, TRUE, TRUE, 0);
//    gtk_paned_add1 (GTK_PANED (self), self->priv->view);
    
    GtkTreeSelection *tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->view));
    g_signal_connect (tree_selection, "changed", G_CALLBACK (selector_changed_cb), self);

    self->priv->notebook = gtk_notebook_new ();
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK(self->priv->notebook), FALSE);
    gtk_paned_add2 (GTK_PANED (self), self->priv->notebook);
        
    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes ("image", renderer,
                                            "stock-id", 0,
                                            NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("desc", renderer,
                                            "text", 1,
                                            NULL);
    g_object_set (column, "expand", TRUE, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->view), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("expander", renderer,
                                            NULL);
    g_object_set (column, "expand", FALSE, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->view), column);
    gtk_tree_view_set_expander_column (GTK_TREE_VIEW (self->priv->view), column);
    
    gtk_widget_show_all (GTK_WIDGET (self));
    
    gtk_tree_view_expand_all (GTK_TREE_VIEW (self->priv->view));
}

GtkWidget*
side_pane_new ()
{
    return g_object_new (SIDE_PANE_TYPE, NULL);
}

static void
selector_changed_cb (GtkTreeSelection *selection, gpointer user_data)
{
    GtkTreeIter iter;
    int page;

    SidePanePrivate *priv = SIDE_PANE_GET_PRIVATE (SIDE_PANE (user_data));
    
    gtk_tree_selection_get_selected (selection, NULL, &iter);
    gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                        2, &page,
                        -1);

    if (page >= 0)
        gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), page);
}

void
side_pane_add (SidePane *self,
               GtkWidget *widget,
               const char *name)
{
    GtkTreeIter iter;
    GtkWidget *name_label = gtk_label_new (name);
    
    SidePanePrivate *priv = SIDE_PANE_GET_PRIVATE (self);
    
    gtk_widget_show_all (widget);
    
    int page = gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                                         widget, name_label);
    
    gtk_tree_store_append (GTK_TREE_STORE (priv->store), &iter, NULL);
    gtk_tree_store_set (GTK_TREE_STORE (priv->store), &iter,
                        0, GTK_STOCK_ABOUT, 1, name, 2, page, -1);
}

void
side_pane_add_bar_widget (SidePane *self, GtkWidget *widget)
{
    gtk_box_pack_start (GTK_BOX (self->priv->vbox), widget, FALSE, FALSE, 0);
    
    gtk_widget_show (widget);
}

