package tests;

import utest.Test;
import utest.Assert;
import externs.RGB;

class TestRgb extends Test
{
   public function testCreate()
   {
      // Pointer-like sysntax
      var rgbPtr = RGB.create(255,0,128);
      Assert.equals( rgbPtr.ptr.toInt(), 0xff0080 );
      rgbPtr.ptr.deleteMe();


      // Structure-like syntax
      var rgbStruct:RGBStruct = null;
      rgbStruct.r = 1;
      rgbStruct = null;
      rgbStruct.r = 1;
      rgbStruct.g = 2;
      rgbStruct.b = 3;
      Assert.equals( rgbStruct.toInt(), 0x010203 );
      // Store in dynamic
      var d:Dynamic = rgbStruct;
      
      // Reference (pointer) like syntax
      var rgbRef:RGBRef = rgbStruct;
      rgbRef.g = 255;
      Assert.equals( rgbStruct.toInt(), 0x01ff03 );

      // Get from dynamic
      rgbStruct = d;
      Assert.equals( rgbStruct.toInt(), 0x010203 );

      var rgbStruct2:RGBStruct = cast rgbRef;
      Assert.equals( rgbStruct2.toInt(), 0x010203 );

      // Reference refers to rgbStruct, not rgbStruct2
      rgbRef.b = 0;
      Assert.equals( rgbStruct2.toInt(), 0x010203 );
      Assert.equals( rgbStruct.toInt(), 0x010200 );

      
      // TODO - non-dynamic versions
      var d2:Dynamic = rgbStruct2;
      // == dynamic
      Assert.equals( d2, d );
      // != dynamic
      var d0:Dynamic = rgbStruct;
      Assert.notEquals( d0, d );

   }
}

