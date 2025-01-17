/*-
* Copyright (c) 2003, Steven G. Kargl
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice unmodified, this list of conditions, and the following
*    disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "private.h"
#include <limits.h>
#include <math.h>
#include <fenv.h>

#ifndef type
#define type		double
#define	roundit		round
#define dtype		long
#define	DTYPE_MIN	LONG_MIN
#define	DTYPE_MAX	LONG_MAX
#define	fn		lround
#endif

#ifdef _MSC_VER
#pragma warning(disable:4305)
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-const-int-float-conversion"
#endif

/*
* If type has more precision than dtype, the endpoints dtype_(min|max) are
* of the form xxx.5; they are "out of range" because lround() rounds away
* from 0.  On the other hand, if type has less precision than dtype, then
* all values that are out of range are integral, so we might as well assume
* that everything is in range.  At compile time, INRANGE(x) should reduce to
* two floating-point comparisons in the former case, or TRUE otherwise.
*/
static const type dtype_min = DTYPE_MIN - 0.5;
static const type dtype_max = DTYPE_MAX + 0.5;
#define	INRANGE(x)	(dtype_max - DTYPE_MAX != 0.5 || \
			 ((x) > dtype_min && (x) < dtype_max))

dtype fn(type x)
{
	if (INRANGE(x)) {
		x = roundit(x);
		return ((dtype)x);
	}
	else {
		feraiseexcept(FE_INVALID);
		return (DTYPE_MAX);
	}
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(default:4305)
#endif
