/*
 *      shell.c
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

#include "shell.h"
#include "player.h"

G_DEFINE_TYPE(Shell, shell, G_TYPE_OBJECT)

struct _ShellPrivate {
    Player *player;
};

const gchar *builtin_uis[] = {
//    SHARE_DIR "ui/main.ui",
//    SHARE_DIR "ui/browser.ui"
    "data/ui/main.ui",
    "data/ui/browser.ui"
};

guint signal_eos;
guint signal_tag;
guint signal_state;
guint signal_ratio;

static gdouble prev_ratio;
static Shell *instance = NULL;

static void
shell_finalize (GObject *object)
{
    Shell *self = SHELL (object);
    
    G_OBJECT_CLASS (shell_parent_class)->finalize (object);
}

static void
shell_class_init (ShellClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (ShellPrivate));
    
    object_class->finalize = shell_finalize;
    
    signal_eos = g_signal_new ("eos", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    
    signal_tag = g_signal_new ("tag", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    
    signal_state = g_signal_new ("state-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);
    
    signal_ratio = g_signal_new ("ratio-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__DOUBLE,
        G_TYPE_NONE, 1, G_TYPE_DOUBLE);
    
    gint i;
    for (i = 0; i < sizeof (builtin_uis) / sizeof (gchar*); i++) {
        g_print ("UI: %s\n", builtin_uis[i]);
    }
}

static void
shell_init (Shell *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), SHELL_TYPE, ShellPrivate);
}

Shell*
shell_new ()
{
    if (!instance) {
        instance = g_object_new (SHELL_TYPE, NULL);
    }

    return instance;
}

Player*
shell_get_player (Shell *self)
{
    return self->priv->player;
}

