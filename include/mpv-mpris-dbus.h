#ifndef MPV_MPRIS_DBUS_H
#define MPV_MPRIS_DBUS_H

#include "mpv-mpris-types.h"
#include "mpv-mpris-metadata.h"

// D-Bus interface vtables (declare as extern, define in .c file)
extern GDBusInterfaceVTable vtable_root;
extern GDBusInterfaceVTable vtable_player;

/**
 * Handle method calls on the root MPRIS interface
 */
void method_call_root(GDBusConnection *connection,
                     const char *sender,
                     const char *object_path,
                     const char *interface_name,
                     const char *method_name,
                     GVariant *parameters,
                     GDBusMethodInvocation *invocation,
                     gpointer user_data);

/**
 * Get property values for the root MPRIS interface
 */
GVariant *get_property_root(GDBusConnection *connection,
                           const char *sender,
                           const char *object_path,
                           const char *interface_name,
                           const char *property_name,
                           GError **error,
                           gpointer user_data);

/**
 * Set property values for the root MPRIS interface
 */
gboolean set_property_root(GDBusConnection *connection,
                          const char *sender,
                          const char *object_path,
                          const char *interface_name,
                          const char *property_name,
                          GVariant *value,
                          GError **error,
                          gpointer user_data);

/**
 * Handle method calls on the player MPRIS interface
 */
void method_call_player(GDBusConnection *connection,
                       const char *sender,
                       const char *object_path,
                       const char *interface_name,
                       const char *method_name,
                       GVariant *parameters,
                       GDBusMethodInvocation *invocation,
                       gpointer user_data);

/**
 * Get property values for the player MPRIS interface
 */
GVariant *get_property_player(GDBusConnection *connection,
                             const char *sender,
                             const char *object_path,
                             const char *interface_name,
                             const char *property_name,
                             GError **error,
                             gpointer user_data);

/**
 * Set property values for the player MPRIS interface
 */
gboolean set_property_player(GDBusConnection *connection,
                            const char *sender,
                            const char *object_path,
                            const char *interface_name,
                            const char *property_name,
                            GVariant *value,
                            GError **error,
                            gpointer user_data);

/**
 * Emit property change signals on D-Bus
 * @param data User data containing changed properties
 * @return TRUE to continue the timeout
 */
gboolean emit_property_changes(gpointer data);

/**
 * Emit the Seeked signal
 * @param ud User data structure
 */
void emit_seeked_signal(UserData *ud);

/**
 * Called when D-Bus name is acquired
 */
void on_bus_acquired(GDBusConnection *connection,
                    const char *name,
                    gpointer user_data);

/**
 * Called when D-Bus name is lost
 */
void on_name_lost(GDBusConnection *connection,
                 const char *name,
                 gpointer user_data);

#endif // MPV_MPRIS_DBUS_H