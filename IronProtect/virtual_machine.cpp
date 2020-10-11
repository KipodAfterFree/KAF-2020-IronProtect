#include "virtual_machine.h"
#include "resource.h"
#include "process.h"
#include "anti_debugging.h"

#include <windows.h>
#include <algorithm>
#include <random>
#include <iostream>

static constexpr int IRONSON_RESOURCE_ID = 101;
static constexpr int SONCODE_RESOURCE_ID = 102;

static const std::string SON_PATH = "ironson.exe";

static constexpr size_t KEY_LENGTH = 8;

static constexpr size_t INSTRUCTIONS_AMOUNT = 0x100;

static std::vector<std::vector<byte>> VMCALL_STUBS = {
    { 0xB0, 0xFF, 0xF3, 0x0F, 0xC7, 0x34, 0x25, 0x37, 0x13, 0x00, 0x00, 0xC3 },
    { 0xB0, 0xFF, 0x0F, 0x01, 0xC4, 0xC3 },
    { 0xB0, 0xFF, 0x66, 0x0F, 0xC7, 0x34, 0x25, 0x37, 0x13, 0x00, 0x00, 0xC3 },
    { 0xB0, 0xFF, 0x0F, 0xC7, 0x34, 0x25, 0x37, 0x13, 0x00, 0x00, 0xC3 },
    { 0xB0, 0xFF, 0x0F, 0x01, 0xC2, 0xC3 },
    { 0xB0, 0xFF, 0x0F, 0x01, 0xC3, 0xC3 }
};

static const std::vector<byte> STACK_PIVOT_STUB = {
    0x48, 0xBC, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xC3
};

static constexpr size_t VMCALL_AL_OFFSET = 1;
static constexpr size_t STACK_PIVOT_RSP_OFFSET = 2;

static constexpr size_t FIXED_CODE_OFFSET = 2;
static constexpr size_t FIXED_CODE_SIZE = 4;

static std::string _flag;

