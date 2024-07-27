package ssl;

import haxe.Resource;
import sys.ssl.Certificate;
import utest.Test;
import utest.Assert;

@:depends(ssl.CertificateLoadingTests)
class CertificateTests extends Test {
    function testCommonName() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('www.example.com', cert.commonName);
    }

    function testNonExistingSubject() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.exception(() -> cert.subject('QQ'));
    }

    function testSubjectCommonName() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('www.example.com', cert.subject('CN'));
    }

    function testSubjectOrganisation() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('Frank4DD', cert.subject('O'));
    }

    function testSubjectState() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('Tokyo', cert.subject('S'));
    }

    function testSubjectCountry() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('JP', cert.subject('C'));
    }

    function testNonExistingIssuer() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.exception(() -> cert.issuer('QQ'));
    }

    function testIssuerCommonName() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('Frank4DD Web CA', cert.issuer('CN'));
    }

    function testIssuerEmail() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('support@frank4dd.com', cert.issuer('E'));
    }

    function testIssuerOrganisation() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('Frank4DD', cert.issuer('O'));
    }

    function testIssuerOrganisationalUnit() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('WebCert Support', cert.issuer('OU'));
    }

    function testIssuerLocality() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('Chuo-ku', cert.issuer('L'));
    }

    function testIssuertState() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('Tokyo', cert.issuer('S'));
    }

    function testIssuerCountry() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals('JP', cert.issuer('C'));
    }

    function testBefore() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals(2012, cert.notBefore.getUTCFullYear());
        Assert.equals(7, cert.notBefore.getUTCMonth());
        Assert.equals(22, cert.notBefore.getUTCDate());

        Assert.equals(0, cert.notBefore.getUTCSeconds());
        Assert.equals(28, cert.notBefore.getUTCMinutes());
        Assert.equals(6, cert.notBefore.getUTCHours());
    }

    function testAfter() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals(2017, cert.notAfter.getUTCFullYear());
        Assert.equals(7, cert.notAfter.getUTCMonth());
        Assert.equals(21, cert.notAfter.getUTCDate());

        Assert.equals(0, cert.notAfter.getUTCSeconds());
        Assert.equals(28, cert.notAfter.getUTCMinutes());
        Assert.equals(6, cert.notAfter.getUTCHours());
    }

    function testNext() {
        // var count = 0;
        // var cert  = Certificate.fromString(Resource.getString('chain'));

        // do {
        //     trace(cert.commonName);
        // } while (null != (cert = cert.next()));

        Assert.pass();
    }
}
