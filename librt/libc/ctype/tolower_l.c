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
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
*
* MollenOS C Library - Standard Library Wide Conversion Macros
*/

/* Define posix */
#define __POSIX_VISIBLE

/* Includes */
#include <locale.h>
#include <ctype.h>
#if defined (_MB_EXTENDED_CHARSETS_ISO) || defined (_MB_EXTENDED_CHARSETS_WINDOWS)
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <wctype.h>
#include <wchar.h>
#include <internal/_locale.h>
#endif

int tolower_l(int c, struct __locale_t *locale)
{
#if defined (_MB_EXTENDED_CHARSETS_ISO) || defined (_MB_EXTENDED_CHARSETS_WINDOWS)
  if ((unsigned char) c <= 0x7f) 
    return isupper_l (c, locale) ? c - 'A' + 'a' : c;
  else if (c != EOF && __locale_mb_cur_max_l (locale) == 1
	   && isupper_l (c, locale))
    {
      char s[MB_LEN_MAX] = { c, '\0' };
      wchar_t wc;
      mbstate_t state;

      memset (&state, 0, sizeof state);
      if (locale->mbtowc (_REENT, &wc, s, 1, &state) >= 0
	  && locale->wctomb (_REENT, s,
			     (wchar_t) towlower_l ((wint_t) wc, locale),
			     &state) == 1)
	c = (unsigned char) s[0];
    }
  return c;
#else
  return isupper_l(c, locale) ? (c) - 'A' + 'a' : c;
#endif
}
