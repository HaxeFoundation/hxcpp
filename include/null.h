#ifndef HX_NULL_H
#define HX_NULL_H



// --- null value  ---------------------------------------------------------
//
// This is used by external operatator and return statments - Most will
//  use operator overloading to convert to the null pointer


// Forward declare ...
class String;
namespace hx { template<typename O> class ObjectPtr; }


class null
{
   public:
     inline null(){ } 

     template<typename T> explicit inline null(const hx::ObjectPtr<T> &){ } 
     template<typename T> explicit inline null(const String &){ } 
     explicit inline null(double){ } 
     explicit inline null(int){ } 
     explicit inline null(bool){ } 

     operator char * () const { return 0; }
     operator wchar_t * () const { return 0; }
     operator bool () const { return false; }
     operator int () const { return 0; }
     operator double () const { return 0; }
     operator unsigned char () const { return 0; }

     bool operator == (const null &inRHS) const { return true; }
     bool operator != (const null &inRHS) const { return false; }

     bool operator == (int inRHS) const { return false; }
     bool operator != (int inRHS) const { return true; }
     bool operator == (double inRHS) const { return false; }
     bool operator != (double inRHS) const { return true; }
     bool operator == (bool inRHS) const { return false; }
     bool operator != (bool inRHS) const { return true; }

     template<typename T> inline bool operator == (const hx::ObjectPtr<T> &);
     template<typename T> inline bool operator != (const hx::ObjectPtr<T> &);
     template<typename T> inline bool operator == (const Array<T> &);
     template<typename T> inline bool operator != (const Array<T> &);
     inline bool operator == (const Dynamic &);
     inline bool operator != (const Dynamic &);
     inline bool operator == (const String &);
     inline bool operator != (const String &);
};

typedef null Void;

inline bool operator == (bool inLHS,const null &inRHS)  { return false; }
inline bool operator != (bool inLHS,const null &inRHS)  { return true; }
inline bool operator == (double inLHS,const null &inRHS)  { return false; }
inline bool operator != (double inLHS,const null &inRHS)  { return true; }
inline bool operator == (int inLHS,const null &inRHS)  { return false; }
inline bool operator != (int inLHS,const null &inRHS)  { return true; }




#endif

