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