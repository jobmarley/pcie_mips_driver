
#include "driver.h"
#include "queue.tmh"
#include "xdma_registers.h"
#include "Dma.h"

#define H2C_CHANNEL_COUNT 1
#define C2H_CHANNEL_COUNT 1

#define DESCRIPTOR_MAX_LENGTH 0xFFFFFFF


LONG GetH2CEngineIndex(ULONG channelIntRequest)
{
    for (int i = 0; i < H2C_CHANNEL_COUNT; ++i)
        if (channelIntRequest & (1 << i))
            return i;
    return -1;
}
LONG GetC2HEngineIndex(ULONG channelIntRequest)
{
    for (int i = 0; i < C2H_CHANNEL_COUNT; ++i)
        if (channelIntRequest & (1 << (i + H2C_CHANNEL_COUNT)))
            return i;
    return -1;
}
BOOLEAN pciemipsdriverInterruptISR(_In_ WDFINTERRUPT Interrupt, _In_ ULONG MessageID)
{
    UNREFERENCED_PARAMETER(MessageID);
    return WdfInterruptQueueDpcForIsr(Interrupt);
}
VOID pciemipsdriverInterruptDPC(_In_ WDFINTERRUPT Interrupt, _In_ WDFOBJECT AssociatedObject)
{
    UNREFERENCED_PARAMETER(AssociatedObject);

    WDFDEVICE device = WdfInterruptGetDevice(Interrupt);
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(device);

    ULONG channelIntRequest = READ_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_CHANNEL_INT_REQUEST]);
    LONG H2CChannelIndex = GetH2CEngineIndex(channelIntRequest);
    LONG C2HChannelIndex = GetC2HEngineIndex(channelIntRequest);

    ULONG userIntPending = READ_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_USER_INT_PENDING]);
    if (userIntPending & 0x1)
    {
        ULONG mipsStatus = READ_REGISTER_ULONG(&deviceContext->MipsRegisters->Status);
        if (mipsStatus & MIPS_STATUS_BREAK)
        {
            // Clear breakpoint flag
            // This doesn't unpause the processor
            WRITE_REGISTER_ULONG(&deviceContext->MipsRegisters->Status, mipsStatus & ~MIPS_STATUS_BREAK);
            userIntPending = READ_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_USER_INT_PENDING]);

        }
    }
    if (H2CChannelIndex != -1)
    {
        // IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK3 is write 1 to clear
        // Disable interrupt
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK3], 1 << H2CChannelIndex);
        
        ULONG channelStatus = READ_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CChannels[H2CChannelIndex][H2C_CHANNEL_STATUS2]);
        //ULONG channelCompletedDescCount = READ_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CChannels[H2CChannelIndex][H2C_CHANNEL_COMPLETED_DESC_COUNT]);
        
        // Stop DMA engine, is it necessary? why? 
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CChannels[H2CChannelIndex][H2C_CHANNEL_CONTROL3], 1);
        
        // Enable interrupt
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK2], 1 << H2CChannelIndex);

        WDFREQUEST request = WdfDmaTransactionGetRequest(deviceContext->DmaTransaction);
        NTSTATUS requestStatus = STATUS_SUCCESS;
        if (channelStatus & CHANNEL_STATUS_STOPPED)
        {
            NTSTATUS status = STATUS_SUCCESS;
            if (WdfDmaTransactionDmaCompleted(deviceContext->DmaTransaction, &status))
            {
                size_t bytesTransferred = WdfDmaTransactionGetBytesTransferred(deviceContext->DmaTransaction);
                WdfDmaTransactionRelease(deviceContext->DmaTransaction);
                WdfRequestCompleteWithInformation(request, STATUS_SUCCESS, bytesTransferred);
            }
        }
        else
        {
            NTSTATUS status = STATUS_SUCCESS;
            WdfDmaTransactionDmaCompletedFinal(deviceContext->DmaTransaction, 0, &status);
            requestStatus = STATUS_DEVICE_HARDWARE_ERROR;
            size_t bytesTransferred = WdfDmaTransactionGetBytesTransferred(deviceContext->DmaTransaction);
            WdfDmaTransactionRelease(deviceContext->DmaTransaction);
            WdfRequestCompleteWithInformation(request, requestStatus, bytesTransferred);
        }
    }
    else if (C2HChannelIndex != -1)
    {
        // IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK3 is write 1 to clear
        // Disable interrupt
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK3], 1 << (C2HChannelIndex + H2C_CHANNEL_COUNT));

        ULONG channelStatus = READ_REGISTER_ULONG(&deviceContext->XDMARegisters.C2HChannels[C2HChannelIndex][C2H_CHANNEL_STATUS2]);
        //ULONG channelCompletedDescCount = READ_REGISTER_ULONG(&deviceContext->XDMARegisters.C2HChannels[C2HChannelIndex][C2H_CHANNEL_COMPLETED_DESC_COUNT]);
        
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.C2HChannels[C2HChannelIndex][C2H_CHANNEL_CONTROL3], 1);
        
        // Enable interrupt
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK2], 1 << (C2HChannelIndex + H2C_CHANNEL_COUNT));
        
        WDFREQUEST request = WdfDmaTransactionGetRequest(deviceContext->DmaTransaction);
        NTSTATUS requestStatus = STATUS_SUCCESS;
        if (channelStatus & CHANNEL_STATUS_STOPPED)
        {
            NTSTATUS status = STATUS_SUCCESS;
            if (WdfDmaTransactionDmaCompleted(deviceContext->DmaTransaction, &status))
            {
                size_t bytesTransferred = WdfDmaTransactionGetBytesTransferred(deviceContext->DmaTransaction);
                WdfDmaTransactionRelease(deviceContext->DmaTransaction);
                WdfRequestCompleteWithInformation(request, STATUS_SUCCESS, bytesTransferred);
            }
        }
        else
        {
            NTSTATUS status = STATUS_SUCCESS;
            WdfDmaTransactionDmaCompletedFinal(deviceContext->DmaTransaction, 0, &status);
            requestStatus = STATUS_DEVICE_HARDWARE_ERROR;
            size_t bytesTransferred = WdfDmaTransactionGetBytesTransferred(deviceContext->DmaTransaction);
            WdfDmaTransactionRelease(deviceContext->DmaTransaction);
            WdfRequestCompleteWithInformation(request, requestStatus, bytesTransferred);
        }
    }
}

