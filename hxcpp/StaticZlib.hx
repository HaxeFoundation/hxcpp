package hxcpp;

#if (hxcpp_api_level>=330)

class StaticZlib { }

#else

@:cppFileCode( 'extern "C" int zlib_register_prims();')
#if HXCPP_LINK_NO_ZLIB
@:buildXml("
<import name='${HXCPP}/project/libs/zlib/Build.xml'/>
")
#else
@:buildXml("
<target id='haxe'>
  <lib name='${HXCPP}/lib/${BINDIR}/libzlib${LIBEXTRA}${LIBEXT}'/>
</target>
")
#end
@:keep class StaticZlib
{
   static function __init__()
   {
     untyped __cpp__("zlib_register_prims();");
   }
}

#end
