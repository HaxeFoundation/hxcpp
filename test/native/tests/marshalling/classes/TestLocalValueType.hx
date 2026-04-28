package tests.marshalling.classes;

import utest.Assert;
import utest.Test;

private class StdVectorIterator {
    final max : Int;
    var current : Int;

    public function new() {
        max     = 10 + Std.random(10);
        current = 0;
    }

    public function hasNext() {
        return current < max;
    }

    public function next() {
        return new StdVector<Int>(current++);
    }
}

class TestLocalValueType extends Test {
    /**
     * Declaring a new variable of a value type will copy the right hand side value.
     */
    function test_var_copying() {
        final v1 = new StdVector<Int>();
        final v2 = v1;

        v1.resize(10);

        Assert.equals(10, v1.size());
        Assert.equals( 0, v2.size());
    }

    /**
     * Value type variables can be re-assigned and the internal reference is still correct.
     */
    function test_var_assignment() {
        var v = new StdVector<Int>(50);

        v = new StdVector<Int>();

        Assert.equals(0, v.size());
    }

    function test_nullable_var_null() {
        final v : Null<StdVector<Int>> = null;

        Assert.isNull(v);
    }

    function test_nullable_var_not_null() {
        final v : Null<StdVector<Int>> = new StdVector<Int>();

        Assert.notNull(v);
    }

    function test_nullable_var_null_copy() {
        final v1 : Null<StdVector<Int>> = null;
        final v2 = v1;

        Assert.isNull(v1);
        Assert.isNull(v2);
    }

    function test_nullable_var_to_non_null() {
        final v1 : Null<StdVector<Int>> = new StdVector<Int>();
        final v2 : StdVector<Int> = v1;

        Assert.notNull(v1);
        Assert.notNull(v2);
    }

    function test_nullable_var_null_to_non_null() {
        final v1 : Null<StdVector<Int>> = null;

        Assert.raises(() -> {
            final _ : StdVector<Int> = v1;
        });
    }

    function test_assigning_null_to_non_null_var() {
        Assert.raises(() -> {
            var v = new StdVector<Int>(50);

            v = null;
        });
    }

    function test_initialising_non_null_var_to_null() {
        Assert.raises(() -> {
            var _ : StdVector<Int> = null;
        });
    }

    function test_initialising_non_null_var_to_dynamic_null() {
        function get_null() : Any {
            return null;
        }

        Assert.raises(() -> {
            var _ : StdVector<Int> = get_null();
        });
    }

    /**
     * Value type function arguments can be re-assigned and the internal reference is still correct.
     */
    function test_argument_assignment() {
        final v = new StdVector<Int>();

        Assert.equals(10, argument_assign(v));
        Assert.equals(0, v.size());
    }

    function argument_assign(arg) {
        arg = new StdVector<Int>(10);

        return arg.size();
    }

    /**
     * Lamba functions which accept value types as arguments can be correctly re-assigned.
     */
    function test_lambda_argument_assignment() {
        final v = new StdVector<Int>();
        final f = arg -> {
            arg = new StdVector<Int>(10);

            return arg.size();
        }

        Assert.equals(10, f(v));
        Assert.equals(0, v.size());
    }

    /**
     * Passing a value type into a function will pass it by value.
     */
    function test_function_copying() {
        final v = new StdVector<Int>();

        by_value(v);

        Assert.equals( 0, v.size());
    }

    function by_value(v:StdVector<Int>) {
        v.resize(10);
    }

    /**
     * Variables captured by closures will be wrapped in a type which moves them to the GC heap.
     */
    function test_heap_promotion() {
        final v = new StdVector<Int>();
        final f = () -> {
            v.resize(10);
        }

        f();

        Assert.equals(10, v.size());
    }

    /**
     * Function arguments which are captured are promoted to retain consistent behaviour.
     */
    function test_promoting_function_argument() {
        final v = new StdVector<Int>();
        final s = promote_arg(v);

        Assert.equals(10, s.fst);
        Assert.equals(10, s.snd);
    }

    function promote_arg(v:StdVector<Int>) {
        final f = () -> {
            v.resize(10);

            return v.size();
        }
        final newSize = f();
        final oldSize = v.size();

        return { fst : oldSize, snd : newSize }
    }

    /**
     * Lambda arguments which are captured are promoted to retain consistent behaviour.
     */
    function test_promoting_lambda_argument() {
        final v = new StdVector<Int>();
        
        function lambda_promote_arg(v:StdVector<Int>) {
            final f = () -> {
                v.resize(10);

                return v.size();
            }

            final newSize = f();
            final oldSize = v.size();

            return { fst : oldSize, snd : newSize }
        }

        final s = lambda_promote_arg(v);

        Assert.equals(10, s.fst);
        Assert.equals(10, s.snd);
    }

    /**
     * Value type variables passed into a function of type dynamic will also be passed a copy.
     */
    function test_dynamic() {
        final v = new StdVector<Int>(5);

        by_value_dynamic(v);

        Assert.equals(5, v.size());
    }

    /**
     * Value type variables which have been promoted to the heap will be copied to dynamic functions argumens.
     */
    function test_promoted_dynamic() {
        final v = new StdVector<Int>();
        final f = () -> {
            v.resize(5);
        }

        f();

        by_value_dynamic(v);

        Assert.equals(5, v.size());
    }

    /**
     * Value type variables passed put into and accessed via an anonymous object will be promoted to the heap.
     */
    function test_anon() {
        final v = new StdVector<Int>(5);

        by_value_anon({ v : v });

        Assert.equals(5, v.size());
    }

    function by_value_anon(a:{ v : StdVector<Int> }) {
        Assert.equals(5, a.v.size());

        a.v.resize(10);

        Assert.equals(10, a.v.size());
    }

    /**
     * Value types which have been promoted can be re-assigned and the internal reference will remain valid.
     */
    function test_assigning_promoted_variable() {
        var v = new StdVector<Int>(5);
        final f = () -> {
            v.resize(10);
        }

        f();

        v = new StdVector<Int>(2);

        Assert.equals(2, v.size());
    }

    function test_lamba_returns() {
        final v1 = new StdVector<Int>(100);
        final f = () -> {
            return v1;
        }

        final v2 = f();

        v2.resize(50);

        Assert.equals( 50, v2.size());
        Assert.equals(100, v1.size());
    }

    function test_iterator() {
        final iter = new StdVectorIterator();

        var count = 0;
        for (vec in iter) {
            Assert.equals(count++, vec.size());
        }
    }

    function test_to_string() {
        final v = new StdVector<Int>();
        final s = Std.string(v);

        Assert.notNull(s);
    }

    function test_equality() {
        final v1 = new StdVector<Int>();
        final v2 = new StdVector<Int>();
        final eq = v1 == v2;

        Assert.isTrue(eq);
    }

    function test_inequality() {
        final v1 = new StdVector<Int>();
        final v2 = new StdVector<Int>();
        final eq = v1 != v2;

        Assert.isFalse(eq);
    }

    function test_just_creation() {
        new StdVector<Int>(10);

        Assert.pass();
    }

    /**
     * Array reading is allowed on value types implementing ArrayAccess.
     */
    // function test_array_access() {
    //     final v = new StdVector<Int>(5);

    //     // v[1] = 7;

    //     final i = v[1];

    //     Assert.equals(0, i);
    // }

    // Various helper functions

    function by_value_dynamic(v:Dynamic) {
        Assert.equals(5, (v:StdVector<Int>).size());

        (v:StdVector<Int>).resize(10);

        Assert.equals(10, (v:StdVector<Int>).size());
    }
}