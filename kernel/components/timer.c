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
 *
 * MollenOS System Component Infrastructure
 * - The Memory component. This component has the task of managing
 *   different memory regions that map to physical components
 */

#define __TRACE

#include <assert.h>
#include <component/timer.h>
#include <debug.h>
#include <heap.h>
#include <machine.h>
#include <string.h>

// for time macros and length of months
#include "../../librt/libc/time/local.h"

static inline uint32_t
__CalculateResolution(
        _In_ UInteger64_t* frequency)
{
    // TODO: should support both a multipler and a divider to account
    // for any timer with a frequency above 1Ghz. Since we account for time in NS
    // we will end up with a zero resolution in this case.
    assert(frequency->QuadPart <= NSEC_PER_SEC);
    return (uint32_t)(NSEC_PER_SEC / frequency->QuadPart);
}

static inline void
__ToTimestamp(
        _In_ uint32_t       multiplier,
        _In_ uint32_t       divider,
        _In_ UInteger64_t*  frequency,
        _In_ UInteger64_t*  tick,
        _In_ OSTimestamp_t* timestamp)
{
    timestamp->Seconds = (int64_t)(tick->QuadPart / frequency->QuadPart);
    timestamp->Nanoseconds = (int64_t)(((tick->QuadPart % frequency->QuadPart) * multiplier) / divider);
}

oserr_t
SystemTimerRegister(
        _In_ const char*               name,
        _In_ SystemTimerOperations_t*  operations,
        _In_ enum SystemTimeAttributes attributes,
        _In_ void*                     context)
{
    SystemTimer_t* systemTimer;
    TRACE("SystemTimerRegister(name=%s, attributes=0x%x)", name, attributes);

    systemTimer = (SystemTimer_t*)kmalloc(sizeof(SystemTimer_t));
    if (!systemTimer) {
        return OS_EOOM;
    }

    memset(systemTimer, 0, sizeof(SystemTimer_t));
    ELEMENT_INIT(&systemTimer->ListHeader, 0, systemTimer);
    memcpy(&systemTimer->Operations, operations, sizeof(SystemTimerOperations_t));
    systemTimer->Name       = name;
    systemTimer->Attributes = attributes;
    systemTimer->Context    = context;
    list_append(&GetMachine()->SystemTimers.Timers, &systemTimer->ListHeader);
    return OS_EOK;
}

// Our system wall clock is valid down to microseconds precision, which allows us for
// a precision of 292.277 years in either direction. This should be sufficient for our
// needs.
static void __LinearTime(SystemTime_t* time, Integer64_t* linear)
{
    // Ok we convert the format of SystemTime to a total second count. We count
    // time starting from January 1, 2000. (UTC) from the value of 0.
    int isLeap = isleap(time->Year);
    int seconds;
    int days;

    if (time->Year < 2000) {
        // calculate the time left in the day, and the days left in the current year
        seconds = SECSPERDAY - (time->Second + (time->Minute * (int)SECSPERMIN) + (time->Hour * (int)SECSPERHOUR));
        days    = DAYSPERYEAR(time->Year) - (__days_before_month[isLeap][time->Month] + time->DayOfMonth);

        // date is in the past, we must count backwards from that date, start by calculating
        // the full number of days
        for (int i = 1999; i > time->Year; i--) {
            days += DAYSPERYEAR(i);
        }

        // do the last conversion of days to seconds and return that value as a negative
        linear->QuadPart = -((days * SECSPERDAY) + seconds);
    } else {
        seconds = time->Second + (time->Minute * (int)SECSPERMIN) + (time->Hour * (int)SECSPERHOUR);
        days    = __days_before_month[isLeap][time->Month] + time->DayOfMonth;
        for (int i = 2000; i < time->Year; i++) {
            days += DAYSPERYEAR(i);
        }
        linear->QuadPart = (days * SECSPERDAY) + seconds;
    }
}

oserr_t
SystemWallClockRegister(
        _In_ SystemWallClockOperations_t* operations,
        _In_ void*                        context)
{
    SystemWallClock_t* clock;

    if (GetMachine()->SystemTimers.WallClock != NULL) {
        return OS_EEXISTS;
    }

    clock = kmalloc(sizeof(SystemWallClock_t));
    if (clock == NULL) {
        return OS_EOOM;
    }

    clock->BaseTick.QuadPart = 0;
    clock->Context = context;
    memcpy(&clock->Operations, operations, sizeof(SystemWallClockOperations_t));

    // store it as our primary wall clock
    GetMachine()->SystemTimers.WallClock = clock;
    return OS_EOK;
}

