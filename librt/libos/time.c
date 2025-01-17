/**
 * Copyright 2011, Philip Meulengracht
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * MollenOS System Interface
 */

#include <internal/_syscalls.h>
#include <internal/_utils.h>
#include <os/time.h>
#include <os/services/process.h>

oserr_t
OSGetTime(
        _In_ enum OSTimeSource source,
        _In_ OSTimestamp_t*    time)
{
    if (!time) {
        return OS_EINVALPARAMS;
    }
    return Syscall_Time(source, time);
}

oserr_t
OSGetClockTick(
        _In_ enum OSClockSource source,
        _In_ UInteger64_t*          tickOut)
{
    if (!tickOut) {
        return OS_EINVALPARAMS;
    }

    if (source == OSClockSource_PROCESS && !__crt_is_phoenix()) {
        clock_t tickBase;
        oserr_t oserr = OSProcessTickBase(&tickBase);
        if (oserr != OS_EOK) {
            return oserr;
        }

        tickOut->QuadPart = (uint64_t)tickBase;
        return OS_EOK;
    }
    return Syscall_ClockTick(source, tickOut);
}

oserr_t
OSGetClockFrequency(
        _In_ enum OSClockSource source,
        _In_ UInteger64_t*       frequencyOut)
{
    if (!frequencyOut) {
        return OS_EINVALPARAMS;
    }

    // The frequency is a bit more funny, because all ticks inheritly source the same frequency
    // so the source, unless HPC is requested, is relatively #dontcare.
    return Syscall_ClockFrequency(source, frequencyOut);
}

oserr_t
OSSleep(
        _In_      const OSTimestamp_t* deadline,
        _Out_Opt_ OSTimestamp_t*       remaining)
{
    if (deadline == NULL) {
        return OS_EINVALPARAMS;
    }
    return Syscall_Sleep(deadline, remaining);
}

oserr_t
OSStall(
        _In_ UInteger64_t* duration)
{
    if (!duration || !duration->QuadPart) {
        return OS_EINVALPARAMS;
    }

    return Syscall_Stall(duration);
}
