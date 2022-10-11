/**
 * MollenOS
 *
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
 *
 * File Definitions & Structures
 * - This header describes the base file-structures, prototypes
 *   and functionality, refer to the individual things for descriptions
 */

#include <ddk/convert.h>
#include <ds/list.h>
#include <internal/_io.h>
#include <io.h>
#include <os/services/file.h>
#include <os/memory.h>

struct file_view {
    element_t       header;
    DMAAttachment_t dmaAttachment;
    uuid_t          file_handle;
    unsigned int    flags;
    size_t          offset;
    size_t          length;
};

static list_t g_fileViews = LIST_INIT;
static size_t g_pageSize  = 0;

oserr_t
SetFileSizeFromPath(
        _In_ const char* path,
        _In_ size_t      size)
{
    oserr_t status;
    int        fd;
    
    if (!path) {
        return OsInvalidParameters;
    }

    fd = open(path, O_RDWR);
    if (fd == -1) {
        return OsErrToErrNo(fd);
    }

    status = SetFileSizeFromFd(fd, size);

    close(fd);
    return status;
}

oserr_t
SetFileSizeFromFd(
        _In_ int    fileDescriptor,
        _In_ size_t size)
{
    struct vali_link_message msg    = VALI_MSG_INIT_HANDLE(GetFileService());
    stdio_handle_t*          handle = stdio_handle_get(fileDescriptor);
    oserr_t               status;
    UInteger64_t          value;

    if (!handle || handle->object.type != STDIO_HANDLE_FILE) {
        return OsInvalidParameters;
    }

    value.QuadPart = size;
    
    sys_file_set_size(
        GetGrachtClient(), 
        &msg.base, 
        *__crt_processid_ptr(),
        handle->object.handle,
        value.u.LowPart,
        value.u.HighPart
    );
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_set_size_result(GetGrachtClient(), &msg.base, &status);
    return status;
}

oserr_t
ChangeFilePermissionsFromPath(
        _In_ const char*  path,
        _In_ unsigned int permissions)
{
    oserr_t status;
    int        fd;
    
    if (!path) {
        return OsInvalidParameters;
    }

    fd = open(path, O_RDWR);
    if (fd == -1) {
        return OsErrToErrNo(fd);
    }

    status = ChangeFilePermissionsFromFd(fd, permissions);
    
    close(fd);
    return status;
}

oserr_t
ChangeFilePermissionsFromFd(
        _In_ int          fileDescriptor,
        _In_ unsigned int permissions)
{
    struct vali_link_message msg    = VALI_MSG_INIT_HANDLE(GetFileService());
    stdio_handle_t*          handle = stdio_handle_get(fileDescriptor);
    oserr_t                 status;
    unsigned int             access;

    if (!handle || handle->object.type != STDIO_HANDLE_FILE) {
        return OsInvalidParameters;
    }

    sys_file_get_access(
        GetGrachtClient(),
        &msg.base,
        *__crt_processid_ptr(),
        handle->object.handle
    );
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_get_access_result(GetGrachtClient(), &msg.base, &status, &access);
    
    sys_file_set_access(
        GetGrachtClient(),
        &msg.base,
        *__crt_processid_ptr(),
        handle->object.handle,
        access
    );
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_set_access_result(GetGrachtClient(), &msg.base, &status);
    return status;
}

oserr_t
GetFileLink(
        _In_ const char* path,
        _In_ char*       linkPathBuffer,
        _In_ size_t      bufferLength)
{
    struct vali_link_message msg = VALI_MSG_INIT_HANDLE(GetFileService());
    oserr_t               status;
    
    if (!path || !linkPathBuffer || bufferLength == 0) {
        return OsInvalidParameters;
    }

    sys_file_fstat_link(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), path);
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_fstat_link_result(GetGrachtClient(), &msg.base, &status, linkPathBuffer, bufferLength);
    return status;
}

