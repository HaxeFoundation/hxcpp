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

    @:depends(testStatus)
    function testBoolMemberStorage() {
        final cls = Type.resolveClass('ClientBoolField');

        if (Assert.notNull(cls, 'Unable to resolve ClientBoolField')) {
            final obj = Type.createInstance(cls, []);

            Assert.equals('true', Std.string(Reflect.field(obj, 'flag')), 'Member Bool did not read back as a boolean');
            Assert.equals('true', Std.string(Reflect.field(cls, 'staticFlag')), 'Static Bool did not read back as a boolean');
        }
    }

    @:depends(testStatus)
    function testBoolMemberFromScript() {
        final cls = Type.resolveClass('ClientBoolField');

        if (Assert.notNull(cls, 'Unable to resolve ClientBoolField')) {
            final obj = Type.createInstance(cls, []);

            Assert.equals(true, Reflect.callMethod(obj, Reflect.field(obj, 'readFlag'), []),
                'Script read of a true Bool member failed');
            Assert.equals(false, Reflect.callMethod(obj, Reflect.field(obj, 'readOffFlag'), []),
                'Script read of a false Bool member failed');
            Assert.equals('true', Reflect.callMethod(obj, Reflect.field(obj, 'flagToString'), []),
                'Bool member did not stringify as a boolean');
            Assert.equals(10, Reflect.callMethod(obj, Reflect.field(obj, 'branchOnFlag'), []),
                'Branch on a true Bool member took the wrong arm');
            Assert.equals(20, Reflect.callMethod(obj, Reflect.field(obj, 'branchOnOffFlag'), []),
                'Branch on a false Bool member took the wrong arm');
        }
    }

    @:depends(testStatus)
    function testBoolMemberWrite() {
        final cls = Type.resolveClass('ClientBoolField');

        if (Assert.notNull(cls, 'Unable to resolve ClientBoolField')) {
            final obj = Type.createInstance(cls, []);

            Assert.equals(false, Reflect.callMethod(obj, Reflect.field(obj, 'clearFlag'), []),
                'Script write of a Bool member did not stick');
            Assert.equals('false', Std.string(Reflect.field(obj, 'flag')),
                'Reflection did not see the script write');

            Reflect.setField(obj, 'flag', true);

            Assert.equals(true, Reflect.callMethod(obj, Reflect.field(obj, 'readFlag'), []),
                'Script did not see the reflection write');
            Assert.equals(10, Reflect.callMethod(obj, Reflect.field(obj, 'branchOnFlag'), []),
                'Branch did not see the reflection write');
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