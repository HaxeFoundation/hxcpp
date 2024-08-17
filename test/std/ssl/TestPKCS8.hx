package ssl;

import haxe.Resource;
import sys.ssl.Key;
import utest.Test;

import utest.Assert;

class TestPKCS8 extends Test {
    function testPemRsaPublicKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_rsa_public_key'), true));
    }

    function testPemEcPublicKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_ecdsa_public_key'), true));
    }

    function testPemRsaPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_rsa_private_key'), false));
    }

    function testPemEcPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_ecdsa_private_key'), false));
    }

    function testPemRsaAesEncryptedPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_rsa_aes_encrypted_private_key'), false, 'demo'));
    }

    function testPemRsa3desEncryptedPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_rsa_3des_encrypted_private_key'), false, 'demo'));
    }

    function testPemEcdsaAesEncryptedPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_ecdsa_aes_encrypted_private_key'), false, 'demo'));
    }

    function testPemEcdsa3desEncryptedPrivateKey() {
        Assert.notNull(Key.readPEM(Resource.getString('pkcs8_ecdsa_3des_encrypted_private_key'), false, 'demo'));
    }
}
