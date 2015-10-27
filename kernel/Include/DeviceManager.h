/* MollenOS
*
* Copyright 2011 - 2014, Philip Meulengracht
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
* MollenOS - Device Manager
*/

#ifndef _MCORE_DRIVER_MANAGER_H_
#define _MCORE_DRIVER_MANAGER_H_

/* Includes */
#include <stdint.h>
#include <Mutex.h>

/* Definitions */
typedef int DevId_t;

/* Device Types */
typedef enum _DeviceType
{
	DeviceCpu,
	DeviceApic,
	DeviceCmos,
	DeviceTimer,
	DeviceInput,
	DeviceStorage,
	DeviceVideo

} DeviceType_t;

/* Structures */
typedef struct _MCoreDevice
{
	/* Name */
	char *Name;

	/* System Id */
	DevId_t Id;

	/* Type */
	DeviceType_t Type;

	/* Device Data */
	void *Data;

} MCoreDevice_t;

/* Prototypes */
_CRT_EXTERN void DmInit(void);

_CRT_EXTERN DevId_t DmCreateDevice(char *Name, DeviceType_t Type, void *Data);
_CRT_EXTERN void *DmGetDevice(DeviceType_t Type);
_CRT_EXTERN void DmDestroyDevice(DevId_t DeviceId);

#endif //_MCORE_DRIVER_MANAGER_H_