package tests.marshalling.classes;

import cpp.Native;
import cpp.Star;
import cpp.Pointer;
import cpp.Reference;
import cpp.RawPointer;
import utest.Assert;
import utest.Test;

private extern class NativeFunctions {
    @:native('vec_by_val')
    static function vec_by_val(v:StdVector<Int>):Void;

    @:native('vec_by_ref')
    static function vec_by_ref(v:StdVector<Int>):Void;

    @:native('vec_by_ptr')
    static function vec_by_ptr(v:StdVector<Int>):Void;
}

private class HaxeFunctions {
    @:unreflective @:generic public static function get_size_doubled<T>(v : StdVector<T>) {
        return v.size() * 2;
    }

    @:unreflective @:generic public static function resize_vec_by_ref<T>(v : Reference<StdVector<T>>, size : Int) {
        v.resize(size);
    }

    @:unreflective @:generic public static function resize_vec_by_ptr<T>(v : Pointer<StdVector<T>>, size : Int) {
        v[0].resize(size);
    }

    @:unreflective @:generic public static function resize_vec_by_raw_ptr<T>(v : RawPointer<StdVector<T>>, size : Int) {
        v[0].resize(size);
    }

    @:unreflective @:generic public static function resize_vec_by_star<T>(v : Star<StdVector<T>>, size : Int) {
        v.resize(size);
    }
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

int* create_int() {
    return new int { 7 };
}

void int_ptr_ptr(int** ptr) {
    *ptr = new int { 14 };
}
')
class TestValueTypeInterop extends Test {
    function test_pass_straight_in() {
        Assert.equals(10, HaxeFunctions.get_size_doubled(new StdVector<Int>(5)));
    }

    /**
     * Extern functions which expect a type will be given a copy of the value type.
     */
    function test_extern_by_val() {
        final v = new StdVector<Int>(5);

        NativeFunctions.vec_by_val(v);

        Assert.equals(5, v.size());
    }

    /**
     * Extern functions which expect a reference to a type will be given a reference to the value type.
     */
    function test_extern_by_ref() {
        final v = new StdVector<Int>(5);

        NativeFunctions.vec_by_ref(v);

        Assert.equals(10, v.size());
    }

    /**
     * Extern functions which expect a pointer to a type will be given a pointer to the value type.
     */
    function test_extern_by_ptr() {
        final v = new StdVector<Int>(5);

        NativeFunctions.vec_by_ptr(v);

        Assert.equals(10, v.size());
    }

    /**
     * Extern functions which expect a type will be given a copy of the promoted value type.
     */
    function test_promoted_extern_by_val() {
        final v = new StdVector<Int>();
        final f = () -> {
            v.resize(5);
        }

        f();

        NativeFunctions.vec_by_val(v);

        Assert.equals(5, v.size());
    }

    /**
     * Extern functions which expect a reference to a type will be given a reference to the promoted value type.
     */
    function test_promoted_extern_by_ref() {
        final v = new StdVector<Int>();
        final f = () -> {
            v.resize(5);
        }

        f();

        NativeFunctions.vec_by_ref(v);

        Assert.equals(10, v.size());
    }

    /**
     * Extern functions which expect a pointer to a type will be given a pointer to the promoted value type.
     */
    function test_promoted_extern_by_ptr() {
        final v = new StdVector<Int>();
        final f = () -> {
            v.resize(5);
        }

        f();

        NativeFunctions.vec_by_ptr(v);

        Assert.equals(10, v.size());
    }

    function test_resize_vec_by_ref() {
        final v    = new StdVector<Int>();
        final size = 100;

        HaxeFunctions.resize_vec_by_ref(v, size);

        Assert.equals(size, v.size());
    }

    function test_resize_vec_by_ptr() {
        final v    = new StdVector<Int>();
        final size = 100;

        HaxeFunctions.resize_vec_by_ptr(Pointer.addressOf(v), size);

        Assert.equals(size, v.size());
    }

    function test_resize_vec_by_raw_ptr() {
        final v    = new StdVector<Int>();
        final size = 100;

        HaxeFunctions.resize_vec_by_raw_ptr(Pointer.addressOf(v).raw, size);

        Assert.equals(size, v.size());
    }

    function test_resize_vec_by_star() {
        final v    = new StdVector<Int>();
        final size = 100;

        HaxeFunctions.resize_vec_by_star(Pointer.addressOf(v).ptr, size);

        Assert.equals(size, v.size());
    }

    function test_copying_from_pointer() {
        final src  = new StdVector<Int>(10);
        final ptr  = Pointer.addressOf(src);
        final copy : StdVector<Int> = ptr.value;

        copy.resize(100);

        Assert.equals( 10, src.size());
        Assert.equals(100, copy.size());
    }

    function test_native_star() {
        final v   = new StdVector<Int>();
        final ptr = Native.addressOf(v);

        ptr.resize(10);

        Assert.equals(10, v.size());
    }

    function test_native_dereference() {
        final v   = new StdVector<Int>();
        final ptr = Pointer.addressOf(v);
        final ref = Native.star(ptr.ptr);

        ref.resize(10);

        Assert.equals(10, v.size());
    }

    function test_vec_of_points() {
        final v = new StdVector<Point>(5);

        Point.point_vec(v);

        Assert.equals(300f64, v.at(0).x);
    }
}