void
SystemTimerGetWallClockTime(
        _In_ OSTimestamp_t* time)
{
    SystemTimer_t* clock;
    UInteger64_t   tick;

    // Should there be no wall clock in the system, then we just
    // convert the ticks from the clock.
    if (GetMachine()->SystemTimers.WallClock == NULL) {
        tick_t timestamp;
        SystemTimerGetTimestamp(&timestamp);
        time->Seconds = (int64_t)(timestamp / NSEC_PER_SEC);
        time->Nanoseconds = (int64_t)(timestamp % NSEC_PER_SEC);
        return;
    }

    clock = GetMachine()->SystemTimers.Clock;

    // The wall clock and default system timer are synchronized. Which means
    // we use the BaseTick of the wall clock, and then add clock timestamp
    // to that to get the final time.
    if (!clock) {
        time->Seconds = GetMachine()->SystemTimers.WallClock->BaseTick.QuadPart;
        time->Nanoseconds = 0;
        return;
    }

    // get current clock tick & subtract base offset
    clock->Operations.Read(clock->Context, &tick);
    tick.QuadPart -= GetMachine()->SystemTimers.WallClock->BaseOffset.QuadPart;

    // Convert from raw ticks to something we recognize
    __ToTimestamp(
            clock->Multiplier,
            1,
            &clock->Frequency,
            &tick,
            time
    );
    time->Seconds += GetMachine()->SystemTimers.WallClock->BaseTick.QuadPart;
}

void
SystemTimerGetTimestamp(
        _Out_ tick_t* timestampOut)
{
    SystemTimer_t* clock = GetMachine()->SystemTimers.Clock;
    UInteger64_t   tick;
    OSTimestamp_t  timestamp;

    // guard against early calls from the log
    if (!clock) {
        *timestampOut = 0;
        return;
    }

    // get current clock tick & subtract inital reading
    clock->Operations.Read(clock->Context, &tick);
    tick.QuadPart -= clock->InitialTick.QuadPart;

    // Convert from raw ticks to something we recognize
    __ToTimestamp(
            clock->Multiplier,
            1,
            &clock->Frequency,
            &tick,
            &timestamp
    );
    *timestampOut = (timestamp.Seconds * NSEC_PER_SEC) + timestamp.Nanoseconds;
}

void
SystemTimerGetClockTick(
        _In_ UInteger64_t* tickOut)
{
    SystemTimer_t* clock = GetMachine()->SystemTimers.Clock;
    if (!clock) {
        tickOut->QuadPart = 0;
        return;
    }
    clock->Operations.Read(clock->Context, tickOut);
}

void
SystemTimerGetClockFrequency(
        _In_ UInteger64_t* frequencyOut)
{
    SystemTimer_t* clock = GetMachine()->SystemTimers.Clock;
    if (!clock) {
        frequencyOut->QuadPart = 0;
        return;
    }
    clock->Operations.GetFrequency(clock->Context, frequencyOut);
}

oserr_t
SystemTimerGetPerformanceFrequency(
        _Out_ UInteger64_t* frequency)
{
    SystemTimer_t* hpc = GetMachine()->SystemTimers.Hpc;
    if (!hpc) {
        return OS_ENOTSUPPORTED;
    }
    hpc->Operations.GetFrequency(hpc->Context, frequency);
    return OS_EOK;
}

oserr_t
SystemTimerGetPerformanceTick(
        _Out_ UInteger64_t* tick)
{
    SystemTimer_t* hpc = GetMachine()->SystemTimers.Hpc;
    if (!hpc) {
        return OS_ENOTSUPPORTED;
    }
    hpc->Operations.Read(hpc->Context, tick);
    return OS_EOK;
}

void
SystemTimerStall(
        _In_ OSTimestamp_t* deadline)
{
    OSTimestamp_t currentTime;
    do {
        SystemTimerGetWallClockTime(&currentTime);
    } while (OSTimestampCompare(&currentTime, deadline) == -1);
}

