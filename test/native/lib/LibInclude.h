#ifndef LIB_INCLUDE_INCLUDED
#define LIB_INCLUDE_INCLUDED

struct RGB
{
   unsigned char r;
   unsigned char g;
   unsigned char b;


   RGB(int inR=0, int inG=0, int inB=0) :
      r(inR), g(inG), b(inB)
   {
   }

   int getLuma();
   int toInt();
};

struct Gradient
{
   RGB   colour0;
   RGB   colour1;
   int   steps;
};


#endif