oserr_t
GetFilePathFromFd(
    _In_ int    fileDescriptor,
    _In_ char*  buffer,
    _In_ size_t maxLength)
{
    struct vali_link_message msg    = VALI_MSG_INIT_HANDLE(GetFileService());
    stdio_handle_t*          handle = stdio_handle_get(fileDescriptor);
    oserr_t               status;

    if (!handle || !buffer || handle->object.type != STDIO_HANDLE_FILE) {
        return OsInvalidParameters;
    }
    
    sys_file_get_path(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), handle->object.handle);
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_get_path_result(GetGrachtClient(), &msg.base, &status, buffer, maxLength);
    return status;
}

oserr_t
GetStorageInformationFromPath(
    _In_ const char*            path,
    _In_ int                    followLinks,
    _In_ OsStorageDescriptor_t* descriptor)
{
    struct vali_link_message   msg = VALI_MSG_INIT_HANDLE(GetFileService());
    oserr_t                 status;
    struct sys_disk_descriptor gdescriptor;
    
    if (descriptor == NULL || path == NULL) {
        return OsInvalidParameters;
    }

    sys_file_ststat_path(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), path, followLinks);
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_ststat_path_result(GetGrachtClient(), &msg.base, &status, &gdescriptor);

    if (status == OsOK) {
        from_sys_disk_descriptor(&gdescriptor, descriptor);
    }
    return status;
}

oserr_t
GetStorageInformationFromFd(
    _In_ int                    fileDescriptor,
    _In_ OsStorageDescriptor_t* descriptor)
{
    struct vali_link_message msg    = VALI_MSG_INIT_HANDLE(GetFileService());
    stdio_handle_t*          handle = stdio_handle_get(fileDescriptor);
    oserr_t                  status;
    struct sys_disk_descriptor gdescriptor;

    if (handle == NULL || descriptor == NULL ||
        handle->object.type != STDIO_HANDLE_FILE) {
        return OsInvalidParameters;
    }

    sys_file_ststat(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), handle->object.handle);
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_ststat_result(GetGrachtClient(), &msg.base, &status, &gdescriptor);

    if (status == OsOK) {
        from_sys_disk_descriptor(&gdescriptor, descriptor);
    }
    return status;
}

oserr_t
GetFileSystemInformationFromPath(
    _In_ const char*               path,
    _In_ int                       followLinks,
    _In_ OsFileSystemDescriptor_t* descriptor)
{
    struct vali_link_message         msg = VALI_MSG_INIT_HANDLE(GetFileService());
    oserr_t                          status;
    struct sys_filesystem_descriptor gdescriptor;
    
    if (descriptor == NULL || path == NULL) {
        return OsInvalidParameters;
    }
    
    sys_file_fsstat_path(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), path, followLinks);
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_fsstat_path_result(GetGrachtClient(), &msg.base, &status, &gdescriptor);

    if (status == OsOK) {
        from_sys_filesystem_descriptor(&gdescriptor, descriptor);
    }
    return status;
}

oserr_t
GetFileSystemInformationFromFd(
    _In_ int                       fileDescriptor,
    _In_ OsFileSystemDescriptor_t* descriptor)
{
    struct vali_link_message         msg    = VALI_MSG_INIT_HANDLE(GetFileService());
    stdio_handle_t*                  handle = stdio_handle_get(fileDescriptor);
    oserr_t                       status;
    struct sys_filesystem_descriptor gdescriptor;

    if (handle == NULL || descriptor == NULL ||
        handle->object.type != STDIO_HANDLE_FILE) {
        return OsInvalidParameters;
    }
    
    sys_file_fsstat(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), handle->object.handle);
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_fsstat_result(GetGrachtClient(), &msg.base, &status, &gdescriptor);

    if (status == OsOK) {
        from_sys_filesystem_descriptor(&gdescriptor, descriptor);
    }
    return status;
}

oserr_t
GetFileInformationFromPath(
    _In_ const char*         path,
    _In_ int                 followLinks,
    _In_ OsFileDescriptor_t* descriptor)
{
    struct vali_link_message   msg = VALI_MSG_INIT_HANDLE(GetFileService());
    oserr_t                 status;
    struct sys_file_descriptor gdescriptor;
    
    if (descriptor == NULL || path == NULL) {
        return OsInvalidParameters;
    }
    
    sys_file_fstat_path(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), path, followLinks);
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_fstat_path_result(GetGrachtClient(), &msg.base, &status, &gdescriptor);

    if (status == OsOK) {
        from_sys_file_descriptor(&gdescriptor, descriptor);
    }
    return status;
}

