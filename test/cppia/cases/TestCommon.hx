package cases;

import cpp.cppia.Host;
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

    function callBoolConst(name:String, args:Array<Dynamic>):Dynamic {
        final cls = Type.resolveClass('ClientBoolConst');

        if (!Assert.notNull(cls, 'Unable to resolve ClientBoolConst')) {
            return null;
        }

        return Reflect.callMethod(null, Reflect.field(cls, name), args);
    }

    @:depends(testStatus)
    function testBoolConstToDynamic() {
        Assert.equals('true', callBoolConst('constToDynamic', []),
            'Bool constant boxed as a number');
    }

    @:depends(testStatus)
    function testBoolConstInAnyArray() {
        final arr:Array<Any> = callBoolConst('anyArrayWithConsts', []);

        if (Assert.notNull(arr, 'No array returned')) {
            Assert.isTrue(Std.isOfType(arr[0], Bool), 'Bool constant did not store as a Bool');
            Assert.isTrue(Std.isOfType(arr[1], Bool), 'Bool constant did not store as a Bool');
            Assert.equals('true', Std.string(arr[0]), 'Bool constant stored in an Any array as a number');
            Assert.equals('false', Std.string(arr[1]), 'Bool constant stored in an Any array as a number');
        }
    }

    @:depends(testStatus)
    function testBoolComparisonInAnyArray() {
        final arr:Array<Any> = callBoolConst('anyArrayWithComparison', [3]);

        if (Assert.notNull(arr, 'No array returned')) {
            Assert.equals('true', Std.string(arr[0]), 'Bool comparison stored in an Any array as a number');
        }
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