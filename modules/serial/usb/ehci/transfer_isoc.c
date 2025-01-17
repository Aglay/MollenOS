/**
 * MollenOS
 *
 * Copyright 2018, Philip Meulengracht
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
 * Enhanced Host Controller Interface Driver
 * TODO:
 * - Power Management
 * - Transaction Translator Support
 */
#define __TRACE
#define __need_minmax
#include <ddk/utils.h>
#include "ehci.h"
#include <stdlib.h>

UsbTransferStatus_t
HciQueueTransferIsochronous(
    _In_ UsbManagerTransfer_t* transfer)
{
    EhciIsochronousDescriptor_t* firstTd    = NULL;
    EhciIsochronousDescriptor_t* previousTd = NULL;
    EhciController_t*            controller;
    size_t                       bytesToTransfer;
    size_t                       maxBytesPerDescriptor;
    int                          i;

    controller = (EhciController_t *)UsbManagerGetController(transfer->DeviceId);
    if (!controller) {
        return TransferInvalid;
    }

    bytesToTransfer = transfer->Transfer.Transactions[0].Length;

    // Calculate mpd
    maxBytesPerDescriptor = 1024 * MAX(3, transfer->Transfer.PeriodicBandwith);
    maxBytesPerDescriptor *= 8;

    // Allocate resources
    while (bytesToTransfer) {
        EhciIsochronousDescriptor_t* iTd;
        uintptr_t                    AddressPointer;
        size_t                       BytesStep;
        
        // Out of three different limiters we must select the lowest one. Either
        // we must transfer lower bytes because of the requested amount, or the limit
        // of a descriptor, or the limit of the DMA table
        BytesStep = MIN(bytesToTransfer, maxBytesPerDescriptor);
        BytesStep = MIN(BytesStep, transfer->Transactions[0].DmaTable.entries[
            transfer->Transactions[0].SgIndex].length - transfer->Transactions[0].SgOffset);
        
        AddressPointer = transfer->Transactions[0].DmaTable.entries[
            transfer->Transactions[0].SgIndex].address + transfer->Transactions[0].SgOffset;
        
        if (UsbSchedulerAllocateElement(controller->Base.Scheduler, EHCI_iTD_POOL, (uint8_t**)&iTd) == OS_EOK) {
            if (EhciTdIsochronous(controller, &transfer->Transfer, iTd,
                                  AddressPointer, BytesStep, transfer->Transfer.Transactions[0].Type,
                                  transfer->Transfer.Address.DeviceAddress,
                                  transfer->Transfer.Address.EndpointAddress) != OS_EOK) {
                // TODO: Out of bandwidth
                TRACE(" > Out of bandwidth");
                for(;;);
            }
        }

        if (iTd == NULL) {
            TRACE(" > Failed to allocate descriptor");
            for(;;);
            break;
        }

        // Update pointers
        if (firstTd == NULL) {
            firstTd    = iTd;
            previousTd = iTd;
        }
        else {
            UsbSchedulerChainElement(controller->Base.Scheduler, EHCI_iTD_POOL,
                                     (uint8_t*)firstTd, EHCI_iTD_POOL, (uint8_t*)iTd, USB_ELEMENT_NO_INDEX, USB_CHAIN_DEPTH);
            
            for (i = 0; i < 8; i++) {
                if (previousTd->Transactions[i] & EHCI_iTD_IOC) {
                    previousTd->Transactions[i] &= ~(EHCI_iTD_IOC);
                    previousTd->TransactionsCopy[i] &= ~(EHCI_iTD_IOC);
                }
            }

            previousTd = iTd;
        }
        
        // Increase the DmaTable metrics
        transfer->Transactions[0].SgOffset += BytesStep;
        if (transfer->Transactions[0].SgOffset ==
            transfer->Transactions[0].DmaTable.entries[
                    transfer->Transactions[0].SgIndex].length) {
            transfer->Transactions[0].SgIndex++;
            transfer->Transactions[0].SgOffset = 0;
        }
        bytesToTransfer -= BytesStep;
    }

    transfer->EndpointDescriptor = (void*)firstTd;

    EhciTransactionDispatch(controller, transfer);
    return TransferInProgress;
}