oserr_t
GetFileInformationFromFd(
    _In_ int                 fileDescriptor,
    _In_ OsFileDescriptor_t* descriptor)
{
    struct vali_link_message   msg    = VALI_MSG_INIT_HANDLE(GetFileService());
    stdio_handle_t*            handle = stdio_handle_get(fileDescriptor);
    oserr_t                 status;
    struct sys_file_descriptor gdescriptor;

    if (handle == NULL || descriptor == NULL ||
        handle->object.type != STDIO_HANDLE_FILE) {
        return OsInvalidParameters;
    }
    
    sys_file_fstat(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), handle->object.handle);
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_fstat_result(GetGrachtClient(), &msg.base, &status, &gdescriptor);

    if (status == OsOK) {
        from_sys_file_descriptor(&gdescriptor, descriptor);
    }
    return status;
}

static struct file_view* __GetFileView(uintptr_t virtualBase)
{
    if (!virtualBase) {
        return NULL;
    }

    foreach(element, &g_fileViews) {
        struct file_view* fileView = element->value;
        if (ISINRANGE(virtualBase,
                      (uintptr_t)fileView->dmaAttachment.buffer,
                      (uintptr_t)fileView->dmaAttachment.buffer + fileView->length)) {
            return fileView;
        }
    }
    return NULL;
}

static inline uintptr_t __GetPageSize(void)
{
    if (!g_pageSize) {
        SystemDescriptor_t descriptor;
        SystemQuery(&descriptor);

        g_pageSize = descriptor.PageSizeBytes;
    }
    return g_pageSize;
}

oserr_t
CreateFileMapping(
    _In_  int      FileDescriptor,
    _In_  int      Flags,
    _In_  uint64_t Offset,
    _In_  size_t   Length,
    _Out_ void**   MemoryPointer)
{
    stdio_handle_t*    handle = stdio_handle_get(FileDescriptor);
    oserr_t            osStatus;
    struct file_view*  fileView;
    size_t             fileOffset = Offset & (__GetPageSize() - 1);
    DMABuffer_t        bufferInfo;

    // Sanitize that the descritor is valid
    if (!handle || handle->object.type != STDIO_HANDLE_FILE) {
        return OsInvalidParameters;
    }

    fileView = malloc(sizeof(struct file_view));
    if (!fileView) {
        return OsOutOfMemory;
    }

    // parse flags
    if (Flags & FILE_MAPPING_READ) { }
    if (Flags & FILE_MAPPING_WRITE) { }
    if (Flags & FILE_MAPPING_EXECUTE) { }

    bufferInfo.name = "mmap_file";
    bufferInfo.capacity = Length;
    bufferInfo.length   = 0;
    bufferInfo.flags    = DMA_CLEAN | DMA_TRAP;
    bufferInfo.type     = DMA_TYPE_REGULAR;

    osStatus = DmaCreate(&bufferInfo, &fileView->dmaAttachment);
    if (osStatus != OsOK) {
        free(fileView);
        return osStatus;
    }

    ELEMENT_INIT(&fileView->header, 0, fileView);
    fileView->file_handle = handle->object.handle;
    fileView->offset = fileOffset;
    fileView->length = Length;
    fileView->flags  = Flags;

    list_append(&g_fileViews, &fileView->header);

    *MemoryPointer = fileView->dmaAttachment.buffer;
    return osStatus;
}