static void
__SynchronizeWallClockAndClocks(
        _In_ void* argument)
{
    SystemTime_t       systemTime;
    SystemWallClock_t* wallClock;

    _CRT_UNUSED(argument);
    TRACE("__SynchronizeWallClockAndClocks");

    // Two requirements must be satisfied for us to do so. There must
    // be a wall clock registered, and there must be a clock source. Otherwise,
    // there is nothing to do.
    if (GetMachine()->SystemTimers.Clock == NULL ||
        GetMachine()->SystemTimers.WallClock == NULL) {
        TRACE("__SynchronizeWallClockAndClocks no clocks to synchronize");
        return;
    }

    wallClock = GetMachine()->SystemTimers.WallClock;

    // Read the system time, and then read the wall-clock
    wallClock->Operations.PerformSync(wallClock->Context);
    wallClock->Operations.Read(wallClock->Context, &systemTime);
    GetMachine()->SystemTimers.Clock->Operations.Read(
            GetMachine()->SystemTimers.Clock->Context,
            &wallClock->BaseOffset
    );

    TRACE("__SynchronizeWallClockAndClocks synced at %i:%i:%i (0x%llx)",
          systemTime.Hour, systemTime.Minute, systemTime.Second,
          wallClock->BaseOffset.QuadPart
    );

    // Convert the system time to a time-point based on the epoch of January 1, 2000
    __LinearTime(&systemTime, &wallClock->BaseTick);
}

static void __SelectTimerFrequency(
        UInteger64_t* low,
        UInteger64_t* high,
        UInteger64_t* result)
{
    UInteger64_t target = { .QuadPart = 1000 };

    // TODO: this should be dynamic based on current cpu speed. On high speed CPUs
    // we can go for higher clocks, but on low speed we should aim to go as high as 10ms
    // precision, or as low as 100hz. Optimally we go for 1000-10000hz.
    if (low->QuadPart >= target.QuadPart) {
        target.QuadPart = low->QuadPart;
    }
    if (high->QuadPart < target.QuadPart) {
        target.QuadPart = high->QuadPart;
    }
    TRACE("__SelectTimerFrequency selected speed %u hz", target.u.LowPart);
    result->QuadPart = target.QuadPart;
}

static void  __CalculateTimerFrequency(
        _In_ SystemTimer_t* timer)
{
    assert(timer->Operations.GetFrequency != NULL);

    // Does the timer support variadic frequency?
    if (timer->Operations.GetFrequencyRange != NULL) {
        UInteger64_t low, high;

        // If we support the frequency range, then we must support configure.
        assert(timer->Operations.Configure != NULL);
        timer->Operations.GetFrequencyRange(timer->Context, &low, &high);
        __SelectTimerFrequency(&low, &high, &timer->Frequency);
    } else {
        timer->Operations.GetFrequency(timer->Context, &timer->Frequency);
    }
    timer->Multiplier = __CalculateResolution(&timer->Frequency);
}

static inline void __SelectIfFrequencyIsHigher(
        _In_ SystemTimer_t** current,
        _In_ SystemTimer_t*  candidate)
{
    if (*current == NULL) {
        *current = candidate;
        return;
    }
    if ((*current)->Frequency.QuadPart < candidate->Frequency.QuadPart) {
        *current = candidate;
    }
}

static oserr_t __EnableTimer(
        _In_ SystemTimer_t* timer)
{
    TRACE("__EnableTimer %s", timer->Name);

    // Recalculate a few things, including the frequency if it's variadic
    __CalculateTimerFrequency(timer);

    // Configure the timer if it was supported
    if (timer->Operations.Configure != NULL) {
        oserr_t oserr = timer->Operations.Configure(timer->Context, &timer->Frequency);
        if (oserr != OS_EOK) {
            return oserr;
        }

        // After the timer has been configured, the frequency we requested may or may not
        // end up being exactly that due to hardware limitations. So it's important that we
        // re-read the *actual* frequency we ended up with
        timer->Operations.GetFrequency(timer->Context, &timer->Frequency);
    }

    // Enable it if the operation is supported
    if (timer->Operations.Enable != NULL) {
        oserr_t oserr = timer->Operations.Enable(timer->Context, true);
        if (oserr != OS_EOK) {
            return oserr;
        }
    }

    // Update initial tick for that timer
    timer->Operations.Read(timer->Context, &timer->InitialTick);
    TRACE("__EnableTimer frequency=%llu, tick=%llu", timer->Frequency.QuadPart, timer->InitialTick.QuadPart);
    return OS_EOK;
}

