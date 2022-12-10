#pragma once

HX_DECLARE_CLASS2(hx, asys, Context)

namespace hx
{
    namespace asys
    {
        class Context_obj : public Object
        {
        public:
            static Context create();

            virtual void enqueue(Dynamic event) = 0;
            virtual Dynamic enqueue(Dynamic event, int intervalMs) = 0;
            virtual void cancel(Dynamic) = 0;
            virtual void loop() = 0;
        };
    }
}