/*
 *      device.c
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

#include "../config.h"

#include "device.h"
#include "shell.h"

static void
device_base_init (gpointer g_iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;

    }
}

GType
device_get_type ()
{
    static GType object_type = 0;
    if (!object_type) {
        static const GTypeInfo object_info = {
            sizeof(DeviceInterface),
            device_base_init,       /* base init */
            NULL,                   /* base finalize */
        };

        object_type = g_type_register_static(G_TYPE_INTERFACE,
            "Device", &object_info, 0);
    }

    return object_type;
}

void
device_add_entry (Device *self, gchar **entry)
{
    DeviceInterface *iface = DEVICE_GET_IFACE (self);

    if (iface->add_entry) {
        iface->add_entry (self, entry);
    }
}

void
device_remove_entry (Device *self, Entry *entry)
{
    DeviceInterface *iface = DEVICE_GET_IFACE (self);

    if (iface->rem_entry) {
        iface->rem_entry (self, entry);
    }
}

gchar*
device_get_name (Device *self)
{
    DeviceInterface *iface = DEVICE_GET_IFACE (self);

    if (iface->get_name) {
        return iface->get_name (self);
    } else {
        return NULL;
    }
}
