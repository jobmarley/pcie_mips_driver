/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_pciemipsdriver,
    0x00d3ac92,0xe6de,0x4fa0,0xac,0x79,0xcd,0x07,0x00,0xfb,0xfe,0x12);
// {00d3ac92-e6de-4fa0-ac79-cd0700fbfe12}
