package externs;

// This class acts as a container for the Rectangle implementation, which will get included
//  in the cpp file, and therefore get compiled and linked with the correct flags
@:cppInclude("./RectangleImpl.cpp")
@:keep
class RectangleApi
{
}
