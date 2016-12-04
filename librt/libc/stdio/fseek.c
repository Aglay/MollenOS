/* MollenOS
*
* Copyright 2011 - 2016, Philip Meulengracht
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
* MollenOS C Library - File Seek
*/

/* Includes */
#include <io.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <os/Syscall.h>
#include <os/MollenOS.h>

/* Externs */
extern int _finv(FILE * stream);
extern int _favail(FILE * stream);
extern int _fbufptr(FILE * stream);
extern int _fbufadjust(FILE * stream, off_t offset);

/* Helper */
off64_t offabs(off64_t Value) {
	return (Value <= 0) ? 0 - Value : Value;
}

/* The lseeki64
 * This is the ANSI C seek
 * function used by filedescriptors */
off64_t _lseeki64(int fd, off64_t offset, int mode)
{
	/* Syscall Result */
	off_t SeekSpotLow = 0, SeekSpotHigh = 0;
	char Buffer[64];
	int RetVal = 0;

	/* Depends on origin */
	if (mode != SEEK_SET) 
	{
		/* We need current position / size */
		uint64_t fPos = 0, fSize = 0;
		off64_t CorrectedValue = offabs(offset);

		/* Prepare a buffer */
		memset(Buffer, 0, sizeof(Buffer));

		/* Syscall */
		RetVal = Syscall4(MOLLENOS_SYSCALL_VFSQUERY, MOLLENOS_SYSCALL_PARAM(fd),
			MOLLENOS_SYSCALL_PARAM(0),
			MOLLENOS_SYSCALL_PARAM(&Buffer[0]),
			MOLLENOS_SYSCALL_PARAM(sizeof(Buffer)));

		/* Now we can calculate */
		fPos = *((uint64_t*)(&Buffer[16]));
		fSize = *((uint64_t*)(&Buffer[0]));

		/* Sanity offset */
		if ((size_t)fPos != fPos) {
			_set_errno(EOVERFLOW);
			return -1;
		}

		/* Lets see .. */
		if (mode == SEEK_CUR) {
			/* Handle negative */
			if (offset < 0) {
				offset = (long)fPos - CorrectedValue;
			}
			else {
				offset = (long)fPos + CorrectedValue;
			}
		}
		else {
			offset = (long)fSize - CorrectedValue;
		}
	}

	/* Build parts */
	SeekSpotLow = offset & 0xFFFFFFFF;
	SeekSpotHigh = (offset >> 32) & 0xFFFFFFFF;

	/* Seek to 0 */
	RetVal = Syscall3(MOLLENOS_SYSCALL_VFSSEEK, MOLLENOS_SYSCALL_PARAM(fd), 
		MOLLENOS_SYSCALL_PARAM(SeekSpotLow), MOLLENOS_SYSCALL_PARAM(SeekSpotHigh));

	/* Done */
	if (_fval(RetVal)) {
		return -1L;
	}
	else
		return ((off64_t)SeekSpotHigh << 32) | SeekSpotLow;
}

/* The lseek
 * This is the ANSI C seek
 * function used by filedescriptors */
long _lseek(int fd, long offset, int mode)
{
	/* Call the 64 bit version */
	return (long)_lseeki64(fd, offset, mode);
}

/* The seeko
 * Set the file position with off_t */
int fseeko(FILE *stream, off_t offset, int origin)
{
	/* Syscall Result */
	off64_t RetVal = 0;

	/* Sanitize parameters before 
	 * doing anything */
	if (stream == NULL
		|| stream == stdin
		|| stream == stdout
		|| stream == stderr) {
		_set_errno(EINVAL);
		return -1;
	}

	/* Save all output-buffered data */
	if (stream->code & _IOWRT) {
		fflush(stream);
	}

	/* Change from CURRENT to SET if we can */
	if (origin == SEEK_CUR && (stream->code & _IOREAD)) {

		/* First of all, before we actually seek 
		 * can we just seek in current buffer? */
		if (_fbufptr(stream) != -1) {
			if (offset >= 0
				&& _favail(stream) > offset) {
				_fbufadjust(stream, offset);
				goto ExitSeek;
			}
			else if (offset < 0
				&& _fbufptr(stream) >= abs(offset)) {
				_fbufadjust(stream, offset);
				goto ExitSeek;
			}
		}

		/* Nah, do an actual seek */
		origin = SEEK_SET;
		offset += ftell(stream);
	}

	/* Reset direction of i/o */
	if (stream->code & _IORW) {
		stream->code &= ~(_IOREAD | _IOWRT);
	}

	/* Deep call _lseek */
	RetVal = _lseeki64(stream->fd, offset, origin);

	/* Invalidate the file buffer 
	 * otherwise we read from wrong place! */
	_finv(stream);

ExitSeek:
	/* Clear end of file */
	stream->code = ~(_IOEOF);

	/* Done */
	return (RetVal == -1) ? -1 : 0;
}

/* The seek
 * Set the file position */
int fseek(FILE * stream, long int offset, int origin)
{
	/* Deep call */
	return fseeko(stream, offset, origin);
}