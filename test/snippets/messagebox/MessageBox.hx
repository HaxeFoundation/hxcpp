import cpp.*;

// Windows only.
// Compile with "-D no_console" for best effect

@:cppFileCode("#include <windows.h>")
class MessageBox
{
   public static function main()
   {
      var messageBox:cpp.Function< Pointer< Void > ->
                                   ConstCharStar ->
                                   ConstCharStar ->
                                   Int  -> Int,  cpp.abi.Winapi > =
           Function.getProcAddress("User32.dll", "MessageBoxA");

      messageBox(null, "Hello, World!", "Hxcpp MessageBox", 0);

      // This will actually print out if you have started from a console (not double-click)
      trace("Sneaky trace");
   }
}


