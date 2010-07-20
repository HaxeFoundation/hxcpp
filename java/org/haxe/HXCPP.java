package org.haxe;

// Wrapper for native library

public class HXCPP {
     static public void run(String inClassName) {
         System.loadLibrary(inClassName);
         main();
     }
    
     public static native void main(); 
}

