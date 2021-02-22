/**
 * MollenOS
 *
 * Copyright 2021, Philip Meulengracht
 *
 * This program is free software : you can redistribute it and / or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ? , either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Service - Usb Manager
 * - Contains the implementation of the usb-manager which keeps track
 *   of all usb-controllers and their devices
 */

#define __TRACE

#include <ddk/usbdevice.h>
#include <usb/usb.h>
#include <ddk/device.h>
#include <ddk/utils.h>
#include <internal/_ipc.h>
#include "manager.h"
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "svc_usb_protocol_server.h"

void __HandlePortEvent(
        _In_ UUId_t  hubDeviceId,
        _In_ uint8_t portAddress)
{
    UsbController_t*      controller = NULL;
    UsbHcPortDescriptor_t portDescriptor;
    OsStatus_t            osStatus = OsSuccess;
    UsbHub_t*             hub;
    UsbPort_t*            port = NULL;

    TRACE("__HandlePortEvent(deviceId=%u, portAddress=%u)", hubDeviceId, portAddress);

    // Retrieve hub, controller and port instances
    hub = UsbCoreHubsGet(hubDeviceId);
    if (!hub) {
        ERROR("__HandlePortEvent hub device id was invalid");
        return;
    }

    controller = UsbCoreControllerGet(hub->ControllerDeviceId);
    if (!controller) {
        ERROR("__HandlePortEvent controller related to hub was invalid");
        return;
    }

    // Query port status so we know the status of the port
    // Also compare to the current state to see if the change was valid
    if (UsbHubQueryPort(hub->DriverId, hubDeviceId, portAddress, &portDescriptor) != OsSuccess) {
        ERROR("__HandlePortEvent Query port failed");
        return;
    }

    port = UsbCoreHubsGetPort(hub, portAddress);
    if (!port) {
        ERROR("__HandlePortEvent failed to retrieve port instance");
        return;
    }

    // Now handle connection events
    if (portDescriptor.Connected == 1 && port->Connected == 0) {
        // Connected event
        // This function updates port-status after reset
        osStatus = UsbCoreDevicesCreate(controller, hub, port);
    }
    else if (portDescriptor.Connected == 0 && port->Connected == 1) {
        // Disconnected event, remember that the descriptor pointer
        // becomes unavailable the moment we call the destroy device
        osStatus = UsbCoreDevicesDestroy(controller, port);
        port->Speed     = portDescriptor.Speed;              // TODO: invalid
        port->Enabled   = portDescriptor.Enabled;            // TODO: invalid
        port->Connected = portDescriptor.Connected;          // TODO: invalid
    }
    else {
        // Ignore
        port->Speed     = portDescriptor.Speed;
        port->Enabled   = portDescriptor.Enabled;
        port->Connected = portDescriptor.Connected;
    }
}

void __HandlePortError(
        _In_ UUId_t  hubDeviceId,
        _In_ uint8_t portAddress)
{
    UsbController_t* controller;
    UsbHub_t*        hub;
    UsbPort_t*       port;

    TRACE("__HandlePortError(deviceId=%u, portAddress=%u)", hubDeviceId, portAddress);

    // Retrieve hub, controller and port instances
    hub = UsbCoreHubsGet(hubDeviceId);
    if (!hub) {
        ERROR("__HandlePortError hub device id was invalid");
        return;
    }

    controller = UsbCoreControllerGet(hub->ControllerDeviceId);
    if (!controller) {
        ERROR("__HandlePortError controller related to hub was invalid");
        return;
    }

    port = UsbCoreHubsGetPort(hub, portAddress);
    if (port && port->Device) {
        UsbCoreDevicesDestroy(controller, port);
    }
}

void svc_usb_port_event_callback(struct gracht_recv_message* message, struct svc_usb_port_event_args* args)
{
    __HandlePortEvent(args->hub_device_id, args->port_address);
}

void svc_usb_port_error_callback(struct gracht_recv_message* message, struct svc_usb_port_error_args* args)
{
    __HandlePortError(args->hub_device_id, args->port_address);
}