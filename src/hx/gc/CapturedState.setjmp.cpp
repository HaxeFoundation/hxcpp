#include <hxcpp.h>
#include "GcRegCapture.h"
#include <setjmp.h>

hx::gc::CapturedState::CapturedState()
    : stackBase(0)
    , stackLimit(0)
    , registers(sizeof(jmp_buf))
{}

void hx::gc::Capture(CapturedState& state)
{
    volatile uintptr_t v{};
    state.stackLimit = reinterpret_cast<uintptr_t>(&v);

    jmp_buf buffer{};
    if (0 != setjmp(buffer))
    {
        hx::CriticalError(HX_CSTRING("Failed to save jump buffer"));
    }

    std::memcpy(state.registers.data(), buffer, state.registers.size());
}