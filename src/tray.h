/*
 *      tray.h
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

#ifndef __TRAY_H__
#define __TRAY_H__

#include <gtk/gtkstatusicon.h>

#define TRAY_TYPE (tray_get_type ())
#define TRAY(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), TRAY_TYPE, Tray))
#define TRAY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TRAY_TYPE, TrayClass))
#define IS_TRAY(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), TRAY_TYPE))
#define IS_TRAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TRAY_TYPE))
#define TRAY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TRAY_TYPE, TrayClass))

G_BEGIN_DECLS

typedef struct _Tray Tray;
typedef struct _TrayClass TrayClass;
typedef struct _TrayPrivate TrayPrivate;

struct _Tray {
    GObject parent;

    TrayPrivate *priv;
};

struct _TrayClass {
    GObjectClass parent;
};

Tray *tray_new ();
GType tray_get_type (void);

void tray_set_type (Tray *self, gint state);

void tray_activate (Tray *self);
void tray_deactivate (Tray *self);

G_END_DECLS

#endif /* __TRAY_H__ */
