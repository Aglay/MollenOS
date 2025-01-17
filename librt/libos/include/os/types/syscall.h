/**
 * Copyright 2022, Philip Meulengracht
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
 */

#ifndef __TYPES_SYSCALL_H__
#define __TYPES_SYSCALL_H__

#include <os/types/time.h>

typedef struct HandleSetWaitParameters {
    struct ioset_event* Events;
    int                 MaxEvents;
    OSTimestamp_t*      Deadline;
    int                 PollEvents;
} HandleSetWaitParameters_t;

typedef struct OSFutexParameters {
    _Atomic(int)*  Futex0;
    _Atomic(int)*  Futex1;
    int            Expected0;
    int            Count;
    int            Op;
    int            Flags;
    OSTimestamp_t* Deadline;
} OSFutexParameters_t;

#endif //!__TYPES_SYSCALL_H__
