/*
 *      ipod-store.h
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

#ifndef __IPOD_STORE_H__
#define __IPOD_STORE_H__

#include <glib-object.h>
#include <gpod/itdb.h>

#include "shell.h"
#include "entry.h"

#define IPOD_STORE_TYPE (ipod_store_get_type ())
#define IPOD_STORE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), IPOD_STORE_TYPE, IPodStore))
#define IPOD_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), IPOD_STORE_TYPE, IPodStoreClass))
#define IS_IPOD_STORE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), IPOD_STORE_TYPE))
#define IS_IPOD_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IPOD_STORE_TYPE))
#define IPOD_STORE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), IPOD_STORE_TYPE, IPodStoreClass))

G_BEGIN_DECLS

typedef struct _IPodStore IPodStore;
typedef struct _IPodStoreClass IPodStoreClass;
typedef struct _IPodStorePrivate IPodStorePrivate;

struct _IPodStore {
    GObject parent;

    IPodStorePrivate *priv;
};

struct _IPodStoreClass {
    GObjectClass parent;
};

IPodStore *ipod_store_new (Itdb_iTunesDB *db, gint mtype);
GType ipod_store_get_type (void);

G_END_DECLS

#endif /* __IPOD_STORE_H__ */
