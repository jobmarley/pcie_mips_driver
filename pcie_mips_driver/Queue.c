/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"
#include "Dma.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, pciemipsdriverQueueInitialize)
#endif

NTSTATUS
pciemipsdriverQueueInitialize(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchParallel
        );

    queueConfig.EvtIoRead = pciemipsdriverEvtIoRead;
    queueConfig.EvtIoWrite = pciemipsdriverEvtIoWrite;
    queueConfig.EvtIoDeviceControl = pciemipsdriverEvtIoDeviceControl;
    queueConfig.EvtIoStop = pciemipsdriverEvtIoStop;

    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &queue
                 );

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return status;
}


VOID pciemipsdriverEvtIoRead(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION,
        TRACE_QUEUE,
        "%!FUNC! Queue 0x%p, Request 0x%p Length %d",
        Queue, Request, (int)Length);

    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(device);

    NTSTATUS status = WdfDmaTransactionInitializeUsingRequest(deviceContext->DmaTransaction, Request, pciemipsdriverEvtProgramDMA, WdfDmaDirectionReadFromDevice);
    if (status != STATUS_SUCCESS)
    {
        WdfRequestComplete(Request, status);
        return;
    }

    status = WdfDmaTransactionExecute(deviceContext->DmaTransaction, WDF_NO_CONTEXT);
}
VOID pciemipsdriverEvtIoWrite(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION,
        TRACE_QUEUE,
        "%!FUNC! Queue 0x%p, Request 0x%p Length %d",
        Queue, Request, (int)Length);

    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(device);

    NTSTATUS status = WdfDmaTransactionInitializeUsingRequest(deviceContext->DmaTransaction, Request, pciemipsdriverEvtProgramDMA, WdfDmaDirectionWriteToDevice);
    if (status != STATUS_SUCCESS)
    {
        WdfRequestComplete(Request, status);
        return;
    }

    status = WdfDmaTransactionExecute(deviceContext->DmaTransaction, WDF_NO_CONTEXT);
}
#define PCIE_MIPS_IOCTRL_WRITE_REG 0x1
#define PCIE_MIPS_IOCTRL_READ_REG 0x2
typedef struct
{
    ULONGLONG address;
    ULONG value;
} PCIE_MIPS_WRITE_REG_DATA, * PPCIE_MIPS_WRITE_REG_DATA;

VOID
pciemipsdriverEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d", 
                Queue, Request, (int) OutputBufferLength, (int) InputBufferLength, IoControlCode);

    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(device);

    WDF_REQUEST_PARAMETERS params;
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    NTSTATUS status = STATUS_SUCCESS;

    switch (params.Parameters.DeviceIoControl.IoControlCode)
    {
    case PCIE_MIPS_IOCTRL_WRITE_REG:
    {
        PPCIE_MIPS_WRITE_REG_DATA data = NULL;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(PCIE_MIPS_WRITE_REG_DATA), &data, NULL);
        if (!NT_SUCCESS(status))
        {
            WdfRequestComplete(Request, status);
            return;
        }
        // If we actually have a power of 2 registers on the device, the address on the AXI bus would just wrap around if OOB
        // if not I don't really know what happens... the write would probably fail, IDK how the CPU would react to that
        if ((data->address & 0x3)) // lets check for alignment as well
        {
            WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
            return;
        }

        WRITE_REGISTER_ULONG((ULONG*)(((UCHAR*)deviceContext->MipsRegisters) + data->address), data->value);
        WdfRequestComplete(Request, STATUS_SUCCESS);
        break;
    }
    case PCIE_MIPS_IOCTRL_READ_REG:
    {
        PPCIE_MIPS_WRITE_REG_DATA dataIn = NULL;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(PCIE_MIPS_WRITE_REG_DATA), &dataIn, NULL);
        if (!NT_SUCCESS(status))
        {
            WdfRequestComplete(Request, status);
            return;
        }
        PPCIE_MIPS_WRITE_REG_DATA dataOut = NULL;
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(PCIE_MIPS_WRITE_REG_DATA), &dataOut, NULL);
        if (!NT_SUCCESS(status))
        {
            WdfRequestComplete(Request, status);
            return;
        }
        // If we actually have a power of 2 registers on the device, the address on the AXI bus would just wrap around if OOB
        // if not I don't really know what happens... the write would probably fail, IDK how the CPU would react to that
        if ((dataIn->address & 0x3)) // lets check for alignment as well
        {
            WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
            return;
        }

        dataOut->value = READ_REGISTER_ULONG((ULONG*)(((UCHAR*)deviceContext->MipsRegisters) + dataIn->address));
        WdfRequestComplete(Request, STATUS_SUCCESS);
        break;
    }
    default:
        WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
        return;
    }

    return;
}

VOID
pciemipsdriverEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
                Queue, Request, ActionFlags);

    //
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
    //   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
    //   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
    //   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //
    //   Before it can call these methods safely, the driver must make sure that
    //   its implementation of EvtIoStop has exclusive access to the request.
    //
    //   In order to do that, the driver must synchronize access to the request
    //   to prevent other threads from manipulating the request concurrently.
    //   The synchronization method you choose will depend on your driver's design.
    //
    //   For example, if the request is held in a shared context, the EvtIoStop callback
    //   might acquire an internal driver lock, take the request from the shared context,
    //   and then release the lock. At this point, the EvtIoStop callback owns the request
    //   and can safely complete or requeue the request.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge with
    //   a Requeue value of FALSE.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time.
    //
    // In this case, the framework waits until the specified request is complete
    // before moving the device (or system) to a lower power state or removing the device.
    // Potentially, this inaction can prevent a system from entering its hibernation state
    // or another low system power state. In extreme cases, it can cause the system
    // to crash with bugcheck code 9F.
    //

    return;
}
