/*
 *      column-funcs.h
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

#ifndef __COLUMN_FUNCS_H__
#define __COLUMN_FUNCS_H__

#include <gtk/gtk.h>

void time_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
void str_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
void int_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);

#endif /* __COLUMN_FUNCS_H__ */
