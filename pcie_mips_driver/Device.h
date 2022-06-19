/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

EXTERN_C_START

#pragma pack(push, 1)
typedef struct
{
    PULONG H2CChannels[16];
    PULONG C2HChannels[16];
    PULONG IRQBlock;
    PULONG Config;
    PULONG H2CSGDMA[16];
    PULONG C2HSGDMA[16];
    PULONG SGDMACommon;
    PULONG MSIX;
} XDMA_REGISTERS;
typedef struct
{
    ULONG Status;
    ULONG Leds;
} PCIE_MIPS_REGISTERS, * PPCIE_MIPS_REGISTERS;
#pragma pack(pop)

#define MIPS_STATUS_ENABLE 0x1
#define MIPS_STATUS_BREAK 0x2

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    ULONG Bar0Size;
    PVOID Bar0MappedAddress;
    ULONG Bar1Size;
    PVOID Bar1MappedAddress;
    WDFCOMMONBUFFER SgCommonBuffer;
    PHYSICAL_ADDRESS SgBufferPhysicalAddress;
    PVOID SgBufferVirtualAddress;
    WDFDMAENABLER DmaEnabler;
    WDFDMATRANSACTION DmaTransaction;
    WDFINTERRUPT Interrupt;
    XDMA_REGISTERS XDMARegisters;
    PPCIE_MIPS_REGISTERS MipsRegisters;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

#pragma pack(push, 1)
typedef struct 
{
    UCHAR Control;
    UCHAR NextAdjacent;         // 2 MSB are reserved
    USHORT Magic;
    ULONG Length;
    PHYSICAL_ADDRESS SrcAddress;
    PHYSICAL_ADDRESS DstAddress;
    PHYSICAL_ADDRESS NextAddress;
} SCATTER_GATHER_DESCRIPTOR, *PSCATTER_GATHER_DESCRIPTOR;
#pragma pack(pop)

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
pciemipsdriverCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

EXTERN_C_END