oserr_t FlushFileMapping(
        _In_ void*  MemoryPointer,
        _In_ size_t Length)
{
    struct file_view* fileView = __GetFileView((uintptr_t)MemoryPointer);
    UInteger64_t   fileOffset;
    oserr_t        osStatus;
    int               pageCount;
    unsigned int*     attributes;
    if (!fileView) {
        return OsInvalidParameters;
    }

    if (!(fileView->flags & FILE_MAPPING_WRITE)) {
        return OsNotSupported;
    }

    pageCount  = DIVUP(MIN(fileView->length, Length), __GetPageSize());
    attributes = malloc(sizeof(unsigned int) * pageCount);
    if (!attributes) {
        osStatus = OsOutOfMemory;
        goto exit;
    }

    osStatus = MemoryQueryAttributes(fileView->dmaAttachment.buffer, fileView->length, &attributes[0]);
    if (osStatus != OsOK) {
        goto exit;
    }

    fileOffset.QuadPart = fileView->offset;
    for (int i = 0; i < pageCount;) {
        int dirtyPageCount = 0;
        int j              = i;

        // detect sequential dirty pages to batch writes
        while (attributes[j] & MEMORY_DIRTY) {
            dirtyPageCount++;
            j++;
        }

        if (dirtyPageCount) {
            struct vali_link_message msg = VALI_MSG_INIT_HANDLE(GetFileService());
            size_t                   bytesTransferred;

            sys_file_transfer_absolute(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), fileView->file_handle,
                                       1, fileOffset.u.LowPart, fileOffset.u.HighPart, fileView->dmaAttachment.handle,
                                       i * __GetPageSize(), dirtyPageCount * __GetPageSize());
            gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
            sys_file_transfer_absolute_result(GetGrachtClient(), &msg.base, &osStatus, &bytesTransferred);

            // update iterator values and account for the auto inc
            i = j;
            fileOffset.QuadPart += dirtyPageCount * __GetPageSize();
        }
        else { i++; fileOffset.QuadPart += __GetPageSize(); }
    }

exit:
    return osStatus;
}

oserr_t
DestroyFileMapping(
        _In_ void* MemoryPointer)
{
    struct file_view* fileView = __GetFileView((uintptr_t)MemoryPointer);
    oserr_t        osStatus;
    if (!fileView) {
        return OsInvalidParameters;
    }

    // is the mapping write enabled, then flush pages
    if (fileView->flags & FILE_MAPPING_WRITE) {
        (void)FlushFileMapping(MemoryPointer, fileView->length);
    }

    osStatus = DmaAttachmentUnmap(&fileView->dmaAttachment);
    if (osStatus != OsOK) {
        // ignore for now
    }

    osStatus = DmaDetach(&fileView->dmaAttachment);
    if (osStatus != OsOK) {
        // ignore for now
    }

    list_remove(&g_fileViews, &fileView->header);
    free(fileView);
    return osStatus;
}

oserr_t HandleMemoryMappingEvent(
        _In_ int   signal,
        _In_ void* vaddressPtr)
{
    struct vali_link_message msg = VALI_MSG_INIT_HANDLE(GetFileService());
    struct file_view* fileView       = __GetFileView((uintptr_t)vaddressPtr);
    uintptr_t         virtualAddress = (uintptr_t)vaddressPtr;
    UInteger64_t   fileOffset;
    oserr_t        osStatus;
    size_t            bytesTransferred;

    if (!fileView) {
        return OsNotExists;
    }

    // Prepare the memory that we want to fill
    virtualAddress &= (__GetPageSize() - 1);

    fileOffset.QuadPart = fileView->offset + (virtualAddress - (uintptr_t)fileView->dmaAttachment.buffer);
    osStatus            = DmaAttachmentCommit(&fileView->dmaAttachment, (vaddr_t) vaddressPtr, __GetPageSize());
    if (osStatus != OsOK) {
        return OsNotExists;
    }

    // Now we perform the actual filling on the memory area
    sys_file_transfer_absolute(GetGrachtClient(), &msg.base, *__crt_processid_ptr(), fileView->file_handle,
                               0, fileOffset.u.LowPart, fileOffset.u.HighPart, fileView->dmaAttachment.handle,
                               virtualAddress - (uintptr_t)fileView->dmaAttachment.buffer, __GetPageSize());
    gracht_client_wait_message(GetGrachtClient(), &msg.base, GRACHT_MESSAGE_BLOCK);
    sys_file_transfer_absolute_result(GetGrachtClient(), &msg.base, &osStatus, &bytesTransferred);
    return osStatus;
}

// add default event handlers
void sys_file_event_storage_ready_invocation(gracht_client_t* client, const char* path) {
    _CRT_UNUSED(client);
    _CRT_UNUSED(path);
}