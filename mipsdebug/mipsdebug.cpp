#include "pch.h"
#include "mipsdebug.h"
#include <initguid.h>

DEFINE_GUID(GUID_DEVINTERFACE_pciemipsdriver,
    0x00d3ac92, 0xe6de, 0x4fa0, 0xac, 0x79, 0xcd, 0x07, 0x00, 0xfb, 0xfe, 0x12);
// {00d3ac92-e6de-4fa0-ac79-cd0700fbfe12}


struct breakpoint_callback_info
{
    std::mutex mutex;
    md_handle_t device = nullptr;
    HANDLE hEvent = 0;
    HANDLE hWaitHandle = 0;
    OVERLAPPED overlapped = {};
};

struct md_handle
{
    HANDLE hDevice;
    HANDLE hDeviceAsync;
    md_callback_t callback;
    std::vector<std::unique_ptr<breakpoint_callback_info>> callbackInfoList;
};
#define PCIE_MIPS_IOCTRL_WRITE_REG 0x1
#define PCIE_MIPS_IOCTRL_READ_REG 0x2
#define PCIE_MIPS_IOCTRL_WAIT_BREAK 0x3

struct PCIE_MIPS_WRITE_REG_DATA
{
    uint64_t address;
    uint32_t value;
};

std::wstring GetInterfacePath(HDEVINFO DeviceInfoSet, PSP_DEVICE_INTERFACE_DATA pDeviceInterfaceData)
{
    DWORD requiredSize = 0;
    SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, pDeviceInterfaceData, NULL, 0, &requiredSize, NULL);

    std::vector<char> buffer(requiredSize);
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)buffer.data();
    ZeroMemory(pDeviceInterfaceDetailData, buffer.size());
    pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    if (!SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet, pDeviceInterfaceData, pDeviceInterfaceDetailData, (DWORD)buffer.size(), &requiredSize, NULL))
        return L"";

    return &pDeviceInterfaceDetailData->DevicePath[0];
}
BOOL FindBestInterface(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA pDeviceInfoData, PSP_DEVICE_INTERFACE_DATA pDeviceInterfaceData)
{
    ZeroMemory(pDeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
    pDeviceInterfaceData->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    ULONG InterfaceIndex = 0;

    if (!SetupDiEnumDeviceInterfaces(DeviceInfoSet, pDeviceInfoData, &GUID_DEVINTERFACE_pciemipsdriver, InterfaceIndex, pDeviceInterfaceData))
        return FALSE;

    return TRUE;
}
md_handle_t md_open()
{
    HDEVINFO DeviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_pciemipsdriver, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    SP_DEVINFO_DATA DeviceInfoData;
    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    ULONG DeviceIndex = 0;

    md_handle* handle = nullptr;
    HANDLE hFile = NULL;
    HANDLE hFileAsync = NULL;

    while (SetupDiEnumDeviceInfo(
        DeviceInfoSet,
        DeviceIndex,
        &DeviceInfoData))
    {
        DeviceIndex++;

        SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
        if (FindBestInterface(DeviceInfoSet, &DeviceInfoData, &DeviceInterfaceData))
        {
            std::wstring devicePath = GetInterfacePath(DeviceInfoSet, &DeviceInterfaceData);
            hFile = CreateFileW(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            // We DO absolutely need 2 handles, otherwise DeviceIoControl fails because overlapped is NULL
            hFileAsync = CreateFileW(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
            
            break;
        }
    }

    if (DeviceInfoSet) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }
    if (hFile != INVALID_HANDLE_VALUE && hFileAsync != INVALID_HANDLE_VALUE)
    {
        handle = new md_handle;
        ZeroMemory(handle, sizeof(md_handle));
        handle->hDevice = hFile;
        handle->hDeviceAsync = hFileAsync;
    }
    else
    {
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
        if (hFileAsync != INVALID_HANDLE_VALUE)
            CloseHandle(hFileAsync);
    }

    return handle;
}


bool queue_breakpoint_callback(breakpoint_callback_info* info)
{
    std::lock_guard<std::mutex> lock(info->mutex);
    md_handle_t device = info->device;
    if (device != NULL)
    {
        // We requeue the request
        ZeroMemory(&info->overlapped, sizeof(info->overlapped));
        info->overlapped.hEvent = info->hEvent;
        DeviceIoControl(device->hDeviceAsync, PCIE_MIPS_IOCTRL_WAIT_BREAK, NULL, 0, NULL, 0, NULL, &info->overlapped);
        DWORD err = GetLastError();
        return err == ERROR_IO_PENDING;
    }
    return false;
}
void breakpoint_callback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    breakpoint_callback_info* info = (breakpoint_callback_info*)lpParameter;

    md_handle_t device = info->device;
    if (!device)
        return;

    md_callback_t callback = device->callback;
    if (callback != nullptr)
        callback(md_event_breakpoint);

    queue_breakpoint_callback(info);
}
bool create_breakpoint_callbacks(md_handle_t device, int count)
{
    for (int i = 0; i < count; ++i)
    {
        HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEvent == NULL)
        {
            return false;
        }
        std::unique_ptr<breakpoint_callback_info> info = std::make_unique<breakpoint_callback_info>();
        info->device = device;
        info->hEvent = hEvent;

        // Register a callback for this event
        HANDLE hWaitHandle = NULL;
        if (!RegisterWaitForSingleObject(&hWaitHandle, hEvent, breakpoint_callback, info.get(), INFINITE, WT_EXECUTEDEFAULT))
        {
            CloseHandle(hEvent);
            return false;
        }

        info->hWaitHandle = hWaitHandle;

        if (!queue_breakpoint_callback(info.get()))
        {
            UnregisterWait(hWaitHandle);
            CloseHandle(hEvent);
            return false;
        }
        device->callbackInfoList.push_back(std::move(info));
    }
    return true;
}
void destroy_breakpoint_callbacks(md_handle_t device)
{
    for (auto& it : device->callbackInfoList)
    {
        it->device = nullptr;
        {
            std::lock_guard<std::mutex> lock(it->mutex);
            CancelIoEx(device->hDeviceAsync, &it->overlapped);
        }
        DWORD bytesTransferred = 0;
        BOOL b = GetOverlappedResult(device->hDeviceAsync, &it->overlapped, &bytesTransferred, TRUE);
        UnregisterWait(it->hWaitHandle);
        CloseHandle(it->hEvent);
        it->device = nullptr;
        it->hWaitHandle = NULL;
        it->hEvent = NULL;
    }
    device->callbackInfoList.clear();
}

