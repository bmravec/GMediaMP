/*
 *      device.h
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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <gtk/gtk.h>

#include "shell.h"

#define DEVICE_TYPE (device_get_type ())
#define DEVICE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), DEVICE_TYPE, Device))
#define IS_DEVICE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), DEVICE_TYPE))
#define DEVICE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), DEVICE_TYPE, DeviceInterface))

G_BEGIN_DECLS

typedef struct _Device Device;
typedef struct _DeviceInterface DeviceInterface;

struct _DeviceInterface {
    GTypeInterface parent;

    void   (*add_entry) (Device *self, gchar **entry);
    void   (*rem_entry) (Device *self, Entry *entry);
    gchar* (*get_name)  (Device *self);
};

GType device_get_type (void);

void device_add_entry (Device *self, gchar **entry);
void device_remove_entry (Device *self, Entry *entry);

gchar *device_get_name (Device *self);

G_END_DECLS

#endif /* __DEVICE_H__ */
