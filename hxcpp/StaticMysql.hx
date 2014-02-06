package hxcpp;

@:cppFileCode( 'extern "C" void mysql_register_prims();')
@:buildXml("
<target id='haxe'>
  <lib name='${HXCPP}/bin/${BINDIR}/libmysql5${LIBEXTRA}${LIBEXT}'/>
</target>
")
@:keep class StaticMysql
{
   static function __init__()
   {
     untyped __cpp__("mysql_register_prims();");
   }
}