md_status_e md_register_callback(md_handle_t device, md_callback_t c)
{
    if (device == nullptr || c == nullptr)
        return md_status_invalid_arg;

    if (device->callback != nullptr)
        return md_status_failure;

    device->callback = c;
    create_breakpoint_callbacks(device, 2);

    return md_status_success;
}
md_status_e md_unregister_callback(md_handle_t device, md_callback_t c)
{
    if (device == nullptr || c == nullptr)
        return md_status_invalid_arg;

    if (device->callback != c)
        return md_status_failure;

    device->callback = nullptr;
    destroy_breakpoint_callbacks(device);

    return md_status_success;
}
md_status_e md_read_memory(md_handle_t device, uint8_t* buffer, uint32_t count, uint32_t offset, uint32_t* readcount)
{
    if (device == nullptr || buffer == nullptr || count == 0)
        return md_status_invalid_arg;

    SetFilePointer(device->hDevice, offset, NULL, FILE_BEGIN);
    DWORD BytesRead = 0;
    BOOL b = ReadFile(device->hDevice, buffer, count, &BytesRead, NULL);

    if (readcount != nullptr)
        *readcount = BytesRead;
    return b ? md_status_success : md_status_failure;
}
md_status_e md_write_memory(md_handle_t device, uint8_t* buffer, uint32_t count, uint32_t offset, uint32_t* writtencount)
{
    if (device == nullptr || buffer == nullptr || count == 0)
        return md_status_invalid_arg;

    SetFilePointer(device->hDevice, offset, NULL, FILE_BEGIN);
    DWORD BytesWritten = 0;
    BOOL b = WriteFile(device->hDevice, buffer, count, &BytesWritten, NULL);

    if (writtencount != nullptr)
        *writtencount = BytesWritten;
    return b ? md_status_success : md_status_failure;
}
md_status_e md_read_register(md_handle_t device, md_register_e r, uint32_t* value)
{
    if (device == nullptr || r < md_register_pc || r > md_register_ra)
        return md_status_invalid_arg;

    PCIE_MIPS_WRITE_REG_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.address = 0x80 + r;
    DWORD BytesReturned = 0;
    BOOL b = DeviceIoControl(device->hDevice, PCIE_MIPS_IOCTRL_READ_REG, &data, sizeof(data), &data, sizeof(data), &BytesReturned, NULL);
    *value = data.value;
    return b ? md_status_success : md_status_failure;
}
md_status_e md_write_register(md_handle_t device, md_register_e r, uint32_t value)
{
    if (device == nullptr || r < md_register_pc || r > md_register_ra)
        return md_status_invalid_arg;

    PCIE_MIPS_WRITE_REG_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.address = 0x80 + r;
    data.value = value;
    DWORD BytesReturned = 0;
    BOOL b = DeviceIoControl(device->hDevice, PCIE_MIPS_IOCTRL_WRITE_REG, &data, sizeof(data), NULL, 0, &BytesReturned, NULL);
    return b ? md_status_success : md_status_failure;
}
md_status_e MD_API md_get_state(md_handle_t device, md_state_e* state)
{
    if (device == nullptr || state == nullptr)
        return md_status_invalid_arg;

    PCIE_MIPS_WRITE_REG_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.address = 0;
    DWORD BytesReturned = 0;
    BOOL b = DeviceIoControl(device->hDevice, PCIE_MIPS_IOCTRL_READ_REG, &data, sizeof(data), &data, sizeof(data), &BytesReturned, NULL);
    *state = (md_state_e)data.value;
    return b ? md_status_success : md_status_failure;
}
md_status_e MD_API md_set_state(md_handle_t device, md_state_e state)
{
    if (device == nullptr)
        return md_status_invalid_arg;

    PCIE_MIPS_WRITE_REG_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.address = 0;
    data.value = state;
    DWORD BytesReturned = 0;
    BOOL b = DeviceIoControl(device->hDevice, PCIE_MIPS_IOCTRL_WRITE_REG, &data, sizeof(data), NULL, 0, &BytesReturned, NULL);
    return b ? md_status_success : md_status_failure;
}
void md_close(md_handle_t device)
{
    destroy_breakpoint_callbacks(device);
    delete device;
}