package ssl;

import utest.Assert;
import haxe.Resource;
import sys.ssl.Certificate;
import utest.Test;

class CertificateLoadingTests extends Test {
    function testFromString() {
        Certificate.fromString(Resource.getString('x509sample'));

        Assert.pass();
    }
}