void (*INSTRUCTION_HANDLERS[INSTRUCTIONS_AMOUNT])(CONTEXT& context) = {
    [](CONTEXT& context) -> void { context.Rdx = 0x00; },
    [](CONTEXT& context) -> void { context.Rdx = 0x01; },
    [](CONTEXT& context) -> void { context.Rdx = 0x02; },
    [](CONTEXT& context) -> void { context.Rdx = 0x03; },
    [](CONTEXT& context) -> void { context.Rdx = 0x04; },
    [](CONTEXT& context) -> void { context.Rdx = 0x05; },
    [](CONTEXT& context) -> void { context.Rdx = 0x06; },
    [](CONTEXT& context) -> void { context.Rdx = 0x07; },
    [](CONTEXT& context) -> void { context.Rdx = 0x08; },
    [](CONTEXT& context) -> void { context.Rdx = 0x09; },
    [](CONTEXT& context) -> void { context.Rdx = 0x0A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x0B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x0C; },
    [](CONTEXT& context) -> void { context.Rcx = context.Rdx; },
    [](CONTEXT& context) -> void { context.Rdx = 0x0E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x0F; },
    [](CONTEXT& context) -> void { context.Rdx = 0x10; },
    [](CONTEXT& context) -> void { context.Rdx = 0x11; },
    [](CONTEXT& context) -> void { context.Rdx = 0x12; },
    [](CONTEXT& context) -> void { context.Rbx = 0; context.Rcx = 0; context.Rdx = 0; },
    [](CONTEXT& context) -> void { context.Rdx = 0x14; },
    [](CONTEXT& context) -> void { context.Rdx = 0x15; },
    [](CONTEXT& context) -> void { context.Rdx = 0x16; },
    [](CONTEXT& context) -> void { context.Rdx = 0x17; },
    [](CONTEXT& context) -> void { context.Rdx = 0x18; },
    [](CONTEXT& context) -> void { context.Rdx = 0x19; },
    [](CONTEXT& context) -> void { context.Rdx = 0x1A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x1B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x1C; },
    [](CONTEXT& context) -> void { context.Rdx = 0x1D; },
    [](CONTEXT& context) -> void { context.Rdx = 0x1E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x1F; },
    [](CONTEXT& context) -> void { context.Rdx = 0x20; },
    [](CONTEXT& context) -> void { context.Rdx = 0x21; },
    [](CONTEXT& context) -> void { context.Rdx = 0x22; },
    [](CONTEXT& context) -> void { context.Rdx = 0x23; },
    [](CONTEXT& context) -> void { context.Rdx = 0x24; },
    [](CONTEXT& context) -> void { context.Rdx = 0x25; },
    [](CONTEXT& context) -> void { context.Rdx = 0x26; },
    [](CONTEXT& context) -> void { context.Rdx = 0x27; },
    [](CONTEXT& context) -> void { context.Rdx = 0x28; },
    [](CONTEXT& context) -> void { context.Rdx = 0x29; },
    [](CONTEXT& context) -> void { context.Rdx = 0x2A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x2B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x2C; },
    [](CONTEXT& context) -> void { context.Rdx = 0x2D; },
    [](CONTEXT& context) -> void { context.Rdx = 0x2E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x2F; },
    [](CONTEXT& context) -> void { context.Rdx = 0x30; },
    [](CONTEXT& context) -> void { context.Rdx = 0x31; },
    [](CONTEXT& context) -> void { context.Rdx = 0x32; },
    [](CONTEXT& context) -> void { context.Rdx = 0x33; },
    [](CONTEXT& context) -> void { context.Rdx = 0x34; },
    [](CONTEXT& context) -> void { context.Rdx = 0x35; },
    [](CONTEXT& context) -> void { context.Rdx = 0x36; },
    [](CONTEXT& context) -> void { context.Rcx = static_cast<DWORD64>(_flag[context.Rbx & 0xFF]); },
    [](CONTEXT& context) -> void { context.Rdx = 0x38; },
    [](CONTEXT& context) -> void { context.Rdx = 0x39; },
    [](CONTEXT& context) -> void { context.Rdx = 0x3A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x3B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x3C; },
    [](CONTEXT& context) -> void { context.Rdx = 0x3D; },
    [](CONTEXT& context) -> void { context.Rdx = 0x3E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x3F; },
    [](CONTEXT& context) -> void { context.Rdx = 0x40; },
    [](CONTEXT& context) -> void { context.Rdx = 0x41; },
    [](CONTEXT& context) -> void { context.Rdx = 0x42; },
    [](CONTEXT& context) -> void { context.Rdx = 0x43; },
    [](CONTEXT& context) -> void { context.Rdx = 0x44; },
    [](CONTEXT& context) -> void { context.Rdx = 0x45; },
    [](CONTEXT& context) -> void { context.Rdx = 0x46; },
    [](CONTEXT& context) -> void { context.Rdx = 0x47; },
    [](CONTEXT& context) -> void { context.Rdx = 0x48; },
    [](CONTEXT& context) -> void { context.Rbx--; },
    [](CONTEXT& context) -> void { context.Rdx = 0x4A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x4B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x4C; },
    [](CONTEXT& context) -> void { context.Rdx = 0x4D; },
    [](CONTEXT& context) -> void { context.Rdx = 0x4E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x4F; },
    [](CONTEXT& context) -> void { context.Rdx = 0x50; },
    [](CONTEXT& context) -> void { context.Rdx = 0x51; },
    [](CONTEXT& context) -> void { context.Rdx = 0x52; },
    [](CONTEXT& context) -> void { context.Rdx = 0x53; },
    [](CONTEXT& context) -> void { context.Rdx = 0x54; },
    [](CONTEXT& context) -> void { context.Rdx = 0x55; },
    [](CONTEXT& context) -> void { context.Rdx = 0x56; },
    [](CONTEXT& context) -> void { context.Rdx = 0x57; },
    [](CONTEXT& context) -> void { context.Rdx = 0x58; },
    [](CONTEXT& context) -> void { context.Rdx = 0x59; },
    [](CONTEXT& context) -> void { context.Rdx = 0x5A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x5B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x5C; },
    [](CONTEXT& context) -> void { context.Rdx = 0x5D; },
    [](CONTEXT& context) -> void { context.Rdx = 0x5E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x5F; },
    [](CONTEXT& context) -> void { context.Rdx = 0x60; },
    [](CONTEXT& context) -> void { context.Rdx = 0x61; },
    [](CONTEXT& context) -> void { context.Rdx = 0x62; },
    [](CONTEXT& context) -> void { context.Rdx = 0x63; },
    [](CONTEXT& context) -> void { context.Rdx = 0x64; },
    [](CONTEXT& context) -> void { context.Rdx = 0x65; },
    [](CONTEXT& context) -> void { context.Rdx = 0x66; },
    [](CONTEXT& context) -> void { context.Rdx = 0x67; },
    [](CONTEXT& context) -> void { context.Rdx = 0x68; },
    [](CONTEXT& context) -> void { context.Rbx++; },
    [](CONTEXT& context) -> void { context.Rdx = 0x6A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x6B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x6C; },
    [](CONTEXT& context) -> void { context.Rdx = 0x6D; },
    [](CONTEXT& context) -> void { context.Rdx = 0x6E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x6F; },
    [](CONTEXT& context) -> void { context.Rdx = 0x70; },
    [](CONTEXT& context) -> void { context.Rdx = 0x71; },
    [](CONTEXT& context) -> void { context.Rdx = 0x72; },
    [](CONTEXT& context) -> void { context.Rdx = 0x73; },
    [](CONTEXT& context) -> void { context.Rdx = 0x74; },
    [](CONTEXT& context) -> void { context.Rdx = 0x75; },
    [](CONTEXT& context) -> void { context.Rdx = 0x76; },
    [](CONTEXT& context) -> void { context.Rdx = 0x77; },
    [](CONTEXT& context) -> void { context.Rdx = 0x78; },
    [](CONTEXT& context) -> void { context.Rdx = 0x79; },
    [](CONTEXT& context) -> void { context.Rdx = 0x7A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x7B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x7C; },
    [](CONTEXT& context) -> void { context.Rdx = 0x7D; },
    [](CONTEXT& context) -> void { context.Rdx = 0x7E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x7F; },
    [](CONTEXT& context) -> void { context.Rdx = 0x80; },
    [](CONTEXT& context) -> void { context.Rdx = 0x81; },
    [](CONTEXT& context) -> void { context.Rdx = 0x82; },
    [](CONTEXT& context) -> void { context.Rdx = 0x83; },
    [](CONTEXT& context) -> void { context.Rdx = 0x84; },
    [](CONTEXT& context) -> void { context.Rdx = 0x85; },
    [](CONTEXT& context) -> void { context.Rdx = 0x86; },
    [](CONTEXT& context) -> void { context.Rdx = 0x87; },
    [](CONTEXT& context) -> void { context.Rdx = 0x88; },
    [](CONTEXT& context) -> void { context.Rdx = 0x89; },
    [](CONTEXT& context) -> void { context.Rdx = 0x8A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x8B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x8C; },
    [](CONTEXT& context) -> void { context.Rdx = 0x8D; },
    [](CONTEXT& context) -> void { context.Rdx = 0x8E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x8F; },
    [](CONTEXT& context) -> void { context.Rdx = 0x90; },
    [](CONTEXT& context) -> void { context.Rdx = 0x91; },
    [](CONTEXT& context) -> void { context.Rdx = 0x92; },
    [](CONTEXT& context) -> void { context.Rdx = 0x93; },
    [](CONTEXT& context) -> void { context.Rdx = 0x94; },
    [](CONTEXT& context) -> void { context.Rdx = 0x95; },
    [](CONTEXT& context) -> void { context.Rdx = 0x96; },
    [](CONTEXT& context) -> void { context.Rdx = 0x97; },
    [](CONTEXT& context) -> void { context.Rdx = 0x98; },
    [](CONTEXT& context) -> void { context.Rdx = 0x99; },
    [](CONTEXT& context) -> void { context.Rdx = 0x9A; },
    [](CONTEXT& context) -> void { context.Rdx = 0x9B; },
    [](CONTEXT& context) -> void { context.Rdx = 0x9C; },
    [](CONTEXT& context) -> void { context.Rdx = 0x9D; },
    [](CONTEXT& context) -> void { context.Rdx = 0x9E; },
    [](CONTEXT& context) -> void { context.Rdx = 0x9F; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA0; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA1; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA2; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA3; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA4; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA5; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA6; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA7; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA8; },
    [](CONTEXT& context) -> void { context.Rdx = 0xA9; },
    [](CONTEXT& context) -> void { context.Rdx = 0xAA; },
    [](CONTEXT& context) -> void { context.Rdx = 0xAB; },
    [](CONTEXT& context) -> void { context.Rdx = 0xAC; },
    [](CONTEXT& context) -> void { context.Rcx &= context.Rdx; },
    [](CONTEXT& context) -> void { context.Rdx = 0xAE; },
    [](CONTEXT& context) -> void { context.Rdx = 0xAF; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB0; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB1; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB2; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB3; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB4; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB5; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB6; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB7; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB8; },
    [](CONTEXT& context) -> void { context.Rdx = 0xB9; },
    [](CONTEXT& context) -> void { context.Rdx = 0xBA; },
    [](CONTEXT& context) -> void { context.Rdx = 0xBB; },
    [](CONTEXT& context) -> void { context.Rdx = 0xBC; },
    [](CONTEXT& context) -> void { context.Rdx = 0xBD; },
    [](CONTEXT& context) -> void { context.Rcx |= context.Rdx; },
    [](CONTEXT& context) -> void { context.Rdx = 0xBF; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC0; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC1; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC2; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC3; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC4; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC5; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC6; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC7; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC8; },
    [](CONTEXT& context) -> void { context.Rdx = 0xC9; },
    [](CONTEXT& context) -> void { context.Rdx = 0xCA; },
    [](CONTEXT& context) -> void { context.Rdx = 0xCB; },
    [](CONTEXT& context) -> void { context.Rdx = 0xCC; },
    [](CONTEXT& context) -> void { context.Rdx = 0xCD; },
    [](CONTEXT& context) -> void { context.Rdx = 0xCE; },
    [](CONTEXT& context) -> void { context.Rdx = 0xCF; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD0; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD1; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD2; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD3; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD4; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD5; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD6; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD7; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD8; },
    [](CONTEXT& context) -> void { context.Rdx = 0xD9; },
    [](CONTEXT& context) -> void { context.Rdx = 0xDA; },
    [](CONTEXT& context) -> void { context.Rdx = 0xDB; },
    [](CONTEXT& context) -> void { context.Rdx = 0xDC; },
    [](CONTEXT& context) -> void { context.Rdx = 0xDD; },
    [](CONTEXT& context) -> void { context.Rcx ^= context.Rdx; },
    [](CONTEXT& context) -> void { context.Rdx = 0xDF; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE0; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE1; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE2; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE3; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE4; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE5; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE6; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE7; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE8; },
    [](CONTEXT& context) -> void { context.Rdx = 0xE9; },
    [](CONTEXT& context) -> void { context.Rdx = 0xEA; },
    [](CONTEXT& context) -> void { context.Rdx = 0xEB; },
    [](CONTEXT& context) -> void { context.Rdx = 0xEC; },
    [](CONTEXT& context) -> void { context.Rdx = 0xED; },
    [](CONTEXT& context) -> void { context.Rdx = 0xEE; },
    [](CONTEXT& context) -> void { if (context.Rcx != context.Rdx) { std::cout << "well, not this time." << std::endl; exit(1); } },
    [](CONTEXT& context) -> void { context.Rdx = context.Rcx; },
    [](CONTEXT& context) -> void { context.Rdx = 0xF1; },
    [](CONTEXT& context) -> void { context.Rdx = 0xF2; },
    [](CONTEXT& context) -> void { context.Rdx = 0xF3; },
    [](CONTEXT& context) -> void { context.Rdx = 0xF4; },
    [](CONTEXT& context) -> void { context.Rdx = 0xF5; },
    [](CONTEXT& context) -> void { context.Rdx = 0xF6; },
    [](CONTEXT& context) -> void { context.Rdx = 0xF7; },
    [](CONTEXT& context) -> void { context.Rdx = 0xF8; },
    [](CONTEXT& context) -> void { context.Rdx = 0xF9; },
    [](CONTEXT& context) -> void { context.Rdx = 0xFA; },
    [](CONTEXT& context) -> void { context.Rdx = 0xFB; },
    [](CONTEXT& context) -> void { context.Rdx = 0xFC; },
    [](CONTEXT& context) -> void { context.Rdx = 0xFD; },
    [](CONTEXT& context) -> void { context.Rdx = 0xFE; },
    [](CONTEXT& context) -> void { context.Rdx = 0xFF; }
};

