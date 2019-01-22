package hxcpp;

#if (hxcpp_api_level>=330)

class StaticStd { }

#else

@:cppFileCode( 'extern "C" int std_register_prims();')
@:buildXml("
<target id='haxe'>
  <lib name='${HXCPP}/lib/${BINDIR}/libstd${LIBEXTRA}${LIBEXT}'/>
   <lib name='ws2_32.lib' if='windows' unless='static_link' />
</target>
")
@:keep class StaticStd
{
   static function __init__()
   {
     untyped __cpp__("std_register_prims();");
   }
}


#end
