/**
 * Copyright 2017, Philip Meulengracht
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
 */

#include <time.h>
#include "local.h"

static inline void __normalize(struct timespec* ts) {
    while (ts->tv_nsec >= NSEC_PER_SEC) {
        ts->tv_sec++;
        ts->tv_nsec -= NSEC_PER_SEC;
    }
    while (ts->tv_nsec < 0) {
        ts->tv_sec--;
        ts->tv_nsec += NSEC_PER_SEC;
    }
}

void timespec_sub(const struct timespec* a, const struct timespec* b, struct timespec* result)
{
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    __normalize(result);
}

void timespec_add(const struct timespec* a, const struct timespec* b, struct timespec* result)
{
    result->tv_sec  = a->tv_sec  + b->tv_sec;
    result->tv_nsec = a->tv_nsec + b->tv_nsec;
    __normalize(result);
}
