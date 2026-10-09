#include <hxcpp.h>
#include <Windows.h>
#include "GcRegCapture.h"

// Strictly speaking we don't need tp capture all of the integer registers
// But let's just do it for simplicity

#if defined(HXCPP_ARM64)
constexpr size_t intRegSize{ sizeof(uintptr_t) * 31 };
constexpr size_t xmmRegSize{ sizeof(ARM64_NT_NEON128) * 32 };
#elif defined(HXCPP_M64)
constexpr size_t intRegSize{ sizeof(uintptr_t) * 16 };
constexpr size_t xmmRegSize{ sizeof(M128A) * 16 };
#elif defined(HXCPP_M32)
constexpr size_t intRegSize{ sizeof(uintptr_t) * 8 };
constexpr size_t xmmRegSize{ sizeof(__m128) * 8 };
#else
#error Unsupported Architecture
#endif

hx::gc::CapturedState::CapturedState()
    : stackBase(0)
    , stackLimit(0)
    , registers(intRegSize + xmmRegSize)
{}

void hx::gc::Capture(CapturedState& state)
{
#if defined(HXCPP_ARM64)
    ARM64_NT_CONTEXT context{};
    context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL | CONTEXT_FLOATING_POINT;
    RtlCaptureContext(reinterpret_cast<PCONTEXT>(&context));

    state.stackLimit = context.Sp;

    std::memcpy(state.registers.data(), &context.X, intRegSize);
    std::memcpy(state.registers.data() + intRegSize, &context.V, xmmRegSize);
#elif defined(HXCPP_M64)
    CONTEXT context{};
    context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL | CONTEXT_FLOATING_POINT;
    RtlCaptureContext(&context);

    state.stackLimit = context.Rsp;

    std::memcpy(state.registers.data(), &context.Rax, intRegSize);
    std::memcpy(state.registers.data() + intRegSize, &context.Xmm0, xmmRegSize);
#elif defined(HXCPP_M32)
    CONTEXT context{};
    context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL | CONTEXT_EXTENDED_REGISTERS;
    RtlCaptureContext(&context);

    state.stackLimit = context.Esp;

    // ExtendedRegisters seems to be the x87 FXSAVE area
    // So we can offset 160 bytes into it to get the 8 SSE registers
    std::memcpy(state.registers.data(), &context.Edi, intRegSize);
    std::memcpy(state.registers.data() + intRegSize, reinterpret_cast<unsigned char*>(&context.ExtendedRegisters) + 160, xmmRegSize);
#else
#error Unsupported Architecture
#endif
}