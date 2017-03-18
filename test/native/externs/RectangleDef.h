#ifndef RECTANGLE_DEF_INCLUDED
#define RECTANGLE_DEF_INCLUDED

struct Rectangle
{
   static int instanceCount;

   int x;
   int y;
   int width;
   int height;

   inline Rectangle(int inX=0, int inY=0, int inW=0, int inH=0) :
      x(inX), y(inY), width(inW), height(inH)
   {
      instanceCount++;
   }
   inline ~Rectangle()
   {
      instanceCount--;
   }

   int area();
};

#endif