BOOLEAN FillDescriptorBuffer(
    PDEVICE_CONTEXT deviceContext,
    PVOID buffer,
    PHYSICAL_ADDRESS deviceAddress,
    PSCATTER_GATHER_LIST SgList,
    WDF_DMA_DIRECTION Direction)
{
    ULONG maxAdjacent = 0x40;

    ULONG count = SgList->NumberOfElements;
    ULONG adjCount = min(count, maxAdjacent) - 1;

    if (Direction == WdfDmaDirectionWriteToDevice)
    {
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CSGDMA[0][H2C_SGDMA_DESCRIPTOR_ADDRESS_LO], deviceContext->SgBufferPhysicalAddress.LowPart);
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CSGDMA[0][H2C_SGDMA_DESCRIPTOR_ADDRESS_HI], deviceContext->SgBufferPhysicalAddress.HighPart);
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CSGDMA[0][H2C_SGDMA_DESCRIPTOR_ADJACENT], adjCount);
    }
    else
    {
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.C2HSGDMA[0][C2H_SGDMA_DESCRIPTOR_ADDRESS_LO], deviceContext->SgBufferPhysicalAddress.LowPart);
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.C2HSGDMA[0][C2H_SGDMA_DESCRIPTOR_ADDRESS_HI], deviceContext->SgBufferPhysicalAddress.HighPart);
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.C2HSGDMA[0][C2H_SGDMA_DESCRIPTOR_ADJACENT], adjCount);

    }
    PSCATTER_GATHER_DESCRIPTOR pDescriptor = (PSCATTER_GATHER_DESCRIPTOR)buffer;
    
    int iSgElem = 0;
    while (count > 0)
	{
		pDescriptor->Magic = 0xAD4B;
		pDescriptor->NextAdjacent = (UCHAR)adjCount;
		pDescriptor->Length = SgList->Elements[iSgElem].Length;
		if (Direction == WdfDmaDirectionWriteToDevice)
		{
			pDescriptor->SrcAddress = SgList->Elements[iSgElem].Address;
			pDescriptor->DstAddress = deviceAddress;
		}
		else
		{
			pDescriptor->SrcAddress = deviceAddress;
			pDescriptor->DstAddress = SgList->Elements[iSgElem].Address;
		}
        pDescriptor->Control = 0;
		pDescriptor->NextAddress.QuadPart = deviceContext->SgBufferPhysicalAddress.QuadPart + (iSgElem + 1) * sizeof(SCATTER_GATHER_DESCRIPTOR);


        if (count == 1)
        {
            // last descriptor, stop bit
            pDescriptor->Control = 0x13;
            pDescriptor->NextAddress.QuadPart = 0;
        }

        if (adjCount == 0)
            adjCount = min(count, maxAdjacent);

        deviceAddress.QuadPart += pDescriptor->Length;
        ++pDescriptor;
        ++iSgElem;
        --adjCount;
        --count;
    }

    return TRUE;
}
BOOLEAN
pciemipsdriverEvtProgramDMA(
    _In_
    WDFDMATRANSACTION Transaction,
    _In_
    WDFDEVICE Device,
    _In_
    WDFCONTEXT Context,
    _In_
    WDF_DMA_DIRECTION Direction,
    _In_
    PSCATTER_GATHER_LIST SgList
)
{
    UNREFERENCED_PARAMETER(Context);

    PDEVICE_CONTEXT deviceContext = DeviceGetContext(Device);
    WDFREQUEST request = WdfDmaTransactionGetRequest(Transaction);
    WDF_REQUEST_PARAMETERS params;
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(request, &params);

    ULONG maxDescriptorCount = 0x1000 / sizeof(SCATTER_GATHER_DESCRIPTOR);
    if (SgList->NumberOfElements > maxDescriptorCount)
    {
        // Cant handle that many descriptors right now
        NTSTATUS status = STATUS_SUCCESS;
        WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
        WdfDmaTransactionRelease(Transaction);
        WdfRequestComplete(request, STATUS_UNSUCCESSFUL);
        return FALSE;
    }

    if (params.Type == WdfRequestTypeWrite && Direction == WdfDmaDirectionWriteToDevice)
    {
        PHYSICAL_ADDRESS deviceAddress;
        deviceAddress.QuadPart = params.Parameters.Write.DeviceOffset;
        FillDescriptorBuffer(deviceContext, deviceContext->SgBufferVirtualAddress, deviceAddress, SgList, Direction);

        LONG channel = 0;
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CChannels[channel][H2C_CHANNEL_INTERRUPT_ENABLE_MASK1], 0x00FFFFFF);
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK2], 1 << channel);
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CChannels[channel][H2C_CHANNEL_CONTROL2], 0x00FFFE3F);
    }
    else if (params.Type == WdfRequestTypeRead && Direction == WdfDmaDirectionReadFromDevice)
    {
        PHYSICAL_ADDRESS deviceAddress;
        deviceAddress.QuadPart = params.Parameters.Read.DeviceOffset;
        FillDescriptorBuffer(deviceContext, deviceContext->SgBufferVirtualAddress, deviceAddress, SgList, Direction);

        LONG channel = 0;
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.C2HChannels[channel][C2H_CHANNEL_INTERRUPT_ENABLE_MASK1], 0x00FFFFFF);
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK2], 1 << (channel + H2C_CHANNEL_COUNT));
        // Maybe we need the ie_idle_stopped bit to know when the engine is available again?
        // idk maybe ie_descriptor_stopped is enough though
        WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.C2HChannels[channel][C2H_CHANNEL_CONTROL2], 0x00FFFE3F);
    }
    else
    {
        NTSTATUS status = STATUS_SUCCESS;
        WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
        WdfDmaTransactionRelease(Transaction);
        WdfRequestComplete(request, STATUS_UNSUCCESSFUL);
        return FALSE;
    }
#pragma warning(disable:4366)
#pragma warning(default:4366)

    return TRUE;
}