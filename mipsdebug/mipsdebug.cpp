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
    HANDLE hEvent = NULL;
    HANDLE hWaitHandle = NULL;
    OVERLAPPED overlapped = {};
};

struct md_handle
{
    HANDLE hDevice = NULL;
    std::mutex mutex;
    OVERLAPPED overlapped = {}; // This is used for synchronous operations (memory read/write)
    HANDLE hEvent = NULL;       // This is used for synchronous operations (memory read/write)
    md_callback_t callback = nullptr;
    std::vector<std::unique_ptr<breakpoint_callback_info>> callbackInfoList;
};
#define PCIE_MIPS_IOCTRL_WRITE_REG 0x1
#define PCIE_MIPS_IOCTRL_READ_REG 0x2
#define PCIE_MIPS_IOCTRL_WAIT_BREAK 0x3

#define MEMORY_SIZE 0x40000000

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
md_status_e md_open(md_handle_t* device)
{
    if (!device)
        return md_status_invalid_arg;

    *device = nullptr;
    md_status_e status = md_status_success;

    HDEVINFO DeviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_pciemipsdriver, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    SP_DEVINFO_DATA DeviceInfoData;
    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    ULONG DeviceIndex = 0;

    HANDLE hFile = NULL;

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
            hFile = CreateFileW(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
            if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
                status = (GetLastError() == ERROR_ACCESS_DENIED) ? md_status_access_denied : md_status_failure;
            break;
        }
    }

    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL || hEvent == INVALID_HANDLE_VALUE)
        status = md_status_failure;

    if (DeviceInfoSet) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }
    if (hFile != INVALID_HANDLE_VALUE && hEvent != NULL)
    {
        *device = new md_handle;
        (*device)->hDevice = hFile;
        (*device)->hEvent = hEvent;
    }
    else
    {
        if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
        if (hEvent != NULL)
            CloseHandle(hEvent);
    }

    return status;
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
        DeviceIoControl(device->hDevice, PCIE_MIPS_IOCTRL_WAIT_BREAK, NULL, 0, NULL, 0, NULL, &info->overlapped);
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

void cleanup_callback_info(breakpoint_callback_info* info)
{
    auto device = info->device;
    info->device = nullptr;
    {
        std::lock_guard<std::mutex> lock(info->mutex);
        CancelIoEx(device->hDevice, &info->overlapped);
    }
    DWORD bytesTransferred = 0;
    BOOL b = GetOverlappedResult(device->hDevice, &info->overlapped, &bytesTransferred, TRUE);
    if (info->hWaitHandle && info->hWaitHandle != INVALID_HANDLE_VALUE)
        UnregisterWait(info->hWaitHandle);
    if (info->hEvent && info->hEvent != INVALID_HANDLE_VALUE)
        CloseHandle(info->hEvent);
    info->hWaitHandle = NULL;
    info->hEvent = NULL;
}
void destroy_breakpoint_callbacks(md_handle_t device)
{
    for (auto& it : device->callbackInfoList)
        cleanup_callback_info(it.get());
    device->callbackInfoList.clear();
}

bool create_breakpoint_callbacks(md_handle_t device, int count)
{
    bool success = true;
    for (int i = 0; i < count; ++i)
    {
        HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEvent == NULL)
        {
            success = false;
            break;
        }
        std::unique_ptr<breakpoint_callback_info> uinfo = std::make_unique<breakpoint_callback_info>();
        breakpoint_callback_info* info = uinfo.get();
        device->callbackInfoList.push_back(std::move(uinfo));

        info->device = device;
        info->hEvent = hEvent;

        // Register a callback for this event
        HANDLE hWaitHandle = NULL;
        if (!RegisterWaitForSingleObject(&hWaitHandle, hEvent, breakpoint_callback, info, INFINITE, WT_EXECUTEDEFAULT))
        {
            success = false;
            break;
        }

        info->hWaitHandle = hWaitHandle;

        if (!queue_breakpoint_callback(info))
        {
            success = false;
            break;
        }
    }
    if (!success)
        destroy_breakpoint_callbacks(device);
    return success;
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
    if (device == nullptr || buffer == nullptr || count == 0 || offset >= MEMORY_SIZE)
        return md_status_invalid_arg;

    if (readcount != nullptr)
        *readcount = 0;

    // Make sure we dont read out of bounds
    count = min(count, MEMORY_SIZE);
    if (MEMORY_SIZE - count < offset)
        count = MEMORY_SIZE - offset;

    std::lock_guard<std::mutex> lock(device->mutex);
    ZeroMemory(&device->overlapped, sizeof(device->overlapped));
    device->overlapped.Offset = offset;

    if (!ResetEvent(device->hEvent))
        return md_status_failure;

    // There is an error when reading 5 bytes or less... I don't understand why.
    // It returns garbage (00 00 1C..., always?). There is no error on the DMA engine
    // and it doesnt seem related to alignment so I need this workaround
    // This doesnt happen with write. Maybe this is related to driver memory alignment?
    DWORD BytesRead = 0;
    if (count <= 8)
    {
        uint8_t smallBuffer[16] = {};

        uint32_t alignedOffset = min(offset & ~(uint32_t)7, 0x40000000 - 16);
        device->overlapped.Offset = alignedOffset;
        BOOL b = ReadFile(device->hDevice, smallBuffer, 16, NULL, &device->overlapped);
        if (!b && GetLastError() != ERROR_IO_PENDING)
            return md_status_failure;

        if (!GetOverlappedResult(device->hDevice, &device->overlapped, &BytesRead, TRUE))
            return md_status_failure;

        BytesRead = min(BytesRead, count);

        memcpy(buffer, &smallBuffer[offset - alignedOffset], BytesRead);
    }
    else
    {
        BOOL b = ReadFile(device->hDevice, buffer, count, NULL, &device->overlapped);
        if (!b && GetLastError() != ERROR_IO_PENDING)
            return md_status_failure;

        if (!GetOverlappedResult(device->hDevice, &device->overlapped, &BytesRead, TRUE))
            return md_status_failure;
    }

    if (readcount != nullptr)
        *readcount = BytesRead;
    return md_status_success;
}
md_status_e md_write_memory(md_handle_t device, uint8_t* buffer, uint32_t count, uint32_t offset, uint32_t* writtencount)
{
    if (device == nullptr || buffer == nullptr || count == 0 || offset >= MEMORY_SIZE)
        return md_status_invalid_arg;

    if (writtencount != nullptr)
        *writtencount = 0;

    // Make sure we dont write out of bounds
    count = min(count, MEMORY_SIZE);
    if (MEMORY_SIZE - count < offset)
        count = MEMORY_SIZE - offset;

    std::lock_guard<std::mutex> lock(device->mutex);
    ZeroMemory(&device->overlapped, sizeof(device->overlapped));
    device->overlapped.Offset = offset;

    if (!ResetEvent(device->hEvent))
        return md_status_failure;

    DWORD BytesWritten = 0;
    BOOL b = WriteFile(device->hDevice, buffer, count, NULL, &device->overlapped);
    if (!b && GetLastError() != ERROR_IO_PENDING)
        return md_status_failure;

    if (!GetOverlappedResult(device->hDevice, &device->overlapped, &BytesWritten, TRUE))
        return md_status_failure;

    if (writtencount != nullptr)
        *writtencount = BytesWritten;
    return md_status_success;
}

