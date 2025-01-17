/* MollenOS
 *
 * Copyright 2019, Philip Meulengracht
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
 * Process Type Definitions & Structures
 * - This header describes the base process-structure, prototypes
 *   and functionality, refer to the individual things for descriptions
 */

#ifndef __TYPES_PROCESS_H__
#define __TYPES_PROCESS_H__

#include <os/osdefs.h>

// PROCESS_MAXMODULES is the maximum number of PE modules that can be loaded
// into a process space. This is an arbitrary limit, and can be raised as fit.
// This limit should always be a multiple of 8, to account for the bitmap allocator.
#define PROCESS_MAXMODULES 64

// PROCESS_INHERIT_* flags denote which kind of file-descriptor inheritation that
// should be done when spawning processes. None simply means nothing will be inheritted,
// and even std{in,out,err} should be NULL file-descriptors.
#define PROCESS_INHERIT_NONE   0x00000000
#define PROCESS_INHERIT_STDOUT 0x00000001
#define PROCESS_INHERIT_STDIN  0x00000002
#define PROCESS_INHERIT_STDERR 0x00000004
#define PROCESS_INHERIT_FILES  0x00000008
#define PROCESS_INHERIT_ALL    (PROCESS_INHERIT_STDOUT | PROCESS_INHERIT_STDIN | PROCESS_INHERIT_STDERR | PROCESS_INHERIT_FILES)

typedef struct OSProcessOptions OSProcessOptions_t;

// ProcessStartupInformation is used internally for processes during startup
// to transfer certain configuration data between the process manager and the
// new process itself.
typedef struct ProcessStartupInformation {
    size_t ArgumentsLength;
    size_t InheritationLength;
    size_t LibraryEntriesLength;
    size_t EnvironmentBlockLength;
} ProcessStartupInformation_t;

#endif //!__TYPES_PROCESS_H__