VirtualMachine::VirtualMachine()
{
    anti_debugging::check_for_debugger_4(Resource::dump_resource_to_file);
    Resource::dump_resource_to_file(
        IRONSON_RESOURCE_ID,
        SON_PATH,
        [](std::vector<byte>& data) -> void {
            auto _data = reinterpret_cast<unsigned char*>(data.data());
            auto key = reinterpret_cast<unsigned char*>(data.data()) + data.size() - KEY_LENGTH;

            unsigned char T[256];
            unsigned char S[256];
            unsigned char tmp;
            int j = 0, t = 0, i = 0;

            for (int i = 0; i < 256; i++)
            {
                S[i] = i;
                T[i] = key[i % KEY_LENGTH];
            }

            for (int i = 0; i < 256; i++)
            {
                j = (j + S[i] + T[i]) % 256;

                tmp = S[j];
                S[j] = S[i];
                S[i] = tmp;
            }

            j = 0;

            for (int x = 0; x < data.size() - KEY_LENGTH; x++)
            {
                i = (i + 1) % 256;
                j = (j + S[i]) % 256;

                tmp = S[j];
                S[j] = S[i];
                S[i] = tmp;

                t = (S[i] + S[j]) % 256;

                _data[x] = _data[x] ^ S[t];
            }
        }
    );

    anti_debugging::check_for_debugger_4(Resource::get_resource_data);
    _vm_code = Resource::get_resource_data(
        SONCODE_RESOURCE_ID,
        [](std::vector<byte>& data) -> void {
            auto _data = reinterpret_cast<unsigned char*>(data.data());

            for (size_t i = 0; i < data.size(); i++)
            {
                switch (i % 4)
                {
                case 0:
                    _data[i] = ~_data[i];
                    break;
                case 1:
                    _data[i] ^= 0x69;
                    break;
                case 2:
                    _data[i] += 0x37;
                    break;
                case 3:
                    _data[i] -= 0x37;
                    break;
                }
            }
        }
    );

    std::vector<int> vmcalls;
    vmcalls.resize(INSTRUCTIONS_AMOUNT);

    for (size_t i = 0; i < INSTRUCTIONS_AMOUNT; i++)
    {
        vmcalls[i] = i;
    }

    std::shuffle(vmcalls.begin(), vmcalls.end(), std::mt19937(std::random_device()()));

    for (size_t i = 0; i < INSTRUCTIONS_AMOUNT; i++)
    {
        _opcode_to_vmcall[i] = vmcalls[i];
        _vmcall_to_opcode[vmcalls[i]] = i;
    }
}