static void __ConfigureTimerSources(void)
{
    SystemTimer_t* irqCandidate     = NULL;
    SystemTimer_t* counterCandidate = NULL;
    oserr_t        oserr;
    TRACE("__ConfigureTimerSources");

    // We always prefer counters against IRQ triggered timers, but sometimes
    // these are not available. Let's loop through counters and find the best counter
    // and irq timers, then we make a decision between both.
    foreach (i, &GetMachine()->SystemTimers.Timers) {
        SystemTimer_t* timer = i->value;
        __CalculateTimerFrequency(timer);
        if (timer->Attributes & SystemTimeAttributes_IRQ) {
            __SelectIfFrequencyIsHigher(&irqCandidate, timer);
        } else if (timer->Attributes & SystemTimeAttributes_COUNTER) {
            // Only counters can be considered HPC
            if (timer->Attributes & SystemTimeAttributes_HPC) {
                GetMachine()->SystemTimers.Hpc = timer;
            }
            __SelectIfFrequencyIsHigher(&counterCandidate, timer);
        }
    }

    // Did we find a counter candidate? Then we will use that as the primary
    // system clock.
    if (counterCandidate != NULL) {
        oserr = __EnableTimer(counterCandidate);
        if (oserr == OS_EOK) {
            GetMachine()->SystemTimers.Clock = counterCandidate;
            return;
        }
        ERROR("__ConfigureTimerSources failed to enable counter candidate (%u), falling back to irq", oserr);
    }

    // Otherwise we have to fall back onto an irq timer, which sucks, but such
    // is life on platforms that suck.
    if (irqCandidate == NULL) {
        WARNING("__ConfigureTimerSources no timer sources present on the system!!");
        return;
    }

    oserr = __EnableTimer(irqCandidate);
    if (oserr != OS_EOK) {
        ERROR("__ConfigureTimerSources failed to enable irq candidate (%u), no counter sources available!!", oserr);
        return;
    }
    GetMachine()->SystemTimers.Clock = irqCandidate;
}

oserr_t SystemSynchronizeTimeSources(void)
{
    SystemWallClock_t* wallClock;
    oserr_t            oserr = OS_EOK;

    // Determine which timer sources to use
    __ConfigureTimerSources();

    // Attempts to synchronize time sources
    // If the underlying RTC supports asynchronous sync, then we utilize that
    // instead of having a thread sleeping which will give us less accurate sync
    // anyway.
    wallClock = GetMachine()->SystemTimers.WallClock;
    if (wallClock && wallClock->Operations.RequestSync) {
        wallClock->Operations.RequestSync(wallClock->Context);
    } else if (wallClock && wallClock->Operations.PerformSync) {
        // Fall back to manual synchronization of the clock.
        uuid_t  timeSyncThreadID;
        oserr = ThreadCreate(
                "time-sync",
                __SynchronizeWallClockAndClocks,
                NULL,
                0,
                UUID_INVALID,
                0,
                0,
                &timeSyncThreadID
        );
    } else {
        WARNING("SystemSynchronizeTimeSources sync was not supported.");
    }
    return oserr;
}

void
SystemTimerHandleSync(void)
{
    SystemTime_t       systemTime;
    SystemWallClock_t* wallClock;
    TRACE("SystemTimerHandleSync()");

    // Two requirements must be satisfied for us to do so. There must
    // be a wall clock registered, and there must be a clock source. Otherwise,
    // there is nothing to do.
    if (GetMachine()->SystemTimers.Clock == NULL) {
        TRACE("SystemTimerHandleSync no clocks to synchronize");
        return;
    }

    wallClock = GetMachine()->SystemTimers.WallClock;

    // Read the system time, and then read the wall-clock
    wallClock->Operations.Read(wallClock->Context, &systemTime);
    GetMachine()->SystemTimers.Clock->Operations.Read(
            GetMachine()->SystemTimers.Clock->Context,
            &wallClock->BaseOffset
    );

    TRACE("SystemTimerHandleSync synced at %i:%i:%i (0x%llx)",
          systemTime.Hour, systemTime.Minute, systemTime.Second,
          wallClock->BaseOffset.QuadPart
    );

    // Convert the system time to a time-point based on the epoch of January 1, 2000
    __LinearTime(&systemTime, &wallClock->BaseTick);
}