md_status_e md_read_register_raw(md_handle_t device, uint32_t address, uint32_t* value)
{
    if (device == nullptr || value == nullptr)
        return md_status_invalid_arg;

    PCIE_MIPS_WRITE_REG_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.address = address;

    std::lock_guard<std::mutex> lock(device->mutex);
    ZeroMemory(&device->overlapped, sizeof(device->overlapped));

    if (!ResetEvent(device->hEvent))
        return md_status_failure;

    DWORD BytesReturned = 0;
    BOOL b = DeviceIoControl(device->hDevice, PCIE_MIPS_IOCTRL_READ_REG, &data, sizeof(data), &data, sizeof(data), NULL, &device->overlapped);
    if (!b && GetLastError() != ERROR_IO_PENDING)
        return md_status_failure;

    if (!GetOverlappedResult(device->hDevice, &device->overlapped, &BytesReturned, TRUE))
        return md_status_failure;

    *value = data.value;
    return md_status_success;
}
md_status_e md_write_register_raw(md_handle_t device, uint32_t address, uint32_t value)
{
    if (device == nullptr)
        return md_status_invalid_arg;

    PCIE_MIPS_WRITE_REG_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.address = address;
    data.value = value;

    std::lock_guard<std::mutex> lock(device->mutex);
    ZeroMemory(&device->overlapped, sizeof(device->overlapped));

    if (!ResetEvent(device->hEvent))
        return md_status_failure;

    DWORD BytesReturned = 0;
    BOOL b = DeviceIoControl(device->hDevice, PCIE_MIPS_IOCTRL_WRITE_REG, &data, sizeof(data), NULL, 0, NULL, &device->overlapped);
    if (!b && GetLastError() != ERROR_IO_PENDING)
        return md_status_failure;

    if (!GetOverlappedResult(device->hDevice, &device->overlapped, &BytesReturned, TRUE))
        return md_status_failure;

    return md_status_success;
}

md_status_e md_read_register(md_handle_t device, md_register_e r, uint32_t* value)
{
    if (device == nullptr || r < md_register_pc || r > md_register_ra || value == nullptr)
        return md_status_invalid_arg;

    return md_read_register_raw(device, ((uint32_t)r) * 4 + 0x80, value);
}
md_status_e md_write_register(md_handle_t device, md_register_e r, uint32_t value)
{
    if (device == nullptr || r < md_register_pc || r > md_register_ra)
        return md_status_invalid_arg;

    return md_write_register_raw(device, ((uint32_t)r) * 4 + 0x80, value);
}
md_status_e MD_API md_get_state(md_handle_t device, md_state_e* state)
{
    if (device == nullptr || state == nullptr)
        return md_status_invalid_arg;

    uint32_t value = 0;
    md_status_e status = md_read_register_raw(device, 0, &value);
    *state = (md_state_e)value;
    return status;
}
md_status_e MD_API md_set_state(md_handle_t device, md_state_e state)
{
    if (device == nullptr)
        return md_status_invalid_arg;

    return md_write_register_raw(device, 0, (uint32_t)state);
}
void md_close(md_handle_t device)
{
    destroy_breakpoint_callbacks(device);
    if (device->hDevice && device->hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(device->hDevice);
    if (device->hEvent)
        CloseHandle(device->hEvent);
    delete device;
}