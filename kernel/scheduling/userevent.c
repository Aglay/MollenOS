/**
 * Copyright 2020, Philip Meulengracht
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
 * Synchronization (UserEvent)
 * - Userspace events that can emulate some of the linux event descriptors like eventfd, timerfd etc. They also
 *   provide binary semaphore functionality that can provide light synchronization primitives for interrupts
 */

#include <futex.h>
#include <handle.h>
#include <handle_set.h>
#include <heap.h>
#include <ioset.h>
#include <memoryspace.h>
#include <userevent.h>

typedef struct UserEvent {
    unsigned int inital_value;
    unsigned int flags;
    atomic_int*  kernel_mapping;
    atomic_int*  userspace_mapping;
} UserEvent_t;

static MemoryCache_t* syncAddressCache = NULL;

void
UserEventInitialize(void)
{
    syncAddressCache = MemoryCacheCreate(
            "userevent_cache",
            sizeof(atomic_int),
            sizeof(void*),
            0,
            HEAP_SLAB_NO_ATOMIC_CACHE,
            NULL,
            NULL);
}

static void
UserEventDestroy(
    _In_ void* resource)
{
    UserEvent_t* event = resource;
    if (!event) {
        return;
    }

    MemoryCacheFree(syncAddressCache, event->kernel_mapping);
    kfree(event);
}

static oserr_t
AllocateSyncAddress(
    _In_ UserEvent_t* event)
{
    uintptr_t   offsetInPage;
    uintptr_t   dmaAddress;
    oserr_t  status;
    uintptr_t   userAddress;
    void*       kernelAddress = MemoryCacheAllocate(syncAddressCache);
    if (!kernelAddress) {
        return OS_EOOM;
    }

    offsetInPage = (uintptr_t)kernelAddress % GetMemorySpacePageSize();
    status       = GetMemorySpaceMapping(GetCurrentMemorySpace(),
                                         (vaddr_t)kernelAddress, 1, &dmaAddress);
    if (status != OS_EOK) {
        MemoryCacheFree(syncAddressCache, kernelAddress);
        return status;
    }

    status = MemorySpaceMap(
            GetCurrentMemorySpace(),
            (vaddr_t*)&userAddress,
            &dmaAddress,
            GetMemorySpacePageSize(),
            0,
            MAPPING_COMMIT | MAPPING_DOMAIN | MAPPING_USERSPACE | MAPPING_PERSISTENT,
            MAPPING_PHYSICAL_FIXED | MAPPING_VIRTUAL_PROCESS
    );
    if (status != OS_EOK) {
        MemoryCacheFree(syncAddressCache, kernelAddress);
        return status;
    }

    event->kernel_mapping    = (atomic_int*)kernelAddress;
    event->userspace_mapping = (atomic_int*)(userAddress + offsetInPage);
    atomic_store(event->kernel_mapping, 0);
    return OS_EOK;
}

oserr_t
UserEventCreate(
        _In_  unsigned int initialValue,
        _In_  unsigned int flags,
        _Out_ uuid_t*      handleOut,
        _Out_ atomic_int** syncAddressOut)
{
    UserEvent_t* event;
    uuid_t       handle;
    oserr_t   status;

    if (!handleOut || !syncAddressOut) {
        return OS_EINVALPARAMS;
    }

    event = kmalloc(sizeof(UserEvent_t));
    if (!event) {
        return OS_EOOM;
    }

    status = AllocateSyncAddress(event);
    if (status != OS_EOK) {
        kfree(event);
        return status;
    }

    event->inital_value = initialValue;
    event->flags        = flags;

    handle = CreateHandle(HandleTypeUserEvent, UserEventDestroy, event);
    if (handle == UUID_INVALID) {
        MemoryCacheFree(syncAddressCache, event->kernel_mapping);
        kfree(event);
        return OS_EUNKNOWN;
    }

    if (EVT_TYPE(flags) == EVT_TIMEOUT_EVENT) {
        // @todo initate the timeout event
    }

    *handleOut      = handle;
    *syncAddressOut = event->userspace_mapping;
    return OS_EOK;
}

oserr_t
UserEventSignal(
        _In_ uuid_t handle)
{
    UserEvent_t* event  = LookupHandleOfType(handle, HandleTypeUserEvent);
    oserr_t      status = OS_EINCOMPLETE;
    int          currentValue;
    int          i;
    int          result;
    int          value = 1;

    if (!event) {
        return OS_EINVALPARAMS;
    }

    // assert not max
    currentValue = atomic_load(event->kernel_mapping);
    if ((currentValue + value) <= event->inital_value) {
        for (i = 0; i < value; i++) {
            while ((currentValue + 1) <= event->inital_value) {
                result = atomic_compare_exchange_weak(event->kernel_mapping,
                        &currentValue, currentValue + 1);
                if (result) {
                    break;
                }

                if (currentValue >= 0) {
                    FutexWake(event->kernel_mapping, 1, 0);
                }
            }
        }
    }

    MarkHandle(handle, EVT_TYPE(event->flags) == EVT_TIMEOUT_EVENT ? IOSETTIM : IOSETSYN);
    return status;
}
