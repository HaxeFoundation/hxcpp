#include <hxcpp.h>
#include <map>
#include <vector>
#include <hx/Thread.h>
#include <hx/OS.h>



#ifdef HX_WINRT
   #define PROFILE_PRINT WINRT_LOG
#else

   #if defined(ANDROID)
      #include <android/log.h>
      #define DBGLOG(...) __android_log_print(ANDROID_LOG_INFO, "HXCPP", __VA_ARGS__)
   #else
      #include <stdio.h>
      #define DBGLOG printf
   #endif

   #define PROFILE_PRINT(...)                      \
        if (out) {                              \
            fprintf(out, __VA_ARGS__);          \
        }                                       \
        else {                                  \
            DBGLOG(__VA_ARGS__);                \
        }
#endif



namespace hx
{

// Profiler functionality separated into this class
class Profiler
{
public:

    Profiler(const String &inDumpFile)
        : mT0(0)
    {
        mDumpFile = inDumpFile;

        // When a profiler exists, the profiler thread needs to exist
        gThreadMutex.Lock();

        gThreadRefCount += 1;

        if (gThreadRefCount == 1) {
            HxCreateDetachedThread(ProfileMainLoop, 0);
        }

        gThreadMutex.Unlock();
    }

    ~Profiler()
    {
        gThreadMutex.Lock();

        gThreadRefCount -= 1;

        gThreadMutex.Unlock();
    }

   void sample(hx::StackContext *stack)
   {
       if (mT0 == gProfileClock) {
           return;
       }

       // Latch the profile clock and calculate the time since the last profile
       // clock tick
       int clock = gProfileClock;
       int delta = clock - mT0;
       if (delta < 0) {
           delta = 1;
       }
       mT0 = clock;

       int depth = stack->getDepth();

       std::map<const char *, bool> alreadySeen;

       // Add children time in to each stack element
       for (int i = 0; i < (depth - 1); i++) {
           const char *fullName = stack->getFullNameAtDepth(i);
           ProfileEntry &pe = mProfileStats[fullName];
           if (!alreadySeen.count(fullName)) {
               pe.total += delta;
               alreadySeen[fullName] = true;
           }
           // For everything except the very bottom of the stack, add the time to
           // that child's total with this entry
           pe.children[stack->getFullNameAtDepth(i + 1)] += delta;
       }

       // Add the time into the actual function being executed
       if (depth > 0) {
           mProfileStats[stack->getFullNameAtDepth(depth - 1)].self += delta;
       }
   }



    void DumpStats()
    {
        FILE *out = 0;
        if (mDumpFile.length > 0)
        {
            out = fopen(mDumpFile.c_str(), "wb");
            if (!out)
            {
                return;
            }
        }

        std::vector<ResultsEntry> results;

        results.reserve(mProfileStats.size());

        int total = 0;
        std::map<const char *, ProfileEntry>::iterator iter = 
            mProfileStats.begin();
        while (iter != mProfileStats.end()) {
            ProfileEntry &pe = iter->second;
            ResultsEntry re;
            re.fullName = iter->first;
            re.self = pe.self;
            re.total = pe.total;
            re.childrenPlusSelf = re.self;
            ChildEntry internal;
            internal.fullName = "(internal)";
            internal.self = re.self;
            std::map<const char *, int>::iterator childIter =
                pe.children.begin();
            int childTotal = 0;
            while (childIter != pe.children.end()) {
                ChildEntry ce;
                ce.fullName = childIter->first;
                ce.self = childIter->second;
                re.childrenPlusSelf += ce.self;
                re.children.push_back(ce);
                childIter++;
            }
            re.children.push_back(internal);
            std::sort(re.children.begin(), re.children.end());
            results.push_back(re);
            total += re.self;
            iter++;
        }

        std::sort(results.begin(), results.end());

        double scale = total ? (100.0 / total) : 1.0;

        int size = results.size();

        for (int i = 0; i < size; i++) {
            ResultsEntry &re = results[i];
            PROFILE_PRINT("%s %.2f%%/%.2f%%\n", re.fullName, re.total * scale,
                          re.self * scale);
            if (re.children.size() == 1) {
                continue;
            }

            int childrenSize = re.children.size();
            for (int j = 0; j < childrenSize; j++) {
                ChildEntry &ce = re.children[j];
                PROFILE_PRINT("   %s %.1f%%\n", ce.fullName,
                              (100.0 * ce.self) / re.childrenPlusSelf);
            }
        }

        if (out) {
            fclose(out);
        }
    }

private:

    struct ProfileEntry
    {
        ProfileEntry()
            : self(0), total(0)
        {
        }

        int self;
        std::map<const char *, int> children;
        int total;
    };

    struct ChildEntry
    {
        bool operator <(const ChildEntry &inRHS) const
        {
            return self > inRHS.self;
        }

        const char *fullName;
        int self;
    };

    struct ResultsEntry
    {
        bool operator <(const ResultsEntry &inRHS) const
        {
            return ((total > inRHS.total) ||
                    ((total == inRHS.total) && (self < inRHS.self)));
        }
        
        const char *fullName;
        int self;
        std::vector<ChildEntry> children;
        int total;
        int childrenPlusSelf;
    };

    static THREAD_FUNC_TYPE ProfileMainLoop(void *)
    {
        int millis = 1;

        while (gThreadRefCount > 0) { 
            HxSleep(millis);

            int count = gProfileClock + 1;
            gProfileClock = (count < 0) ? 0 : count;
        }

        THREAD_FUNC_RET
    }

    String mDumpFile;
    int mT0;
    std::map<const char *, ProfileEntry> mProfileStats;

    static HxMutex gThreadMutex;
    static int gThreadRefCount;
    static int gProfileClock;
};
/* static */ HxMutex Profiler::gThreadMutex;
/* static */ int Profiler::gThreadRefCount;
/* static */ int Profiler::gProfileClock;

void profDestroy(Profiler * prof)
{
   delete prof;
}
void profAttach(Profiler *prof, StackContext *ctx)
{
}
void profDetach(Profiler *, StackContext *ctx)
{
    delete ctx->mProfiler;
    ctx->mProfiler = 0;
}
void profSample(Profiler *prof, StackContext *ctx)
{
   prof->sample(ctx);
}



} // end namespace hx



void __hxcpp_start_profiler(String inDumpFile)
{
   hx::StackContext *stack = hx::StackContext::getCurrent();
   delete stack->mProfiler;
   stack->mProfiler = new hx::Profiler(inDumpFile);
}


void __hxcpp_stop_profiler()
{
   hx::StackContext *stack = hx::StackContext::getCurrent();
   stack->mProfiler->DumpStats();
   delete stack->mProfiler;
   stack->mProfiler = 0;
}



