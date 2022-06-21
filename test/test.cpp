
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>

#include <Windows.h>
#include <setupapi.h>
#include <initguid.h>


DEFINE_GUID(GUID_DEVINTERFACE_pciemipsdriver,
    0x00d3ac92, 0xe6de, 0x4fa0, 0xac, 0x79, 0xcd, 0x07, 0x00, 0xfb, 0xfe, 0x12);
// {00d3ac92-e6de-4fa0-ac79-cd0700fbfe12}


std::wstring GetInterfacePath(HDEVINFO DeviceInfoSet, PSP_DEVICE_INTERFACE_DATA pDeviceInterfaceData)
{
    DWORD requiredSize = 0;
    SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, pDeviceInterfaceData, NULL, 0, &requiredSize, NULL);

    std::vector<char> buffer(requiredSize);
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)buffer.data();
    ZeroMemory(pDeviceInterfaceDetailData, buffer.size());
    pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    if (!SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet, pDeviceInterfaceData, pDeviceInterfaceDetailData, buffer.size(), &requiredSize, NULL))
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
HANDLE CreateDevice(PHANDLE phAsync)
{
    HDEVINFO DeviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_pciemipsdriver, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    SP_DEVINFO_DATA DeviceInfoData;
    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    ULONG DeviceIndex = 0;

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
            HANDLE hFile = CreateFileW(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            // We DO absolutely need 2 handles, otherwise DeviceIoControl fails because overlapped is NULL
            if (phAsync)
            {
                *phAsync = CreateFileW(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
            }
            return hFile;
        }
    }

    if (DeviceInfoSet) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }
    return NULL;
}

void readmem(HANDLE h, int size)
{
    std::vector<BYTE> buffer(size);
    DWORD BytesRead = 0;

    BOOL b = ReadFile(h, buffer.data(), size, &BytesRead, NULL);
    std::cout << "ReadFile 0x" << size
        << " bytes, result: " << (bool)b << ", bytes read: 0x"
        << BytesRead << std::endl;

    for (int i = 0; i < BytesRead;)
    {
        int count = min(i + 16, BytesRead);
        for (; i < count; ++i)
        {
            std::cout << std::setw(2) << (int)buffer[i] << " ";
        }
        std::cout << std::endl;
    }
}
void writemem(HANDLE h, const std::vector<uint8_t>& buffer)
{
    DWORD BytesWritten = 0;

    BOOL b = WriteFile(h, (LPVOID)buffer.data(), buffer.size(), &BytesWritten, NULL);
    std::cout << "WriteFile 0x" << buffer.size()
        << " bytes, result: " << (bool)b << ", bytes written: 0x"
        << BytesWritten << std::endl;
}
#define PCIE_MIPS_IOCTRL_WRITE_REG 0x1
#define PCIE_MIPS_IOCTRL_READ_REG 0x2
#define PCIE_MIPS_IOCTRL_WAIT_BREAK 0x3

struct PCIE_MIPS_WRITE_REG_DATA
{
    uint64_t address;
    uint32_t value;
};
void writereg(HANDLE h, uint64_t address, uint32_t value)
{
    PCIE_MIPS_WRITE_REG_DATA data = { 0 };
    data.address = address;
    data.value = value;
    DWORD BytesReturned = 0;
    BOOL b = DeviceIoControl(h, PCIE_MIPS_IOCTRL_WRITE_REG, &data, sizeof(data), NULL, 0, &BytesReturned, NULL);
    std::cout << "return value: " << (bool)b << std::endl;
}
void readreg(HANDLE h, uint64_t address)
{
    PCIE_MIPS_WRITE_REG_DATA data = { 0 };
    data.address = address;
    data.value = 0;
    DWORD BytesReturned = 0;
    BOOL b = DeviceIoControl(h, PCIE_MIPS_IOCTRL_READ_REG, &data, sizeof(data), &data, sizeof(data), &BytesReturned, NULL);
    std::cout << "return value: " << (bool)b << std::endl;
    std::cout << "read value: " << std::hex << std::setfill('0') << std::setw(8) << data.value << std::endl;
}
std::vector<std::string> GetCommandParams()
{
    std::vector<std::string> cmd;
    std::string s;
    std::getline(std::cin, s);
    std::stringstream ss(s);
    while (!ss.eof())
    {
        std::string s;
        ss >> s;
        if (s.size()) // if input is "   " s is empty
            cmd.push_back(s);
    }
    return cmd;
}

