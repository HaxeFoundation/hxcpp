#ifndef STATIC_LINK
#define IMPLEMENT_API
#endif

#if defined(HX_WINDOWS) || defined(HX_MACOS) || defined(HX_LINUX)
// Include neko glue....
#define NEKO_COMPATIBLE
#endif
#include <hx/CFFIPrime.h>
#include <math.h>


int addInts(int a, int b)
{
   return a+b;
}
DEFINE_PRIME2(addInts);

void printString(const char *inMessage)
{
   printf("Message : %s.\n", inMessage);
}
DEFINE_PRIME1v(printString);


double distance3D(int x, int y, int z)
{
   return sqrt( (double)(x*x + y*y+ z*z) );
}
DEFINE_PRIME3(distance3D);

void fields(value object)
{
   printf("x : %f\n", val_field_numeric(object, val_id("x")) );
}
DEFINE_PRIME1v(fields);


HxString stringVal(HxString inString)
{
   printf("String : %s (%d)\n", inString.__s, inString.length);
   return HxString("Ok");
}
DEFINE_PRIME1(stringVal);


// Conflict with name - use anon-namespace
namespace {
value select(int which, value object0, value object1, value object2, value object3)
{
   switch(which)
   {
      case 0:  return object0;
      case 1:  return object1;
      case 2:  return object2;
      case 3:  return object3;
      default: return alloc_null();
   }
}
DEFINE_PRIME5(select);
}

float floats(bool add, float firstVal, float secondVal)
{
   return add ? firstVal + secondVal : firstVal - secondVal;
}
DEFINE_PRIME3(floats);


int multi5(int i0, int i1, int i2, int i3, int i4)
{
   return i0 + i1 + i2 + i3 + i4;
}
DEFINE_PRIME5(multi5);



int multi6(int i0, int i1, int i2, int i3, int i4, int i5)
{
   return i0 + i1 + i2 + i3 + i4 + i5;
}
DEFINE_PRIME6(multi6);

int multi7(int i0, int i1, int i2, int i3, int i4, int i5, int i6)
{
   return i0 + i1 + i2 + i3 + i4 + i5 + i6;
}
DEFINE_PRIME7(multi7);


int multi8(int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7)
{
   return i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7;
}
DEFINE_PRIME8(multi8);


int multi9(int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8)
{
   return i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8;
}
DEFINE_PRIME9(multi9);

int multi10(int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9)
{
   return i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9;
}
DEFINE_PRIME10(multi10);

int multi11(int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10)
{
   return i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9 + i10;
}
DEFINE_PRIME11(multi11);


int multi12(int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10, int i11)
{
   return i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9 + i10 + i11;
}
DEFINE_PRIME12(multi12);


// Old-style CFFI
value isBool(value inVal)
{
   return alloc_bool( val_is_bool(inVal) );
}
DEFINE_PRIM(isBool,1);

value isNull(value inVal)
{
   return alloc_bool( val_is_null(inVal) );
}
DEFINE_PRIM(isNull,1);


value allocNull()
{
   return alloc_null();
}
DEFINE_PRIM(allocNull,0);









