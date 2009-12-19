/*
 *      gmediadb-store.h
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

#ifndef __GMEDIADB_STORE_H__
#define __GMEDIADB_STORE_H__

#include <glib-object.h>

#include "shell.h"
#include "entry.h"

#define GMEDIADB_STORE_TYPE (gmediadb_store_get_type ())
#define GMEDIADB_STORE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GMEDIADB_STORE_TYPE, GMediaDBStore))
#define GMEDIADB_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMEDIADB_STORE_TYPE, GMediaDBStoreClass))
#define IS_GMEDIADB_STORE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GMEDIADB_STORE_TYPE))
#define IS_GMEDIADB_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMEDIADB_STORE_TYPE))
#define GMEDIADB_STORE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMEDIADB_STORE_TYPE, GMediaDBStoreClass))

G_BEGIN_DECLS

typedef struct _GMediaDBStore GMediaDBStore;
typedef struct _GMediaDBStoreClass GMediaDBStoreClass;
typedef struct _GMediaDBStorePrivate GMediaDBStorePrivate;

struct _GMediaDBStore {
    GObject parent;

    GMediaDBStorePrivate *priv;
};

struct _GMediaDBStoreClass {
    GObjectClass parent;
};

GMediaDBStore *gmediadb_store_new (gchar *media_type, gint mtype);
GType gmediadb_store_get_type (void);

G_END_DECLS

#endif /* __GMEDIADB_STORE_H__ */
