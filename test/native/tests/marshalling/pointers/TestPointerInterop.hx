package tests.marshalling.pointers;

import cpp.RawPointer;
import cpp.Star;
import cpp.Pointer;
import utest.Test;
import utest.Assert;

@:include('ctx.hpp')
private extern class NativeFunction {
    @:native('ctx_ptr')
    static function ctx_ptr(ctx:Context):Void;

    @:native('ctx_ptr_ptr')
    static function ctx_ptr_ptr(ctx:Context):Void;

    @:native('ctx_void_ptr')
    static function ctx_void_ptr(ctx:Context):Void;

    @:native('ctx_void_ptr_ptr')
    static function ctx_void_ptr_ptr(ctx:Context):Void;
}

private class HaxeFunctions {
    @:unreflective public static function set_number_ptr(ctx : Pointer<Context>) {
        ctx[0].number = 20;
    }

    @:unreflective public static function set_number_raw_ptr(ctx : RawPointer<Context>) {
        ctx[0].number = 20;
    }

    // @:unreflective public static function set_number_star(ctx : Star<Context>) {
    //     ctx.number = 20;
    // }

    @:unreflective public static function is_ptr_null(ctx : Pointer<Context>) {
        return ctx[0] == null;
    }

    @:unreflective public static function is_raw_ptr_null(ctx : RawPointer<Context>) {
        return ctx[0] == null;
    }

    // @:unreflective public static function is_star_null(ctx : Star<Context>) {
    //     return ctx == null;
    // }
}

class TestPointerInterop extends Test {
    function test_implicit_to_ptr() {
        final ctx = Context.create();

        Assert.equals(7, ctx.number);

        NativeFunction.ctx_ptr(ctx);

        Assert.equals(20, ctx.number);
    }

    function test_implicit_to_ptr_ptr() {
        final ctx = Context.create();

        Assert.equals(7, ctx.number);

        NativeFunction.ctx_ptr_ptr(ctx);

        Assert.equals(20, ctx.number);
    }

    function test_implicit_to_ptr_ptr_null() {
        final ctx : Context = null;

        Assert.isTrue(ctx == null);

        NativeFunction.ctx_ptr_ptr(ctx);

        if (Assert.isTrue(ctx != null)) {
            Assert.equals(20, ctx.number);
        }
    }

    function test_implicit_to_ptr_ptr_copy() {
        final ctx  = Context.create();
        final copy = ctx;

        Assert.equals(7, ctx.number);

        NativeFunction.ctx_ptr_ptr(ctx);

        Assert.equals(20, ctx.number);
        Assert.equals(7, copy.number);
    }

    function test_implicit_to_void_ptr() {
        final ctx = Context.create();

        Assert.equals(7, ctx.number);

        NativeFunction.ctx_void_ptr(ctx);

        Assert.equals(20, ctx.number);
    }

    function test_implicit_to_void_ptr_ptr() {
        final ctx = Context.create();

        Assert.equals(7, ctx.number);

        NativeFunction.ctx_void_ptr_ptr(ctx);

        Assert.equals(20, ctx.number);
    }

    function test_implicit_to_void_ptr_ptr_null() {
        final ctx : Context = null;

        Assert.isTrue(ctx == null);

        NativeFunction.ctx_void_ptr_ptr(ctx);

        if (Assert.isTrue(ctx != null)) {
            Assert.equals(20, ctx.number);
        }
    }

    function test_to_cpp_pointer() {
        final ctx = Context.create();

        HaxeFunctions.set_number_ptr(cast ctx);

        Assert.equals(20, ctx.number);
    }

    function test_null_to_cpp_pointer_throws() {
        final ctx : Context = null;

        Assert.isTrue(HaxeFunctions.is_ptr_null(cast ctx));
    }

    function test_to_cpp_raw_pointer() {
        final ctx = Context.create();

        HaxeFunctions.set_number_raw_ptr(cast ctx);

        Assert.equals(20, ctx.number);
    }

    function test_null_to_cpp_raw_pointer_throws() {
        final ctx : Context = null;

        Assert.isTrue(HaxeFunctions.is_raw_ptr_null(cast ctx));
    }

    // function test_to_cpp_star() {
    //     final ctx = Context.create();

    //     HaxeFunctions.set_number_star(cast ctx);

    //     Assert.equals(20, ctx.number);
    // }

    // function test_null_to_cpp_star_throws() {
    //     final ctx : Context = null;

    //     Assert.isTrue(HaxeFunctions.is_star_null(cast ctx));
    // }
}