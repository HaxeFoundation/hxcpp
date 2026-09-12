#include <hxcpp.h>
#include "GcRegCapture.h"

constexpr size_t intRegSize{ sizeof(uintptr_t) * 31 };

hx::gc::CapturedState::CapturedState()
    : stackBase(0)
    , stackLimit(0)
    , registers(intRegSize)
{}

void hx::gc::Capture(CapturedState& state)
{
    uintptr_t rX0{};
    asm("mov %0, x0\n\t" : "=r" (rX0));
    uintptr_t rX1{};
    asm("mov %0, x1\n\t" : "=r" (rX1));
    uintptr_t rX2{};
    asm("mov %0, x2\n\t" : "=r" (rX2));
    uintptr_t rX3{};
    asm("mov %0, x3\n\t" : "=r" (rX3));
    uintptr_t rX4{};
    asm("mov %0, x4\n\t" : "=r" (rX4));
    uintptr_t rX5{};
    asm("mov %0, x5\n\t" : "=r" (rX5));
    uintptr_t rX6{};
    asm("mov %0, x6\n\t" : "=r" (rX6));
    uintptr_t rX7{};
    asm("mov %0, x7\n\t" : "=r" (rX7));
    uintptr_t rX8{};
    asm("mov %0, x8\n\t" : "=r" (rX8));
    uintptr_t rX9{};
    asm("mov %0, x9\n\t" : "=r" (rX9));
    uintptr_t rX10{};
    asm("mov %0, x10\n\t" : "=r" (rX10));
    uintptr_t rX11{};
    asm("mov %0, x11\n\t" : "=r" (rX11));
    uintptr_t rX12{};
    asm("mov %0, x12\n\t" : "=r" (rX12));
    uintptr_t rX13{};
    asm("mov %0, x13\n\t" : "=r" (rX13));
    uintptr_t rX14{};
    asm("mov %0, x14\n\t" : "=r" (rX14));
    uintptr_t rX15{};
    asm("mov %0, x15\n\t" : "=r" (rX15));
    uintptr_t rX16{};
    asm("mov %0, x16\n\t" : "=r" (rX16));
    uintptr_t rX17{};
    asm("mov %0, x17\n\t" : "=r" (rX17));
    uintptr_t rX18{};
    asm("mov %0, x18\n\t" : "=r" (rX18));
    uintptr_t rX19{};
    asm("mov %0, x19\n\t" : "=r" (rX19));
    uintptr_t rX20{};
    asm("mov %0, x20\n\t" : "=r" (rX20));
    uintptr_t rX21{};
    asm("mov %0, x21\n\t" : "=r" (rX21));
    uintptr_t rX22{};
    asm("mov %0, x22\n\t" : "=r" (rX22));
    uintptr_t rX23{};
    asm("mov %0, x23\n\t" : "=r" (rX23));
    uintptr_t rX24{};
    asm("mov %0, x24\n\t" : "=r" (rX24));
    uintptr_t rX25{};
    asm("mov %0, x25\n\t" : "=r" (rX25));
    uintptr_t rX26{};
    asm("mov %0, x26\n\t" : "=r" (rX26));
    uintptr_t rX27{};
    asm("mov %0, x27\n\t" : "=r" (rX27));
    uintptr_t rX28{};
    asm("mov %0, x28\n\t" : "=r" (rX28));
    uintptr_t rX29{};
    asm("mov %0, x29\n\t" : "=r" (rX29));
    uintptr_t rX30{};
    asm("mov %0, x30\n\t" : "=r" (rX30));

    uintptr_t rSp{};
    asm("mov %0, k\n\t" : "=r" (rSp));

    state.stackLimit = rSp;

    auto pointers = reinterpret_cast<uintptr_t*>(state.registers.data());
    pointers[0] = rX0;
    pointers[1] = rX1;
    pointers[2] = rX2;
    pointers[3] = rX3;
    pointers[4] = rX4;
    pointers[5] = rX5;
    pointers[6] = rX6;
    pointers[7] = rX7;
    pointers[8] = rX8;
    pointers[9] = rX9;
    pointers[10] = rX10;
    pointers[11] = rX11;
    pointers[12] = rX12;
    pointers[13] = rX13;
    pointers[14] = rX14;
    pointers[15] = rX15;
    pointers[16] = rX16;
    pointers[17] = rX17;
    pointers[18] = rX18;
    pointers[19] = rX19;
    pointers[20] = rX20;
    pointers[21] = rX21;
    pointers[22] = rX22;
    pointers[23] = rX23;
    pointers[24] = rX24;
    pointers[25] = rX25;
    pointers[26] = rX26;
    pointers[27] = rX27;
    pointers[28] = rX28;
    pointers[29] = rX29;
    pointers[30] = rX30;
}