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
#include <internal/_file.h>
#include <internal/_io.h>
#include <io.h>
#include <os/services/file.h>
#include <stdio.h>

FILE* freopen(
	_In_ const char* filename, 
	_In_ const char* mode, 
	_In_ FILE*       stream)
{
	stdio_handle_t* handle;
	int             open_flags;
	int             stream_flags;
	int             fd;

	// Sanitize parameters
	if ((filename == NULL && mode == NULL)
		|| stream == NULL || stream == stdin
		|| stream == stdout || stream == stderr) {
		_set_errno(EINVAL);
		return NULL;
	}

	// Ok, if filename is not null we must open a new file
    flockfile(stream);
	if (filename != NULL) {
		close(stream->_fd);

		// Open a new file descriptor
		_fflags(mode, &open_flags, &stream_flags);
		fd = open(filename, open_flags, 0755);
		if (fd == -1) {
            funlockfile(stream);
			return NULL;
		}
		handle = stdio_handle_get(fd);
		stdio_handle_set_buffered(handle, stream, stream_flags);
	} else {
		if (mode != NULL) {
			oserr_t status;

            _fflags(mode, &open_flags, &stream_flags);

			// TODO: support multiple types of streams
            status = ChangeFileHandleAccessFromFd(stream->_fd, _fopts(open_flags));
			OsErrToErrNo(status);
		}
	}
	stream->_flag &= ~(_IOEOF | _IOERR);
    funlockfile(stream);
	return stream;
}