bool ToUInt64(const std::string& s, uint64_t* p)
{
    uint64_t u = 0;
    size_t idx = 0;
    if (s.size() > 2 && s.substr(0, 2) == "0x")
    {
        u = std::stoull(s.substr(2), &idx, 16);
        idx += 2;
    }
    else
        u = std::stoull(s, &idx, 10);

    *p = u;
    return idx == s.size();
}
bool ToUInt8Hex(const std::string& s, uint8_t* p)
{
    if (s.size() != 2)
        return false;

    size_t idx = 0;
    unsigned long i = std::stoul(s, &idx, 16);
    *p = (uint8_t)i;
    return idx == s.size() && s.size() == 2 && i >= 0 && i <= 0xFF;
}
bool ReadDataLine(std::vector<uint8_t>& buffer, int ofs)
{
    std::string parts[16];
    for (int i = 0; i < 16; ++i)
        std::cin >> parts[i];

    for (auto s : parts)
    {
        if (!ToUInt8Hex(s, &buffer[ofs++]))
        {
            std::cout << "Invalid syntax" << std::endl;
            return false;
        }
    }
    return true;
}

struct BreakpointCallbackInfo
{
    std::mutex mutex;
    HANDLE hDevice;
    HANDLE hEvent;
    HANDLE hWaitHandle;
    OVERLAPPED overlapped;
};
std::vector<std::unique_ptr<BreakpointCallbackInfo>> g_callbackInfoList;

void BreakpointCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    BreakpointCallbackInfo* pInfo = (BreakpointCallbackInfo*)lpParameter;
    std::cout << "A breakpoint was triggered" << std::endl;

    std::lock_guard<std::mutex> lock(pInfo->mutex);
	if (pInfo->hDevice != NULL)
	{
		// We requeue the request
		ZeroMemory(&pInfo->overlapped, sizeof(pInfo->overlapped));
		pInfo->overlapped.hEvent = pInfo->hEvent;
		// Dont check for success because... what should we do anyway, log it?
		DeviceIoControl(pInfo->hDevice, PCIE_MIPS_IOCTRL_WAIT_BREAK, NULL, 0, NULL, 0, NULL, &pInfo->overlapped);
	}
}
void CreateBreakpointCallbacks(HANDLE hDevice, int count)
{
    for (int i = 0; i < count; ++i)
    {
        HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEvent == NULL)
        {
            std::cout << "CreateEvent returned NULL" << std::endl;
            throw std::exception();
        }
        std::unique_ptr<BreakpointCallbackInfo> pInfo = std::make_unique<BreakpointCallbackInfo>();
        pInfo->hDevice = hDevice;
        pInfo->hEvent = hEvent;

        // Register a callback for this event
        HANDLE hWaitHandle = NULL;
        if (!RegisterWaitForSingleObject(&hWaitHandle, hEvent, BreakpointCallback, pInfo.get(), INFINITE, WT_EXECUTEDEFAULT))
        {
            std::cout << "RegisterWaitForSingleObject returned 0" << std::endl;
            throw std::exception();
        }

        pInfo->hWaitHandle = hWaitHandle;

        ZeroMemory(&pInfo->overlapped, sizeof(pInfo->overlapped));
        pInfo->overlapped.hEvent = hEvent;
        
        DeviceIoControl(hDevice, PCIE_MIPS_IOCTRL_WAIT_BREAK, NULL, 0, NULL, 0, NULL, &pInfo->overlapped);
		DWORD err = GetLastError();
		if (err != ERROR_IO_PENDING)
		{
			std::cout << "DeviceIoControl failed with error code " << err << std::endl;
			throw std::exception();
		}
        g_callbackInfoList.push_back(std::move(pInfo));
    }
    std::cout << "Created " << count << " breakpoint callbacks" << std::endl;
}
void DestroyBreakpointCallbacks()
{
    for (auto& it : g_callbackInfoList)
	{
		std::lock_guard<std::mutex> lock(it->mutex);
		CancelIoEx(it->hDevice, &it->overlapped);
		UnregisterWait(it->hWaitHandle);
		CloseHandle(it->hEvent);
		it->hDevice = NULL;
		it->hWaitHandle = NULL;
		it->hEvent = NULL;
	}
    // overlapped must not be freed before the operations have completed
}
int main()
{
    HANDLE hAsync = NULL;
    HANDLE h = CreateDevice(&hAsync);
    if (!h || !hAsync)
    {
        std::cout << "CreateDevice failed" << std::endl;
        return 1;
    }
    std::cout << "Created device successfully, handle: 0x" << std::hex << std::setfill('0') << h << ", 0x" << hAsync << std::endl;

    CreateBreakpointCallbacks(hAsync, 2);

    while (true)
    {
        std::cout << "enter command (h for help):" << std::endl;
        auto params = GetCommandParams();
        if (params.size() == 0)
            continue;

        std::string cmd = params[0];
        if (cmd == "h")
        {
            std::cout << "setoffset <offset> (set <offset> as current offset in the device's memory)" << std::endl;
            std::cout << "readmem <size> (read <size> bytes from the device's memory)" << std::endl;
            std::cout << "writemem <size> (write <size> bytes to the device's memory, bytes must be input after. 16 bytes per line, 2 hex per byte)" << std::endl;
            std::cout << "writereg <address> <value> (write the given value to the register at address)" << std::endl;
            std::cout << "readreg <address> (read the register at address)" << std::endl;
            std::cout << "exit (close the application)" << std::endl;
        }
        else if (cmd == "exit")
        {
            break;
        }
        else if (cmd == "setoffset")
        {
            if (params.size() != 2)
            {
                std::cout << "wrong args" << std::endl;
                continue;
            }

            uint64_t ofs = 0;
            if (!ToUInt64(params[1], &ofs))
            {
                std::cout << "unable to read offset" << std::endl;
                continue;
            }

            SetFilePointer(h, ofs, NULL, FILE_BEGIN);
        }
        else if (cmd == "readmem")
        {
            if (params.size() != 2)
            {
                std::cout << "wrong args" << std::endl;
                continue;
            }

            uint64_t size = 0;
            if (!ToUInt64(params[1], &size))
            {
                std::cout << "unable to read size" << std::endl;
                continue;
            }
            if (size == 0)
            {
                std::cout << "size must be bigger than 0" << std::endl;
                continue;
            }

            readmem(h, size);
        }
        else if (cmd == "writemem")
        {
            if (params.size() != 2)
            {
                std::cout << "wrong args" << std::endl;
                continue;
            }

            uint64_t size = 0;
            if (!ToUInt64(params[1], &size))
            {
                std::cout << "unable to read size" << std::endl;
                continue;
            }
            if (size == 0)
            {
                std::cout << "size must be bigger than 0" << std::endl;
                continue;
            }

            std::vector<uint8_t> buffer(size);
            for (int i = 0; i < size / 16; ++i)
            {
                if (!ReadDataLine(buffer, i * 16))
                    continue;
            }
            writemem(h, buffer);
        }
        else if (cmd == "writetest")
        {
            if (params.size() != 3)
            {
                std::cout << "wrong args" << std::endl;
                continue;
            }

            uint64_t offset = 0;
            if (!ToUInt64(params[1], &offset))
            {
                std::cout << "unable to read offset" << std::endl;
                continue;
            }
            uint64_t size = 0;
            if (!ToUInt64(params[2], &size))
            {
                std::cout << "unable to read size" << std::endl;
                continue;
            }

            uint8_t pattern[15] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
            std::vector<uint8_t> buffer(size);
            for (int i = 0; i < buffer.size(); ++i)
                buffer[i] = pattern[i % 15];

            SetFilePointer(h, offset, NULL, FILE_BEGIN);
            writemem(h, buffer);
        }
        else if (cmd == "writereg")
        {
            if (params.size() != 3)
            {
                std::cout << "wrong args" << std::endl;
                continue;
            }

            uint64_t address = 0;
            if (!ToUInt64(params[1], &address))
            {
                std::cout << "wrong address" << std::endl;
                continue;
            }
            uint64_t value = 0;
            if (!ToUInt64(params[2], &value) || value > 0xFFFFFFFF)
            {
                std::cout << "wrong value" << std::endl;
                continue;
            }
            writereg(h, address, value);
        }
        else if (cmd == "readreg")
        {
            if (params.size() != 2)
            {
                std::cout << "wrong args" << std::endl;
                continue;
            }

            uint64_t address = 0;
            if (!ToUInt64(params[1], &address))
            {
                std::cout << "wrong address" << std::endl;
                continue;
            }
            readreg(h, address);
        }
        else if (cmd == "writefile")
        {
            if (params.size() != 2)
            {
                std::cout << "wrong args" << std::endl;
                continue;
            }

            std::string filepath = params[1];
            if (!std::filesystem::is_regular_file(filepath))
            {
                std::cout << "argument must be a valid file" << std::endl;
                continue;
            }
            std::fstream f(filepath);
            std::vector<uint8_t> buffer(std::filesystem::file_size(filepath));
            f.read((char*)buffer.data(), buffer.size());

            writemem(h, buffer);
        }
        else
        {
            std::cout << "unknown command" << std::endl;
        }
    }

    DestroyBreakpointCallbacks();
    CloseHandle(h);
    CloseHandle(hAsync);
    std::cout << "Done." << std::endl;
    return 0;
}