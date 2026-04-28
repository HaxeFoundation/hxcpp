package tests.marshalling.classes;

import utest.Assert;
import utest.Test;

class Foo {
    public static var v_static = new StdVector<Int>(50);

    public var v : StdVector<Int>;

    public function new() {
        v = new StdVector(7);
    }

    public function get() {
        return v;
    }

    public function multiply(v:StdVector<Int>) {
        v.resize(v.size() * 2);

        return v.size();
    }

    public dynamic function pass_through(v:StdVector<Int>) {
        return v;
    }

    public static function get_random(max:Int) {
        return new StdVector<Int>(max + Std.random(max));
    }
}

class WithConstruction {
    public final v : StdVector<Int>;

    public function new(v) {
        this.v = v;
    }
}

class TestClassValueType extends Test {

    // Member Variables

    function test_class_field_access() {
        final f = new Foo();

        Assert.equals(7, f.v.size());
    }

    function test_mutating_class_field() {
        final f = new Foo();

        f.v.resize(100);

        Assert.equals(100, f.v.size());
    }

    function test_class_field_copy() {
        final f = new Foo();
        final v = f.v;

        v.resize(100);

        Assert.equals(100, v.size());
        Assert.equals(7, f.v.size());
    }

    function test_class_field_assignment() {
        final f = new Foo();

        f.v = new StdVector(100);

        Assert.equals(100, f.v.size());
    }

    function test_class_reflect_variable() {
        final f = new Foo();
        final s = (Reflect.field(f, 'v') : StdVector<Int>).size();
        
        Assert.equals(7, s);
    }

    function test_class_reflect_variable_mutation() {
        final f = new Foo();

        (Reflect.field(f, 'v') : StdVector<Int>).resize(100);
        
        Assert.equals(100, f.v.size());
    }

    function test_class_reflect_variable_assignment() {
        final f = new Foo();

        Reflect.setField(f, 'v', new StdVector<Int>(100));

        Assert.equals(100, f.v.size());
    }

    // Static Variables

    function test_static_variable_mutation() {
        Foo.v_static.resize(100);

        Assert.equals(100, Foo.v_static.size());
    }

    function test_static_variable_assignment() {
        Foo.v_static = new StdVector<Int>(25);
        
        Assert.equals(25, Foo.v_static.size());
    }

    function test_static_variable_copy() {
        final v = Foo.v_static;
        
        v.resize(75);

        Assert.equals(75, v.size());
        Assert.notEquals(75, Foo.v_static.size());
    }

    function test_static_class_reflect_variable() {
        Foo.v_static.resize(11);
      
        final s = (Reflect.field(Foo, 'v_static') : StdVector<Int>).size();
        
        Assert.equals(11, s);
    }

    function test_static_class_reflect_variable_mutation() {
        (Reflect.field(Foo, 'v_static') : StdVector<Int>).resize(12);
        
        Assert.equals(12, Foo.v_static.size());
    }

    function test_static_class_reflect_variable_assignment() {
        Reflect.setField(Foo, 'v_static', new StdVector<Int>(13));

        Assert.equals(13, Foo.v_static.size());
    }

    // Member Functions

    function test_class_function_copy() {
        final f = new Foo();
        final v = f.get();

        v.resize(100);

        Assert.equals(100, v.size());
        Assert.equals(7, f.v.size());
    }

    function test_class_function_pass() {
        final f = new Foo();
        final v = new StdVector<Int>(7);
        final s = f.multiply(v);

        Assert.equals(7, v.size());
        Assert.equals(14, s);
    }
    
    function test_class_function_call() {
        final f = new Foo();
        final s = f.get().size();

        Assert.equals(7, s);
    }

    function test_member_dynamic_function() {
        final f = new Foo();
        final v = f.pass_through(new StdVector<Int>(10));

        Assert.equals(10, v.size());
    }

    // Static Functions

    function test_static_function() {
        Assert.equals(true, Foo.get_random(100).size() >= 100);
    }

    // Construction

    function test_constructor() {
        final v = new StdVector<Int>(7);
        final o = new WithConstruction(v);

        o.v.resize(100);

        Assert.equals(100, o.v.size());
        Assert.equals(7, v.size());
    }

    function test_reflection_create_empty() {
        final f : WithConstruction = Type.createEmptyInstance(WithConstruction);

        Assert.isNull(f.v);
    }

    function test_reflection_create() {
        final f : WithConstruction = Type.createInstance(WithConstruction, [ new StdVector<Int>(7) ]);

        Assert.equals(7, f.v.size());
    }
}