/*
 *      plugin-manager.h
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

#ifndef __PLUGIN_MANAGER_H__
#define __PLUGIN_MANAGER_H__

#include <glib-object.h>

#define PLUGIN_MANAGER_TYPE (plugin_manager_get_type ())
#define PLUGIN_MANAGER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), PLUGIN_MANAGER_TYPE, PluginManager))
#define PLUGIN_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PLUGIN_MANAGER_TYPE, PluginManagerClass))
#define IS_PLUGIN_MANAGER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PLUGIN_MANAGER_TYPE))
#define IS_PLUGIN_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLUGIN_MANAGER_TYPE))
#define PLUGIN_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PLUGIN_MANAGER_TYPE, PluginManagerClass))

G_BEGIN_DECLS

typedef struct _PluginManager PluginManager;
typedef struct _PluginManagerClass PluginManagerClass;
typedef struct _PluginManagerPrivate PluginManagerPrivate;

struct _PluginManager {
    GObject parent;

    PluginManagerPrivate *priv;
};

struct _PluginManagerClass {
    GObjectClass parent;
};

PluginManager *plugin_manager_new ();
GType plugin_manager_get_type (void);

G_END_DECLS

#endif /* __PLUGIN_MANAGER_H__ */
