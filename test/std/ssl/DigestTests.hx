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

    function new(hash, expected) {
        super();

        this.input    = Bytes.ofString("Hello, World!");
        this.hash     = hash;
        this.expected = expected;
    }

    public function testHash() {
        Assert.equals(0, Digest.make(input, hash).compare(expected));
    }

    function testSign() {
        final key    = Key.readPEM(Resource.getString('pkcs8_private_key'), false);
        final result = Digest.sign(input, key, hash);

        if (Assert.notNull(result)) {
            Assert.isTrue(result.length > 0);
        }
    }

    function testVerify() {
        final privateKey = Key.readPEM(Resource.getString('pkcs8_private_key'), false);
        final publicKey  = Key.readPEM(Resource.getString('pkcs8_public_key'), true);
        final signature  = Digest.sign(input, privateKey, hash);

        Assert.isTrue(Digest.verify(input, signature, publicKey, hash));
    }

    function testVerifySameKey() {
        final privateKey = Key.readPEM(Resource.getString('pkcs8_private_key'), false);
        final signature  = Digest.sign(input, privateKey, hash);

        Assert.isTrue(Digest.verify(input, signature, privateKey, hash));
    }
}

class MD5Tests extends DigestTests {

    public function new() {
        super(MD5, Bytes.ofHex('65a8e27d8879283831b664bd8b7f0ad4'));
    }
}

class SHA1Tests extends DigestTests {

    public function new() {
        super(SHA1, Bytes.ofHex('0a0a9f2a6772942557ab5355d76af442f8f65e01'));
    }
}

class SHA256Tests extends DigestTests {

    public function new() {
        super(SHA256, Bytes.ofHex('dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f'));
    }
}

class SHA384Tests extends DigestTests {

    public function new() {
        super(SHA384, Bytes.ofHex('5485cc9b3365b4305dfb4e8337e0a598a574f8242bf17289e0dd6c20a3cd44a089de16ab4ab308f63e44b1170eb5f515'));
    }
}

class SHA512Tests extends DigestTests {

    public function new() {
        super(SHA512, Bytes.ofHex('374d794a95cdcfd8b35993185fef9ba368f160d8daf432d08ba9f1ed1e5abe6cc69291e0fa2fe0006a52570ef18c19def4e617c33ce52ef0a6e5fbe318cb0387'));
    }
}

class RIPEMD160Tests extends DigestTests {
    public function new() {
        super(RIPEMD160, Bytes.ofHex('527a6a4b9a6da75607546842e0e00105350b1aaf'));
    }
}
