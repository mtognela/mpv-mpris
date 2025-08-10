/*
    MIT License

    Copyright (c) 2025 Mattia Tognela

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef MPV_MPRIS_DBUS_H
#define MPV_MPRIS_DBUS_H

#include "mpv-mpris-types.h"
#include "mpv-mpris-metadata.h"

extern GDBusInterfaceVTable vtable_root;
extern GDBusInterfaceVTable vtable_player;

void method_call_root(GDBusConnection *connection,
                     const char *sender,
                     const char *object_path,
                     const char *interface_name,
                     const char *method_name,
                     GVariant *parameters,
                     GDBusMethodInvocation *invocation,
                     gpointer user_data);

GVariant *get_property_root(GDBusConnection *connection,
                           const char *sender,
                           const char *object_path,
                           const char *interface_name,
                           const char *property_name,
                           GError **error,
                           gpointer user_data);

gboolean set_property_root(GDBusConnection *connection,
                          const char *sender,
                          const char *object_path,
                          const char *interface_name,
                          const char *property_name,
                          GVariant *value,
                          GError **error,
                          gpointer user_data);

void method_call_player(GDBusConnection *connection,
                       const char *sender,
                       const char *object_path,
                       const char *interface_name,
                       const char *method_name,
                       GVariant *parameters,
                       GDBusMethodInvocation *invocation,
                       gpointer user_data);

GVariant *get_property_player(GDBusConnection *connection,
                             const char *sender,
                             const char *object_path,
                             const char *interface_name,
                             const char *property_name,
                             GError **error,
                             gpointer user_data);

gboolean set_property_player(GDBusConnection *connection,
                            const char *sender,
                            const char *object_path,
                            const char *interface_name,
                            const char *property_name,
                            GVariant *value,
                            GError **error,
                            gpointer user_data);

gboolean emit_property_changes(gpointer data);

void emit_seeked_signal(UserData *ud);

void on_bus_acquired(GDBusConnection *connection,
                    const char *name,
                    gpointer user_data);

void on_name_lost(GDBusConnection *connection,
                 const char *name,
                 gpointer user_data);

#endif // MPV_MPRIS_DBUS_H
