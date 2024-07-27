package ssl;

import haxe.Resource;
import sys.ssl.Key;
import utest.Test;

import utest.Assert;

class TestPKCS1 extends Test {
    function testPemPublicKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs1_public_key'), true));
    }

    function testPemPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs1_private_key'), false));
    }

    function testAesEncryptedPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs1_aes_private_key'), false, 'password'));
    }

    function testTripleDesPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs1_private_3des_key'), false, 'password'));
    }
}
