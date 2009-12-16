/*
 *      device-manager.h
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

#ifndef __DEVICE_MANAGER_H__
#define __DEVICE_MANAGER_H__

#include <glib-object.h>
#include "shell.h"

#define DEVICE_MANAGER_TYPE (device_manager_get_type ())
#define DEVICE_MANAGER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), DEVICE_MANAGER_TYPE, DeviceManager))
#define DEVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DEVICE_MANAGER_TYPE, DeviceManagerClass))
#define IS_DEVICE_MANAGER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), DEVICE_MANAGER_TYPE))
#define IS_DEVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DEVICE_MANAGER_TYPE))
#define DEVICE_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DEVICE_MANAGER_TYPE, DeviceManagerClass))

G_BEGIN_DECLS

typedef struct _DeviceManager DeviceManager;
typedef struct _DeviceManagerClass DeviceManagerClass;
typedef struct _DeviceManagerPrivate DeviceManagerPrivate;

struct _DeviceManager {
    GObject parent;

    DeviceManagerPrivate *priv;
};

struct _DeviceManagerClass {
    GObjectClass parent;
};

DeviceManager *device_manager_new (Shell *shell);
GType device_manager_get_type (void);

G_END_DECLS

#endif /* __DEVICE_MANAGER_H__ */
