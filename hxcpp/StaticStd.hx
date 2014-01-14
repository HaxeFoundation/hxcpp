package hxcpp;

@:cppFileCode( 'extern "C" void std_register_prims();')
@:buildXml("
<target id='haxe'>
  <lib name='${HXCPP}/bin/${BINDIR}/libstd${LIBEXTRA}${LIBEXT}'/>
</target>
")
@:keep class StaticStd
{
   static function __init__()
   {
     untyped __cpp__("std_register_prims();");
   }
}

