package com.thomasuster.threadpool;
#if cpp
import cpp.vm.Mutex;
#else
import neko.vm.Mutex;
#end
class ThreadModel {
    public var id:Int;
    public var start:Int;
    public var end:Int;
    public var mutex:Mutex;
    public var pending:Bool;
    public var done:Bool;

    public function new():Void {
        mutex = new Mutex();
    }
}