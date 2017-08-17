package com.thomasuster.threadpool;

#if cpp
import cpp.vm.Thread;
#else
import neko.vm.Thread;
#end

private typedef LoopIndex = Int;
private typedef ThreadID = Int;
private typedef Task = ThreadID->Void;
private typedef Loop = ThreadID->LoopIndex->Void;

class ThreadPool {

    var num:Int;
    var theEnd:Bool;
    var models:Array<ThreadModel>;
    var queue:Array<Task>;

    public function new(num:Int):Void {
        this.num = num;
        models = [];
        queue = [];

        for (i in 0...num) {
            var model = new ThreadModel();
            model.id = i;
            model.mutex.acquire();
            models.push(model);
        }

        for (i in 0...num) {
            Thread.create(function() {
                threadLoop(models[i]);
            });
        }
    }

    function threadLoop(model:ThreadModel):Void {
        var id:ThreadID = model.id; 
        while(true) {
            model.mutex.acquire();
            model.pending = false;
            if(theEnd) {
                model.mutex.release();
                break;
            }
            if(!model.done) {
                for (i in model.start...model.end)
                    queue[i](id);
                model.done = true;   
            }
            model.mutex.release();
        }
    }

    public function addConcurrent(task:Task):Void {
        queue.push(task);
    }

    public function blockRunAll():Void {
        var numPerThread:Int = Math.floor(queue.length / num);
        var remainder:Int = queue.length - num * numPerThread;
        for (i in 0...num) {
            models[i].start = i*numPerThread;
            models[i].end = models[i].start + numPerThread;
            if(i == num-1) {
                models[i].end += remainder;
            }
            models[i].pending = true;
            models[i].mutex.release();
        }
        
        var i:Int = 0;
        while(i < num) {
            models[i].mutex.acquire();
            if(models[i].pending) {
                models[i].mutex.release();
            }
            else {
                models[i].done = false;
                i++;
            }
        }
        queue = [];
    }

    public function distributeLoop(length:Int, loop:Loop):Void {
        var numPerThread:Int = Math.floor(length / num);
        var remainder:Int = length - numPerThread * num;
        for (i in 0...num) {
            var start:Int = i * numPerThread;
            var end:Int = start + numPerThread;
            if(i == num-1) {
                end+=remainder;
            }
            for (j in start...end) {
                addConcurrent(function(i) {
                    loop(i,j);
                });
            }
        }
    }

    public function end():Void {
        theEnd = true;
        for (i in 0...num) {
            models[i].mutex.release();
        }
    }

}