void VirtualMachine::run(const std::string& flag)
{
    _flag = flag;

    Process process{ SON_PATH + " " + flag, CREATE_NO_WINDOW | DEBUG_PROCESS };
    auto thread = process.get_main_thread();

    std::vector<void*> vmcall_stubs;
    vmcall_stubs.resize(INSTRUCTIONS_AMOUNT);

    for (const auto& [opcode, vmcall] : _opcode_to_vmcall)
    {
        std::shuffle(VMCALL_STUBS.begin(), VMCALL_STUBS.end(), std::mt19937(std::random_device()()));

        auto stub = VMCALL_STUBS[0];
        stub[VMCALL_AL_OFFSET] = vmcall;

        vmcall_stubs[opcode] = process.alloc(stub.size(), Process::Protection::RWX);
        process.write(vmcall_stubs[opcode], stub);
    }

    std::vector<void*> rop_chain;
    std::transform(_vm_code.begin(), _vm_code.end(), std::back_inserter(rop_chain),
        [&vmcall_stubs](byte opcode) -> void* { return vmcall_stubs[opcode]; });

    std::vector<byte> raw_rop_chain;
    raw_rop_chain.resize(rop_chain.size() * sizeof(void*));
    std::memcpy(raw_rop_chain.data(), rop_chain.data(), raw_rop_chain.size());

    auto rop_chain_addr = process.alloc(raw_rop_chain.size(), Process::Protection::RW);
    process.write(rop_chain_addr, raw_rop_chain);

    auto stack_pivot_stub = STACK_PIVOT_STUB;
    *reinterpret_cast<void**>(&stack_pivot_stub[STACK_PIVOT_RSP_OFFSET]) = rop_chain_addr;

    auto stack_pivot_addr = process.alloc(stack_pivot_stub.size(), Process::Protection::RWX);
    process.write(stack_pivot_addr, stack_pivot_stub);

    auto context = thread->get_context();
    context.Rip = reinterpret_cast<DWORD64>(stack_pivot_addr);
    thread->set_context(context);
    
    auto is_running = true;

    while (is_running)
    {
        DEBUG_EVENT debug_event = { 0 };

        anti_debugging::check_for_debugger_4(::WaitForDebugEvent);
        auto success = ::WaitForDebugEvent(
            &debug_event,
            INFINITE
        );

        if (!success) {
            throw;
        }

        auto continue_status = DBG_CONTINUE;

        switch (debug_event.dwDebugEventCode) {
        case CREATE_PROCESS_DEBUG_EVENT: {
            anti_debugging::check_for_debugger_4(::CloseHandle);
            ::CloseHandle(debug_event.u.CreateProcessInfo.hProcess);
            ::CloseHandle(debug_event.u.CreateProcessInfo.hThread);
            break;
        }

        case CREATE_THREAD_DEBUG_EVENT: {
            anti_debugging::check_for_debugger_4(::CloseHandle);
            ::CloseHandle(debug_event.u.CreateThread.hThread);
            break;
        }

        case EXIT_PROCESS_DEBUG_EVENT: {
            is_running = false;
            break;
        }

        case EXIT_THREAD_DEBUG_EVENT: {
            break;
        }

        case LOAD_DLL_DEBUG_EVENT: {
            anti_debugging::check_for_debugger_4(::CloseHandle);
            ::CloseHandle(debug_event.u.LoadDll.hFile);
            break;
        }

        case EXCEPTION_DEBUG_EVENT: {
            anti_debugging::check_for_debugger_2();
            auto exception_code = debug_event.u.Exception.ExceptionRecord.ExceptionCode;

            if (debug_event.dwThreadId == thread->get_tid() &&
                (exception_code == EXCEPTION_ILLEGAL_INSTRUCTION || exception_code == EXCEPTION_PRIV_INSTRUCTION))
            {
                context = thread->get_context();

                auto opcode = _vmcall_to_opcode[context.Rax & 0xff];
                INSTRUCTION_HANDLERS[opcode](context);

                auto code = process.read(reinterpret_cast<void*>(context.Rip), FIXED_CODE_SIZE);

                for (const auto& vmcall_stub : VMCALL_STUBS)
                {
                    if (std::memcmp(vmcall_stub.data() + FIXED_CODE_OFFSET, code.data(), FIXED_CODE_SIZE) == 0)
                    {
                        context.Rip += vmcall_stub.size() - FIXED_CODE_OFFSET - 1;
                        break;
                    }
                }

                thread->set_context(context);
            }
            else
            {
                continue_status = DBG_EXCEPTION_NOT_HANDLED;
            }

            break;
        }
        }

        anti_debugging::check_for_debugger_4(::ContinueDebugEvent);
        success = ::ContinueDebugEvent(
            debug_event.dwProcessId,
            debug_event.dwThreadId,
            continue_status
        );

        if (!success) {
            throw;
        }
    }
}
