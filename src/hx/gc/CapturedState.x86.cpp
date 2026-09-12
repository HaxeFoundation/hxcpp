#include <hxcpp.h>
#include <immintrin.h>
#include "GcRegCapture.h"

#if defined(HXCPP_M32)
constexpr size_t intRegSize = sizeof(uintptr_t) * 8;
constexpr size_t xmmRegSize = 16 * 8;
#elif defined(HXCPP_M64)
constexpr size_t intRegSize = sizeof(uintptr_t) * 16;
constexpr size_t xmmRegSize = 16 * 16;
#else
#error Unable to determine x86 architecture
#endif

hx::gc::CapturedState::CapturedState()
    : stackBase(0)
    , stackLimit(0)
    , registers(intRegSize + xmmRegSize)
{}

void hx::gc::Capture(CapturedState& state)
{
#if defined(HXCPP_M32)
	uintptr_t rEax{};
	volatile asm("mov %%eax, %0\n\t" : "=r" (rEax));
	uintptr_t rEbx{};
	volatile asm("mov %%ebx, %0\n\t" : "=r" (rEbx));
	uintptr_t rEcx{};
	volatile asm("mov %%ecx, %0\n\t" : "=r" (rEcx));
	uintptr_t rEdx{};
	volatile asm("mov %%edx, %0\n\t" : "=r" (rEdx));
	uintptr_t rEbp{};
	volatile asm("mov %%ebp, %0\n\t" : "=r" (rEbp));
	uintptr_t rEdi{};
	volatile asm("mov %%edi, %0\n\t" : "=r" (rEdi));
	uintptr_t rEsi{};
	volatile asm("mov %%esi, %0\n\t" : "=r" (rEsi));
	uintptr_t rEsp{};
	volatile asm("mov %%esp, %0\n\t" : "=r" (rEsp));

	state.stackLimit = rEsp;

	auto pointers = reinterpret_cast<uintptr_t*>(state.registers.data());
	pointers[0] = rEax;
	pointers[1] = rEbx;
	pointers[2] = rEcx;
	pointers[3] = rEdx;
	pointers[4] = rEsi;
	pointers[5] = rEdi;
	pointers[6] = rEsp;
	pointers[7] = rEbp;
#elif defined(HXCPP_M64)
	uintptr_t rRax{};
	volatile asm("movq %%rax, %0\n\t" : "=r" (rRax));
	uintptr_t rRbx{};
	volatile asm("movq %%rbx, %0\n\t" : "=r" (rRbx));
	uintptr_t rRcx{};
	volatile asm("movq %%rcx, %0\n\t" : "=r" (rRcx));
	uintptr_t rRdx{};
	volatile asm("movq %%rdx, %0\n\t" : "=r" (rRdx));
	uintptr_t rRsi{};
	volatile asm("movq %%rsi, %0\n\t" : "=r" (rRsi));
	uintptr_t rRdi{};
	volatile asm("movq %%rdi, %0\n\t" : "=r" (rRdi));
	uintptr_t rRsp{};
	volatile asm("movq %%rsp, %0\n\t" : "=r" (rRsp));
	uintptr_t rRbp{};
	volatile asm("movq %%rbp, %0\n\t" : "=r" (rRbp));
	uintptr_t rR8{};
	volatile asm("movq %%r8, %0\n\t" : "=r" (rR8));
	uintptr_t rR9{};
	volatile asm("movq %%r9, %0\n\t" : "=r" (rR9));
	uintptr_t rR10{};
	volatile asm("movq %%r10, %0\n\t" : "=r" (rR10));
	uintptr_t rR11{};
	volatile asm("movq %%r11, %0\n\t" : "=r" (rR11));
	uintptr_t rR12{};
	volatile asm("movq %%r12, %0\n\t" : "=r" (rR12));
	uintptr_t rR13{};
	volatile asm("movq %%r13, %0\n\t" : "=r" (rR13));
	uintptr_t rR14{};
	volatile asm("movq %%r14, %0\n\t" : "=r" (rR14));
	uintptr_t rR15{};
	volatile asm("movq %%r15, %0\n\t" : "=r" (rR15));

	state.stackLimit = rRsp;

	auto pointers = reinterpret_cast<uintptr_t*>(state.registers.data());
	pointers[ 0] = rRax;
	pointers[ 1] = rRbx;
	pointers[ 2] = rRcx;
	pointers[ 3] = rRdx;
	pointers[ 4] = rRsi;
	pointers[ 5] = rRdi;
	pointers[ 6] = rRsp;
	pointers[ 7] = rRbp;
	pointers[ 8] = rR8;
	pointers[ 9] = rR9;
	pointers[10] = rR10;
	pointers[11] = rR11;
	pointers[12] = rR12;
	pointers[13] = rR13;
	pointers[14] = rR14;
	pointers[15] = rR15;
#else
#error Unable to determine x86 architecture
#endif

	char fxsaveRegion[512] __attribute__((aligned(16)));
	_fxsave(fxsaveRegion);

	std::memcpy(state.registers.data() + intRegSize, fxsaveRegion + 160, xmmRegSize);
}