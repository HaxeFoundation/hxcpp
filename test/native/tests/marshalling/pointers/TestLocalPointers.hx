package tests.marshalling.pointers;

import utest.Assert;
import utest.Test;

class TestLocalPointers extends Test {
    function test_null_ptr() {
        final ptr : Context = null;

        Assert.isTrue(ptr == null);
    }

    function test_null_captured_ptr() {
        final ptr : Context = null;
        final f = () -> {
            return ptr;
        }

        f();

        Assert.isTrue(ptr == null);
    }

    function test_non_null_ptr() {
        final ptr = Context.create();

        Assert.isTrue(ptr != null);
    }

    function test_non_null_captured_ptr() {
        final ptr = Context.create();
        final f = () -> {
            return ptr;
        }

        f();

        Assert.isTrue(ptr != null);
    }

    function test_assinging_ptr() {
        var ptr : Context = null;

        Assert.isTrue(ptr == null);

        ptr = Context.create();

        Assert.isTrue(ptr != null);
    }

    function test_assinging_captured_ptr() {
        var ptr : Context = null;
        final f = () -> {
            return ptr;
        }

        f();

        Assert.isTrue(ptr == null);

        ptr = Context.create();

        Assert.isTrue(ptr != null);
    }

    function test_assinging_captured_ptr_in_closure() {
        var ptr : Context = null;
        final f = () -> {
            ptr = Context.create();
        }

        f();

        Assert.isTrue(ptr != null);
    }

    function test_pointer_equality() {
        final ptr  = Context.create();
        final copy = ptr;
        
        Assert.isTrue(ptr == copy);
    }

    function test_captured_pointer_equality() {
        final ptr  = Context.create();
        final f    = () -> {
            return ptr;
        }
        final copy = f();
        
        Assert.isTrue(ptr == copy);
    }

    function test_captured_pointer_inequality() {
        final ptr  = Context.create();
        final f    = () -> {
            return Context.create();
        }
        final copy = f();
        
        Assert.isTrue(ptr != copy);
    }

    function test_pointer_field_access() {
        final ptr = Context.create();

        Assert.equals(7, ptr.number);
    }

    function test_captured_pointer_field_access() {
        final ptr = Context.create();
        final f = () -> {
            return ptr.number;
        }

        Assert.equals(7, f());
    }

    function test_pointer_field_mutation() {
        final ptr = Context.create();

        ptr.number = 14;

        Assert.equals(14, ptr.number);
    }

    function test_capturd_pointer_field_mutation() {
        final ptr = Context.create();
        final f = () -> {
            ptr.number = 14;
        }

        f();

        Assert.equals(14, ptr.number);
    }

    function test_pointer_function_call() {
        final ptr = Context.create();

        Assert.equals(14, ptr.double());
    }

    function test_captured_pointer_function_call() {
        final ptr = Context.create();
        final f = () -> {
            return ptr.double();
        }

        Assert.equals(14, f());
    }

    function test_dynamic() {
        final ptr = Context.create();

        by_dynamic(ptr);

        Assert.equals(20, ptr.number);
    }

    function test_promoted_dynamic() {
        var ptr = null;

        final f = () -> {
            ptr = Context.create();
        }

        f();

        by_dynamic(ptr);

        Assert.equals(20, ptr.number);
    }

    function test_anon() {
        final ptr = Context.create();

        by_anon({ v : ptr });

        Assert.equals(20, ptr.number);
    }

    function test_lambda_return() {
        final f = () -> {
            return Context.create();
        }

        final ptr = f();

        Assert.notNull(ptr);
    }

    function test_to_string() {
        final ptr = Context.create();
        final str = Std.string(ptr);

        Assert.notNull(str);
    }

    function test_just_creation() {
        Context.create();

        Assert.pass();
    }

    function test_reassignment() {
        var ptr = Context.create();

        ptr.number = 20;

        ptr = Context.create();

        Assert.equals(7, ptr.number);
    }

    function test_null_access_exception() {
        final ptr : Context = null;

        Assert.raises(() -> ptr.number = 7);
    }

    function test_promoted_null_access_exception() {
        final ptr : Context = null;
        final f = () -> {
            return ptr;
        }

        f();

        Assert.raises(() -> ptr.number = 7);
    }

    // function test_weird_nullness() {
    //     function isAnyNull(a:Any) {
    //         return a == null;
    //     }

    //     final ptr = Context.createNull();

    //     Assert.isTrue(ptr == null);
    //     Assert.isTrue(isAnyNull(ptr));
    // }

    // function test_weird_promoted_nullness() {
    //     function isAnyNull(a:Any) {
    //         return a == null;
    //     }

    //     final ptr = Context.createNull();
    //     final f   = () -> {
    //         return ptr;
    //     }

    //     f();

    //     Assert.isTrue(ptr == null);
    //     Assert.isTrue(isAnyNull(ptr));
    // }

    //

    function by_anon(a : { v : Context }) {
        a.v.number = 20;
    }

    function by_dynamic(v:Dynamic) {
        (v:Context).number = 20;
    }
}