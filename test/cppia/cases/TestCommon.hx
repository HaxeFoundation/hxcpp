package cases;

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

        Reflect.field(Type.resolveClass("Client"), "setupCommon")();
    }

    function testClientCalled() {
        Reflect.field(Type.resolveClass("Client"), "useHostImplementation")();

        Assert.equals(2, HostOne.called, 'No client implementation call');
    }

    function testClientImplementation() {
        if (Assert.notNull(Common.clientImplementation, 'No client implementation')) {
            Assert.equals(1, Common.clientImplementation.getOne(), 'Bad client Int implementation');
            Assert.equals('1', Common.clientImplementation.getOneString(), 'Bad client String implementation');
        }
    }

    function testResolvingScriptType() {
        var hostBase:HostBase = Type.createInstance(Type.resolveClass("ClientExtends2"),[]);
        if (Assert.notNull(hostBase, 'Failed to create client type')) {
            Assert.isTrue(hostBase.testUpdateOverride(), 'Bad update override');
        }
    }

    function testCallback() {
        Common.callback();

        Assert.equals(2, Common.callbackSet, 'Bad cppia closure');
    }

    function testInterfaceCalling() {
        final obj : IFoo = Type.createInstance(Type.resolveClass('ClientFoo'), []);

        if (Assert.notNull(obj, 'Unable to create client implementation')) {
            Assert.equals('foo', obj.baz());
        }
    }
}
