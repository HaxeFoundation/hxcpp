package tests.marshalling.pointers;

import utest.Assert;
import utest.Test;

class Foo {
    public static var ptr_static = Context.create();

    public var v : Context;

    public function new() {
        v = Context.create();
    }

    public function get() {
        return v;
    }

    public function mutate(size : Int) {
        return v.number = size;
    }

    public dynamic function pass_through(ctx : Context) {
        return ctx;
    }
}

class WithConstruction {
    public final v : Context;

    public function new(v) {
        this.v = v;
    }
}

class TestClassPointers extends Test {
    
    // Member Variables

    function test_class_field_access() {
        final f = new Foo();

        Assert.equals(7, f.v.number);
    }

    function test_mutating_class_field() {
        final f = new Foo();

        f.v.number = 20;

        Assert.equals(20, f.v.number);
    }

    function test_class_field_copy() {
        final f = new Foo();
        final p = f.v;

        p.number = 20;

        Assert.equals(20, f.v.number);
        Assert.equals(20, p.number);
    }

    function test_class_field_assignment() {
        final f = new Foo();
        final p = Context.create();

        p.number = 20;

        f.v = p;

        Assert.isTrue(p == f.v);
        Assert.equals(20, f.v.number);
    }

    function test_class_reflect_variable() {
        final f = new Foo();
        final n = (Reflect.field(f, 'v') : Context).number;

        Assert.equals(7, n);
    }

    function test_class_reflect_variable_mutation() {
        final f = new Foo();

        (Reflect.field(f, 'v') : Context).number = 20;

        Assert.equals(20, f.v.number);
    }

    function test_class_reflect_variable_assignment() {
        final f = new Foo();
        final p = Context.create();

        p.number = 20;

        Reflect.setField(f, 'v', p);

        Assert.isTrue(p == f.v);
        Assert.equals(20, f.v.number);
    }

    // Static Variables

    function test_static_variable_mutation() {
        Foo.ptr_static.number = 20;

        Assert.equals(20, Foo.ptr_static.number);
    }

    function test_static_variable_assignment() {
        final ptr = Context.create();
        final old = Foo.ptr_static;

        Foo.ptr_static = ptr;
        
        Assert.isTrue(Foo.ptr_static == ptr);
        Assert.isTrue(Foo.ptr_static != old);
    }

    function test_static_variable_copy() {
        final c = Foo.ptr_static;
        
        c.number = 75;

        Assert.equals(75, c.number);
        Assert.equals(75, Foo.ptr_static.number);
    }

    function test_static_class_reflect_variable() {
        Foo.ptr_static.number = 11;
      
        final s = (Reflect.field(Foo, 'ptr_static') : Context).number;
        
        Assert.equals(11, s);
    }

    function test_static_class_reflect_variable_mutation() {
        (Reflect.field(Foo, 'ptr_static') : Context).number = 12;
        
        Assert.equals(12, Foo.ptr_static.number);
    }

    function test_static_class_reflect_variable_assignment() {
        final ptr = Context.create();
        final old = Foo.ptr_static;

        Reflect.setField(Foo, 'ptr_static', ptr);

        Assert.isTrue(Foo.ptr_static == ptr);
        Assert.isTrue(Foo.ptr_static != old);
    }

    // Member Functions

    function test_class_function_call() {
        final f = new Foo();

        Assert.equals(14, f.v.double());
    }

    function test_class_function_pass() {
        final f = new Foo();
        final s = f.mutate(20);

        Assert.equals(20, f.v.number);
    }

    function test_member_dynamic_function() {
        final f = new Foo();
        final p = Context.create();
        final v = f.pass_through(p);

        Assert.isTrue(p == v);
    }

    // Construction

    function test_constructor() {
        final v = Context.create();
        final o = new WithConstruction(v);

        o.v.number = 20;

        Assert.equals(20, o.v.number);
        Assert.equals(20, v.number);
    }

    function test_reflection_create_empty() {
        final f : WithConstruction = Type.createEmptyInstance(WithConstruction);

        Assert.isTrue(f.v == null);
    }

    function test_reflection_create() {
        final f : WithConstruction = Type.createInstance(WithConstruction, [ Context.create() ]);

        Assert.equals(7, f.v.number);
    }
}