/*
 *      device-ipod.h
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

#ifndef __DEVICE_IPOD_H__
#define __DEVICE_IPOD_H__

#include <glib-object.h>
#include <gio/gio.h>

#include "device.h"

#define DEVICE_IPOD_TYPE (device_ipod_get_type ())
#define DEVICE_IPOD(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), DEVICE_IPOD_TYPE, DeviceIPod))
#define DEVICE_IPOD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DEVICE_IPOD_TYPE, DeviceIPodClass))
#define IS_DEVICE_IPOD(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), DEVICE_IPOD_TYPE))
#define IS_DEVICE_IPOD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DEVICE_IPOD_TYPE))
#define DEVICE_IPOD_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DEVICE_IPOD_TYPE, DeviceIPodClass))

G_BEGIN_DECLS

typedef struct _DeviceIPod DeviceIPod;
typedef struct _DeviceIPodClass DeviceIPodClass;
typedef struct _DeviceIPodPrivate DeviceIPodPrivate;

struct _DeviceIPod {
    GObject parent;

    DeviceIPodPrivate *priv;
};

struct _DeviceIPodClass {
    GObjectClass parent;
};

Device *device_ipod_new (Shell *shell, GMount *mount);
GType device_ipod_get_type (void);

G_END_DECLS

#endif /* __DEVICE_IPOD_H__ */
