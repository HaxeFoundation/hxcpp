package cases;

import cpp.cppia.Host;
import cpp.vm.Gc;
import cpp.vm.WeakRef;
import utest.Test;
import utest.Assert;

private class HostOne implements pack.HostInterface
{
    public static var called = 0;

    public function new() {}

    public function getOne() : Int {
        called ++;
        return 1;
    }

    public function getOneString() : String {
        called++;
        return "1";
    }
}

class TestCommon extends Test {
    function setupClass() {  
        Common.hostImplementation = new HostOne();
    
        Common.callback = () -> Common.callbackSet = 1;

        Host.main();
    }

    function testStatus() {
        Assert.equals('ok', Common.status);
    }

    @:depends(testStatus)
    function testClientImplementation() {
        Assert.equals(2, HostOne.called, 'No client implementation call');

        if (Assert.notNull(Common.clientImplementation, 'No client implementation')) {
            Assert.equals(1, Common.clientImplementation.getOne(), 'Bad client Int implementation');
            Assert.equals('1', Common.clientImplementation.getOneString(), 'Bad client String implementation');
        }
    }

    @:depends(testStatus)
    function testResolvingScriptType() {
        var hostBase:HostBase = Type.createInstance(Type.resolveClass("ClientExtends2"),[]);
        if (Assert.notNull(hostBase, 'Failed to create client type')) {
            Assert.isTrue(hostBase.testUpdateOverride(), 'Bad update override');
        }
    }

    @:depends(testStatus)
    function testCallback() {
        Common.callback();
    
        Assert.equals(2, Common.callbackSet, 'Bad cppia closure');
    }

    @:depends(testStatus)
    function testSetFieldOfOldObject() {
        final holder = Type.createInstance(Type.resolveClass('ClientHolder'), []);

        // Full collections make the holder old: a generational collection no longer traverses it.
        for (_ in 0...3) {
            Gc.run(true);
        }
        final values = setValues(holder);
        Gc.run(false);

        Assert.notNull(values.get(), 'Freed a value set by name on an old object');
    }

    // Only a weak reference to the value leaves this frame, so that the stack does not keep it.
    static function setValues(holder:Dynamic):WeakRef<Array<Int>> {
        final values = [42];
        Reflect.setField(holder, 'values', values);
        return new WeakRef(values);
    }

    @:depends(testStatus)
    function testInterfaceCalling() {
        final obj : IFoo = Type.createInstance(Type.resolveClass('ClientFoo'), []);

        if (Assert.notNull(obj, 'Unable to create client implementation')) {
            Assert.equals('foo', obj.baz());
        }
    }

#if (haxe >= version("4.3.6"))
    @:depends(testStatus)
    function testMultiLevelInheritance() {
        if (Assert.notNull(Common.clientRoot, 'Null client root class')) {
            if (Assert.equals(3, Common.clientRoot.values.length, 'Expected three items in the array')) {
                Assert.equals(0, Common.clientRoot.values[0]);
                Assert.equals(1, Common.clientRoot.values[1]);
                Assert.equals(2, Common.clientRoot.values[2]);
            }
        }
    }
#end
}