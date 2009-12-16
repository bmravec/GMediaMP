/*
 *      device-manager.c
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

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "device-manager.h"
#include "device.h"

#ifdef ENABLE_IPOD
#include "device-ipod.h"
#endif

typedef Device* DeviceNewFunc (Shell*, const gchar*);

DeviceNewFunc *dev_new_funcs[] = {
#ifdef ENABLE_IPOD
    device_ipod_new,
#endif
    NULL
};

#include "shell.h"

G_DEFINE_TYPE(DeviceManager, device_manager, G_TYPE_OBJECT)

struct _DeviceManagerPrivate {
    Shell *shell;

    GVolumeMonitor *monitor;
};

static void
device_manager_finalize (GObject *object)
{
    DeviceManager *self = DEVICE_MANAGER (object);

    G_OBJECT_CLASS (device_manager_parent_class)->finalize (object);
}

static void
device_manager_class_init (DeviceManagerClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (DeviceManagerPrivate));

    object_class->finalize = device_manager_finalize;
}

static void
device_manager_init (DeviceManager *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), DEVICE_MANAGER_TYPE, DeviceManagerPrivate);
}

DeviceManager*
device_manager_new (Shell *shell)
{
    DeviceManager *self = g_object_new (DEVICE_MANAGER_TYPE, NULL);

    self->priv->shell = g_object_ref (shell);

    self->priv->monitor = g_volume_monitor_get ();

    GList *mps, *iter;
    mps = g_volume_monitor_get_mounts (self->priv->monitor);

    for (iter = mps; iter; iter = iter->next) {
        GMount *mp = G_MOUNT (iter->data);
        gchar *mpname = g_mount_get_name (mp);
        GFile *mproot = g_mount_get_root (mp);
        gchar *rpath = g_file_get_path (mproot);

        Device *new_dev = NULL;
        gint i;
        for (i = 0; dev_new_funcs[i] && !new_dev; i++) {
            new_dev = dev_new_funcs[i] (self->priv->shell, rpath);
        }

//        g_print ("Mount at %s :: %s :: ", mpname, rpath);

        if (new_dev) {
//            g_print ("Is a %s device\n", device_get_name (new_dev));
//            g_object_unref (new_dev);
        } else {
//            g_print ("Not a valid media device\n");
        }

        g_free (mpname);
        g_free (rpath);

        g_object_unref (mproot);

        g_object_unref (mp);
    }

    g_list_free (mps);

    g_print ("Device manager ready %x\n", self->priv->monitor);

    return self;
}
