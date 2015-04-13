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


value appendString(value bufVal, value stringVal)
{
   buffer buf = val_to_buffer(bufVal);
   val_buffer(buf,stringVal);
   return buffer_val(buf);
}
DEFINE_PRIM(appendString,2);


value bufferToString(value bufVal)
{
   buffer buf = val_to_buffer(bufVal);
   return buffer_to_string(buf);
}
DEFINE_PRIM(bufferToString, 1);


value valToString(value a, value b)
{
   buffer buf = alloc_buffer("String:");
   val_buffer(buf,a);
   val_buffer(buf,b);
   return buffer_to_string(buf);
}
DEFINE_PRIM(valToString, 2);



value valIsBuffer(value bufVal)
{
   return alloc_bool( val_is_buffer(bufVal) );
}
DEFINE_PRIM(valIsBuffer, 1);


value subBuffer(value inString, value inLen)
{
   buffer buf = alloc_buffer("Cold as ");
   const char *string = val_string(inString);
   buffer_append_sub(buf,string, val_int(inLen) );
   return buffer_to_string(buf);
}
DEFINE_PRIM(subBuffer, 2);


value charString(value inC0, value inC1, value inC2)
{
   buffer buf = alloc_buffer("A ");
   buffer_append_char(buf,val_int(inC0));
   buffer_append_char(buf,val_int(inC1));
   buffer_append_char(buf,val_int(inC2));
   return buffer_to_string(buf);
}
DEFINE_PRIM(charString, 3);


value byteDataSize(value byteData)
{
   CffiBytes bytes = getByteData(byteData);
   if (bytes.data==0)
      return alloc_null();
   return alloc_int(bytes.length);
}
DEFINE_PRIM(byteDataSize, 1);


value byteDataByte(value byteData, value inIndex)
{
   CffiBytes bytes = getByteData(byteData);
   if (bytes.data==0)
      return alloc_null();

   return alloc_int(bytes.data[ val_int(inIndex) ]);
}
DEFINE_PRIM(byteDataByte, 2);




