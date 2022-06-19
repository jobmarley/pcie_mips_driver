/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "driver.tmh"
#include "xdma_registers.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, pciemipsdriverEvtDeviceAdd)
#pragma alloc_text (PAGE, pciemipsdriverEvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = pciemipsdriverEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config,
                           pciemipsdriverEvtDeviceAdd
                           );

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             WDF_NO_HANDLE
                             );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}


void EnableInterrupts(PDEVICE_CONTEXT deviceContext)
{
    //WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CChannels[0][H2C_CHANNEL_INTERRUPT_ENABLE_MASK1], 0xFFFFFFFF);
    //WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.C2HChannels[0][C2H_CHANNEL_INTERRUPT_ENABLE_MASK1], 0xFFFFFFFF);
    //WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK1], 0xFFFFFFFF);
    /*ULONG id = READ_REGISTER_ULONG(&deviceContext->XDMARegisters.H2CChannels[0][H2C_CHANNEL_IDENTIFIER]);
    ULONG id2 = READ_REGISTER_ULONG(deviceContext->XDMARegisters.H2CChannels[0]);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(id2);*/
    //WRITE_REGISTER_ULONG(&deviceContext->MipsRegisters->Leds, 0xFFFFFFFF);
    WRITE_REGISTER_ULONG(&deviceContext->XDMARegisters.IRQBlock[IRQ_BLOCK_USER_INT_ENABLE_MASK2], 0x1);

}

PVOID GetDMARegisterAddress(PVOID base, ULONG target, ULONG channel)
{
    return (((UCHAR*)base) + ((target << 12) | (channel << 8)));
}
NTSTATUS pciemipsdriverEvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
)
{
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(Device);

    ULONG count = WdfCmResourceListGetCount(ResourcesTranslated);
    UNREFERENCED_PARAMETER(count);
    // Map BARs memory into memory space
    int iBar = 0;
    for (ULONG i = 0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);
        PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptorraw = WdfCmResourceListGetDescriptor(ResourcesRaw, i);
        UNREFERENCED_PARAMETER(descriptorraw);

        if (descriptor->Type == CmResourceTypeMemory)
        {
            if (iBar == 1)
            {
                deviceContext->Bar0Size = descriptor->u.Memory.Length;
                deviceContext->Bar0MappedAddress = MmMapIoSpaceEx(descriptor->u.Memory.Start, descriptor->u.Memory.Length, PAGE_READWRITE | PAGE_NOCACHE);
                deviceContext->MipsRegisters = (PPCIE_MIPS_REGISTERS)deviceContext->Bar0MappedAddress;
            }
            else if (iBar == 0)
            {
                deviceContext->Bar1Size = descriptor->u.Memory.Length;
                deviceContext->Bar1MappedAddress = MmMapIoSpaceEx(descriptor->u.Memory.Start, descriptor->u.Memory.Length, PAGE_READWRITE | PAGE_NOCACHE);
                for (ULONG iChannel = 0; iChannel < 16; ++iChannel)
                {
                    deviceContext->XDMARegisters.H2CChannels[iChannel] = GetDMARegisterAddress(deviceContext->Bar1MappedAddress, 0, iChannel);
                    deviceContext->XDMARegisters.C2HChannels[iChannel] = GetDMARegisterAddress(deviceContext->Bar1MappedAddress, 1, iChannel);
                    deviceContext->XDMARegisters.H2CSGDMA[iChannel] = GetDMARegisterAddress(deviceContext->Bar1MappedAddress, 4, iChannel);
                    deviceContext->XDMARegisters.C2HSGDMA[iChannel] = GetDMARegisterAddress(deviceContext->Bar1MappedAddress, 5, iChannel);

                }
                deviceContext->XDMARegisters.IRQBlock = GetDMARegisterAddress(deviceContext->Bar1MappedAddress, 2, 0);
                deviceContext->XDMARegisters.Config = GetDMARegisterAddress(deviceContext->Bar1MappedAddress, 3, 0);
                deviceContext->XDMARegisters.SGDMACommon = GetDMARegisterAddress(deviceContext->Bar1MappedAddress, 6, 0);
                deviceContext->XDMARegisters.MSIX = GetDMARegisterAddress(deviceContext->Bar1MappedAddress, 8, 0);
            }
            ++iBar;
        }
        else if (descriptor->Type == CmResourceTypeInterrupt)
        {

        }


        if (!descriptor) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfResourceCmGetDescriptor");
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    }

    EnableInterrupts(deviceContext);

    return STATUS_SUCCESS;
}
NTSTATUS pciemipsdriverEvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
)
{
    //UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PDEVICE_CONTEXT deviceContext = DeviceGetContext(Device);
    if (deviceContext->Bar0MappedAddress != NULL)
    {
        MmUnmapIoSpace(deviceContext->Bar0MappedAddress, deviceContext->Bar0Size);
        deviceContext->Bar0MappedAddress = 0;
    }
    if (deviceContext->Bar1MappedAddress != NULL)
    {
        MmUnmapIoSpace(deviceContext->Bar1MappedAddress, deviceContext->Bar1Size);
        deviceContext->Bar1MappedAddress = 0;
    }
    return STATUS_SUCCESS;
}
NTSTATUS
pciemipsdriverEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = pciemipsdriverEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = pciemipsdriverEvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    status = pciemipsdriverCreateDevice(DeviceInit);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

VOID
pciemipsdriverEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}
