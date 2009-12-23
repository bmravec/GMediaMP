/*
 *      column-funcs.c
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

#include "column-funcs.h"
#include "entry.h"
#include "player.h"

void
str_column_func (GtkTreeViewColumn *column,
                 GtkCellRenderer *cell,
                 GtkTreeModel *model,
                 GtkTreeIter *iter,
                 gchar *data)
{
    Entry *entry;

    gtk_tree_model_get (model, iter, 0, &entry, -1);
    if (entry) {
        g_object_set (G_OBJECT (cell), "text", entry_get_tag_str (entry, data), NULL);
        g_object_unref (entry);
    } else {
        g_object_set (G_OBJECT (cell), "text", "", NULL);
    }
}

void
time_column_func (GtkTreeViewColumn *column,
                  GtkCellRenderer *cell,
                  GtkTreeModel *model,
                  GtkTreeIter *iter,
                  gchar *data)
{
    Entry *entry;

    gtk_tree_model_get (model, iter, 0, &entry, -1);
    if (entry) {
        gchar *str = time_to_string ((gdouble) entry_get_tag_int (entry, data));
        g_object_set (G_OBJECT (cell), "text", str, NULL);
        g_free (str);
        g_object_unref (entry);
    } else {
        g_object_set (G_OBJECT (cell), "text", "", NULL);
    }
}


void
int_column_func (GtkTreeViewColumn *column,
                 GtkCellRenderer *cell,
                 GtkTreeModel *model,
                 GtkTreeIter *iter,
                 gchar *data)
{
    Entry *entry;
    gint num = 0;

    gtk_tree_model_get (model, iter, 0, &entry, -1);
    if (entry) {
        num = entry_get_tag_int (entry, data);
        g_object_unref (entry);
    }

    if (num != 0) {
        gchar *str = g_strdup_printf ("%d", num);
        g_object_set (G_OBJECT (cell), "text", str, NULL);
        g_free (str);
    } else {
        g_object_set (G_OBJECT (cell), "text", "", NULL);
    }
}
