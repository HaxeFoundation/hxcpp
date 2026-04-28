package tests;

import utest.Assert;
import utest.Test;
import cpp.Reference;

@:include('vector')
@:cpp.ValueType({ type : 'vector', namespace : [ 'std' ], flags : [ ImplicitConstruction ] })
private extern class StdVector<T> implements ArrayAccess<Reference<T>> {
    @:overload(function ():Void {})
    function new(s:Int):Void;

    function size():Int;

    function resize(s:Int):Void;
}

private extern class Helpers {
    @:native('vec_by_val')
    static function vec_by_val<T>(v:StdVector<T>):Void;

    @:native('vec_by_ref')
    static function vec_by_ref<T>(v:StdVector<T>):Void;

    @:native('vec_by_ptr')
    static function vec_by_ptr<T>(v:StdVector<T>):Void;
}

@:cppNamespaceCode('

void vec_by_val(std::vector<int> v)
{
    v.resize(v.size() * 2);
}

void vec_by_ref(std::vector<int>& v)
{
    v.resize(v.size() * 2);
}

void vec_by_ptr(std::vector<int>* v)
{
    v->resize(v->size() * 2);
}

')
class TestValueType extends Test {

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

    function test_var_asignment() {
        var v = new StdVector<Int>(50);

        v = new StdVector<Int>();

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

    function test_nullable() {
        var v : Null<StdVector<Int>> = null;

        var thrown = false;

        try {
            var _ = v.size();
        }
        catch (exn) {
            thrown = true;
        }

        Assert.isTrue(thrown);

        v = new StdVector(5);

        Assert.equals(5, v.size());
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


    function test_extern_by_val() {
        final v = new StdVector<Int>(5);

        Helpers.vec_by_val(v);

        Assert.equals(5, v.size());
    }

    function test_extern_by_ref() {
        final v = new StdVector<Int>(5);

        Helpers.vec_by_ref(v);

        Assert.equals(10, v.size());
    }

    function test_extern_by_ptr() {
        final v = new StdVector<Int>(5);

        Helpers.vec_by_ptr(v);

        Assert.equals(10, v.size());
    }

    function test_promoted_extern_by_val() {
        final v = new StdVector<Int>();
        final f = () -> {
            v.resize(5);
        }

        f();

        Helpers.vec_by_val(v);

        Assert.equals(5, v.size());
    }

    function test_promoted_extern_by_ref() {
        final v = new StdVector<Int>();
        final f = () -> {
            v.resize(5);
        }

        f();

        Helpers.vec_by_ref(v);

        Assert.equals(10, v.size());
    }

    function test_promoted_extern_by_ptr() {
        final v = new StdVector<Int>();
        final f = () -> {
            v.resize(5);
        }

        f();

        Helpers.vec_by_ptr(v);

        Assert.equals(10, v.size());
    }

    function test_asigning_promoted_variable() {
        var v = new StdVector<Int>(5);
        final f = () -> {
            v.resize(10);
        }

        f();

        v = new StdVector<Int>(2);

        Assert.equals(2, v.size());
    }

    //

    function by_value_anon(a:{ v : StdVector<Int> }) {
        Assert.equals(5, a.v.size());

        a.v.resize(10);

        Assert.equals(10, a.v.size());
    }

    function by_value_dynamic(v:Dynamic) {
        Assert.equals(5, (v:StdVector<Int>).size());

        (v:StdVector<Int>).resize(10);

        Assert.equals(10, (v:StdVector<Int>).size());
    }
}