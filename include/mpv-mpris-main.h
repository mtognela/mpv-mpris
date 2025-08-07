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

#ifndef MPV_MPRIS_MAIN_H
#define MPV_MPRIS_MAIN_H

#include "mpv-mpris-types.h"
#include "mpv-mpris-artwork.h"
#include "mpv-mpris-metadata.h"
#include "mpv-mpris-dbus.h"
#include "mpv-mpris-events.h"

/**
 * Initialize the UserData structure
 * @param ud Pointer to UserData structure to initialize
 * @param mpv_handle MPV handle to associate with the user data
 * @return TRUE on success, FALSE on failure
 */
gboolean init_user_data(UserData *ud, mpv_handle *mpv_handle);

/**
 * Cleanup the UserData structure and free resources
 * @param ud Pointer to UserData structure to cleanup
 */
void cleanup_user_data(UserData *ud);

/**
 * Initialize MPV properties and observers
 * @param ud User data structure
 * @return TRUE on success, FALSE on failure
 */
gboolean init_mpv_properties(UserData *ud);

/**
 * Initialize D-Bus interfaces and introspection data
 * @param ud User data structure
 * @return TRUE on success, FALSE on failure
 */
gboolean init_dbus_interfaces(UserData *ud);

/**
 * Setup event handling for MPV
 * @param ud User data structure
 * @param wakeup_fd File descriptor for wakeup notifications
 * @return TRUE on success, FALSE on failure
 */
gboolean setup_event_handling(UserData *ud, int wakeup_fd);

/**
 * Main application entry point
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit status code
 */
int main(int argc, char *argv[]);

#endif // MPV_MPRIS_MAIN_H