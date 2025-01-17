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

#include <errno.h>
#include <io.h>
#include <internal/_io.h>

int dup(int iod)
{
    stdio_handle_t* objectToCopy = stdio_handle_get(iod);
    stdio_handle_t* duplicatedObject;
    int             duplicatedIod;

    if (!objectToCopy) {
        errno = (EBADFD);
        return -1;
    }

    // So duplicated handles we never inherit unless specifically requested
    duplicatedIod = stdio_handle_create(-1, objectToCopy->wxflag | WX_DONTINHERIT, &duplicatedObject);
    if (duplicatedIod < 0) {
        return -1;
    }

    // copy data
    stdio_handle_clone(duplicatedObject, objectToCopy);
    return duplicatedIod;
}
