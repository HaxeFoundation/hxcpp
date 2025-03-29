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

    function testPemPrivateEncryptedKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs1_private_aes_encrypted_key'), false, 'demo'));
    }

    function testPemPrivateEncryptedKey3DES() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs1_private_3des_encrypted_key'), false, 'testpassword'));
    }
}
