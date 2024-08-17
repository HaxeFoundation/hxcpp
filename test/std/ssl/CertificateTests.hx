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

        Assert.isNull(cert.subject('QQ'));
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

        Assert.isNull(cert.issuer('QQ'));
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

        Assert.equals(2012, cert.notBefore.getFullYear());
        Assert.equals(7, cert.notBefore.getMonth());
        Assert.equals(22, cert.notBefore.getDate());

        Assert.equals(0, cert.notBefore.getSeconds());
        Assert.equals(28, cert.notBefore.getMinutes());
        Assert.equals(5, cert.notBefore.getHours());
    }

    function testAfter() {
        final cert = Certificate.fromString(Resource.getString('x509sample'));

        Assert.equals(2017, cert.notAfter.getFullYear());
        Assert.equals(7, cert.notAfter.getMonth());
        Assert.equals(21, cert.notAfter.getDate());

        Assert.equals(0, cert.notAfter.getSeconds());
        Assert.equals(28, cert.notAfter.getMinutes());
        Assert.equals(5, cert.notAfter.getHours());
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
