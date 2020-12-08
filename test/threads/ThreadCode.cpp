#include <hx/Native.h>
#include <hx/Thread.h>

THREAD_FUNC_TYPE threadFunc(void *data)
{
   printf("In thread\n");
   hx::NativeAttach attach;
   Test_obj::callFromThread();
   THREAD_FUNC_RET
}

void runThread()
{
   HxCreateDetachedThread(threadFunc,nullptr);
}

