package ssl;

import haxe.Resource;
import sys.ssl.Key;
import utest.Test;

import utest.Assert;

class TestPKCS8 extends Test {
    function testPemPublicKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_public_key'), true));
    }

    function testPemPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_private_key'), false));
    }

    function testPerAesEncryptedPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_aes_private_key'), false, 'password'));
    }
}
