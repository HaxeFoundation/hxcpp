package ssl;

import sys.ssl.DigestAlgorithm;
import haxe.Resource;
import sys.ssl.Key;
import utest.Assert;
import sys.ssl.Digest;
import haxe.io.Bytes;
import utest.Test;

abstract class DigestTests extends Test {

    final hash : DigestAlgorithm;

    final input : Bytes;

    final expected : Bytes;

    final publicKey : String;

    final privateKey : String;

    function new(hash, expected, publicKey, privateKey) {
        super();

        this.input      = Bytes.ofString("Hello, World!");
        this.hash       = hash;
        this.expected   = expected;
        this.publicKey  = publicKey;
        this.privateKey = privateKey;
    }

    public function testHash() {
        Assert.equals(0, Digest.make(input, hash).compare(expected));
    }

    function testSign() {
        final key    = Key.readPEM(Resource.getString(privateKey), false);
        final result = Digest.sign(input, key, hash);

        if (Assert.notNull(result)) {
            Assert.isTrue(result.length > 0);
        }
    }

    function testVerify() {
        final privateKey = Key.readPEM(Resource.getString(privateKey), false);
        final publicKey  = Key.readPEM(Resource.getString(publicKey), true);
        final signature  = Digest.sign(input, privateKey, hash);

        Assert.isTrue(Digest.verify(input, signature, publicKey, hash));
    }

    function testVerifySameKey() {
        final privateKey = Key.readPEM(Resource.getString(privateKey), false);
        final signature  = Digest.sign(input, privateKey, hash);

        Assert.isTrue(Digest.verify(input, signature, privateKey, hash));
    }
}

abstract class MD5Tests extends DigestTests {

    public function new(publicKey, privateKey) {
        super(MD5, Bytes.ofHex('65a8e27d8879283831b664bd8b7f0ad4'), publicKey, privateKey);
    }
}

class RsaMD5Tests extends MD5Tests {
    public function new() {
        super('pkcs8_rsa_public_key', 'pkcs8_rsa_private_key');
    }
}

class EcdsaMD5Tests extends MD5Tests {
    public function new() {
        super('pkcs8_ecdsa_public_key', 'pkcs8_ecdsa_private_key');
    }
}

abstract class SHA1Tests extends DigestTests {

    public function new(publicKey, privateKey) {
        super(SHA1, Bytes.ofHex('0a0a9f2a6772942557ab5355d76af442f8f65e01'), publicKey, privateKey);
    }
}

class RsaSHA1Tests extends SHA1Tests {
    public function new() {
        super('pkcs8_rsa_public_key', 'pkcs8_rsa_private_key');
    }
}

class EcdsaSHA1Tests extends SHA1Tests {
    public function new() {
        super('pkcs8_ecdsa_public_key', 'pkcs8_ecdsa_private_key');
    }
}

abstract class SHA256Tests extends DigestTests {

    public function new(publicKey, privateKey) {
        super(SHA256, Bytes.ofHex('dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f'), publicKey, privateKey);
    }
}

class RsaSHA256Tests extends SHA256Tests {
    public function new() {
        super('pkcs8_rsa_public_key', 'pkcs8_rsa_private_key');
    }
}

class EcdsaSHA256Tests extends SHA256Tests {
    public function new() {
        super('pkcs8_ecdsa_public_key', 'pkcs8_ecdsa_private_key');
    }
}

abstract class SHA384Tests extends DigestTests {

    public function new(publicKey, privateKey) {
        super(SHA384, Bytes.ofHex('5485cc9b3365b4305dfb4e8337e0a598a574f8242bf17289e0dd6c20a3cd44a089de16ab4ab308f63e44b1170eb5f515'), publicKey, privateKey);
    }
}

class RsaSHA384Tests extends SHA384Tests {
    public function new() {
        super('pkcs8_rsa_public_key', 'pkcs8_rsa_private_key');
    }
}

class EcdsaSHA384Tests extends SHA384Tests {
    public function new() {
        super('pkcs8_ecdsa_public_key', 'pkcs8_ecdsa_private_key');
    }
}

abstract class SHA512Tests extends DigestTests {

    public function new(publicKey, privateKey) {
        super(SHA512, Bytes.ofHex('374d794a95cdcfd8b35993185fef9ba368f160d8daf432d08ba9f1ed1e5abe6cc69291e0fa2fe0006a52570ef18c19def4e617c33ce52ef0a6e5fbe318cb0387'), publicKey, privateKey);
    }
}

class RsaSHA512Tests extends SHA512Tests {
    public function new() {
        super('pkcs8_rsa_public_key', 'pkcs8_rsa_private_key');
    }
}

class EcdsaSHA512Tests extends SHA512Tests {
    public function new() {
        super('pkcs8_ecdsa_public_key', 'pkcs8_ecdsa_private_key');
    }
}
