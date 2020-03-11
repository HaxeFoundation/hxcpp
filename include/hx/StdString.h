#ifndef HX_STD_STRING_INCLUDEDED
#define HX_STD_STRING_INCLUDEDED

#include <string>

namespace hx
{
   class StdString : public std::string
   {
      public:
         StdString() : std::string() { }
         StdString(const char *inPtr) : std::string(inPtr) { }
         StdString(const char *inPtr, int inLen) : std::string(inPtr, inLen) { }
         StdString(const std::string &inS) : std::string(inS) { }
         StdString(const StdString &inS) : std::string(inS) { }

         #if (HXCPP_API_LEVEL>1)
         StdString(const Dynamic &inS) : std::string(inS.mPtr ? inS.mPtr->toString().utf8_str() : "null") { }
         StdString(const String &inS) : std::string(inS.utf8_str()) { }
         String toString() const { return String(c_str(),size()).dup(); }
         String toString() { return String(c_str(),size()).dup(); }
         operator Dynamic() const { return const_cast<StdString*>(this)->toString(); }
         #endif

         inline const StdString &toStdString() const { return *this; }

   };
}

#endif
