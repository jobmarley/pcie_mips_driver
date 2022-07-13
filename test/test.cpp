/*
 Copyright (C) 2022 jobmarley

 This file is part of pcie_mips_driver.

 This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.
 */


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
#include "../mipsdebug/mipsdebug.h"

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
bool ReadDataLine(std::vector<uint8_t>& buffer, int ofs, int count)
{
    std::vector<std::string> parts(count);
    for (int i = 0; i < parts.size(); ++i)
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

void readmem(md_handle_t device, uint32_t size, uint32_t offset)
{
    std::vector<BYTE> buffer(size);
    uint32_t BytesRead = 0;

    md_status_e status = md_read_memory(device, buffer.data(), size, offset, &BytesRead);
    std::cout << "md_read_memory 0x" << size
        << " bytes, result: 0x" << std::hex << std::setw(8) << status << ", bytes read: 0x"
        << std::hex << std::setw(8) << BytesRead << std::endl;

    for (uint32_t i = 0; i < BytesRead;)
    {
        uint32_t count = min(i + 16, BytesRead);
        for (; i < count; ++i)
        {
            std::cout << std::setw(2) << (int)buffer[i] << " ";
        }
        std::cout << std::endl;
    }
}
void writemem(md_handle_t device, const std::vector<uint8_t>& buffer, uint32_t offset)
{
    uint32_t BytesWritten = 0;

    md_status_e status = md_write_memory(device, (uint8_t*)buffer.data(), (uint32_t)buffer.size(), offset, &BytesWritten);
    std::cout << "md_write_memory 0x" << buffer.size()
        << " bytes, result: 0x" << std::hex << std::setw(8) << status << ", bytes written: 0x"
        << std::hex << std::setw(8) << BytesWritten << std::endl;
}
void BreakpointCallback(md_event_e e)
{
    std::cout << "A breakpoint was triggered" << std::endl;
}
int main()
{
    md_handle_t device = nullptr;
    md_status_e status = md_open(&device);
    if (status != md_status_success)
    {
        std::cout << "md_open returned 0x" << std::hex << std::setfill('0') << status << std::endl;
        return 1;
    }
    std::cout << "md_open completed successfully, handle: 0x" << std::hex << std::setfill('0') << device << std::endl;

    status = md_register_callback(device, BreakpointCallback);
    if (status != md_status_success)
    {
        std::cout << "md_register_callback returned 0x" << std::hex << std::setfill('0') << status << std::endl;
        return 1;
    }

    uint64_t offset = 0;

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
            std::cout << "readreg <address> (read the register at address)" << std::endl; std::cout << "writereg <address> <value> (write the given value to the register at address)" << std::endl;
            std::cout << "setstate <value> (write the given value to the state of the processor)" << std::endl;
            std::cout << "getstate (read the state of the processor)" << std::endl;
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

            offset = ofs;
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

            readmem(device, (uint32_t)size, (uint32_t)offset);
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
            if (!ReadDataLine(buffer, 0, size))
                continue;

            writemem(device, buffer, (uint32_t)offset);
        }
        else if (cmd == "writetest")
        {
            if (params.size() != 3)
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

            writemem(device, buffer, (uint32_t)ofs);
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
            md_status_e status = md_write_register(device, (md_register_e)address, (uint32_t)value);
            std::cout << "result : 0x" << std::hex << std::setw(8) << status << std::endl;
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
            uint32_t value = 0;
            md_status_e status = md_read_register(device, (md_register_e)address, &value);
            std::cout << "result : 0x" << std::hex << std::setw(8) << status << ", value : 0x" << std::hex << std::setw(8) << value << std::endl;
        }
        else if (cmd == "setstate")
        {
            if (params.size() != 2)
            {
                std::cout << "wrong args" << std::endl;
                continue;
            }

            uint64_t value = 0;
            if (!ToUInt64(params[1], &value) || value > 0xFFFFFFFF)
            {
                std::cout << "wrong value" << std::endl;
                continue;
            }
            md_status_e status = md_set_state(device, (md_state_e)value);
            std::cout << "result : 0x" << std::hex << std::setw(8) << status << std::endl;
        }
        else if (cmd == "getstate")
        {
            if (params.size() != 1)
            {
                std::cout << "wrong args" << std::endl;
                continue;
            }

            md_state_e state;
            md_status_e status = md_get_state(device, (md_state_e*)&state);
            std::cout << "result : 0x" << std::hex << std::setw(8) << status << ", value : 0x" << std::hex << std::setw(8) << state << std::endl;
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

            writemem(device, buffer, (uint32_t)offset);
        }
        else
        {
            std::cout << "unknown command" << std::endl;
        }
    }

    std::cout << "Closing..." << std::endl;
    md_close(device);
    std::cout << "Done." << std::endl;
    return 0;
}