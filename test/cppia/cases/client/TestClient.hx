package cases.client;

class TestClient extends Test {
	public static var clientBool0 = true;
	public static var clientBool1 = false;
	public static var clientBool2 = true;
	public static var clientBool3 = false;

	function testThread() {
		// See if it compiles
		Assert.notNull(sys.thread.Thread.current());
	}

	function testInterfaces() {
		var c = new ClientExtends();

		Assert.isTrue(c.ok(), "Bad client extension");

		Assert.equals("HostBase", c.whoStartedYou());
		Assert.equals("ClientExtends", c.whoOverridesYou());
		Assert.isTrue(c.testPointers());

		var hostInterface:IHostInterface = c;
		Assert.equals("HostBase", hostInterface.whoStartedYou(), "Bad interface fallthrough");
		Assert.equals("ClientExtends", hostInterface.whoOverridesYou(), "Bad interface override");
		Assert.equals("1two3", hostInterface.hostImplOnly(1, "two", 3), "Bad hostImplOnly implementation");

		Assert.isTrue(c.testOne(), "Bad ClientExtends getOne");

		var clientInterface:IClientInterface = c;
		Assert.equals("HostBase", clientInterface.whoStartedYou(), "Bad client interface fallthrough");
		Assert.equals("uniqueClientFunc", clientInterface.uniqueClientFunc(), "Bad new client interface call");
		Assert.equals("ClientExtends", clientInterface.whoOverridesYou(), "Bad client interface override");

		var clientHostInterface:IClientHostInterface = c;
		Assert.equals("HostBase", clientHostInterface.whoStartedYou(), "Bad client interface fallthrough");
		Assert.equals("ClientExtends", clientHostInterface.whoOverridesYou(), "Bad client interface override");
		Assert.equals("ClientExtends", clientHostInterface.whoAreYou(), "Bad client/host interface");

		var c:ClientIHostImpl = new ClientIHostImpl();
		Assert.equals("client", c.hostImplOnly(0, null, 0), "Trouble implementing host interface");
		Assert.equals("client", c.whoStartedYou(), "Trouble implementing host interface");
		Assert.equals("client", c.whoOverridesYou(), "Trouble implementing host interface");

		var c:ClientExtends = new ClientExtends2();
		Assert.equals(2, c.getGeneration(), "Error calling cppia super function");

		var c = new ClientExtends2();
		Assert.isFalse(c.testOne(), "ClientExtends2 getOne should fail");
		Assert.isTrue(c.testOneExtended());
		Assert.isTrue(c.testFour());
	}

	function testBools() {
		var hostBools = HostBase.hostBool0 + "/" + HostBase.hostBool1 + "/" + HostBase.hostBool2 + "/" + HostBase.hostBool3;
		var clientBools = clientBool0 + "/" + clientBool1 + "/" + clientBool2 + "/" + clientBool3;
		Assert.equals(hostBools, clientBools);
	}

	function testIssue926() {
		var x:Dynamic = 3;
		x *= 5;
		Assert.equals(x, 15);
	}

	function testIssue1257() {
		var x = 1290555;
		x *= 1290555;
		Assert.equals(x, -915102823);
	}
}
