#include <string.h>

#ifdef HX_WINDOWS
#   include <winsock2.h>
#   include <wincrypt.h>
#else
#   include <sys/socket.h>
#   include <strings.h>
#   include <errno.h>
typedef int SOCKET;
#endif


#include <hxcpp.h>
#include <hx/OS.h>

#if defined(NEKO_MAC) && !defined(IPHONE) && !defined(APPLETV)
#include <Security/Security.h>
#endif

typedef size_t socket_int;

#define SOCKET_ERROR (-1)
#define NRETRYS	20

#include "mbedtls/error.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md.h"
#include "mbedtls/pk.h"
#include "mbedtls/oid.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"

#define val_ssl(o)	((sslctx*)o.mPtr)
#define val_conf(o)	((sslconf*)o.mPtr)
#define val_cert(o) ((sslcert*)o.mPtr)
#define val_pkey(o) ((sslpkey*)o.mPtr)

struct SocketWrapper : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdSocket };
   SOCKET socket;
};

struct sslctx : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdSsl };

	mbedtls_ssl_context *s;

	void create()
	{
		s = (mbedtls_ssl_context *)malloc(sizeof(mbedtls_ssl_context));
		mbedtls_ssl_init(s);
		_hx_set_finalizer(this, finalize);
	}

	void destroy()
	{
		if( s )
		{
			mbedtls_ssl_free( s );
			free(s);
			s = 0;
		}
	}

	static void finalize(Dynamic obj)
	{
		((sslctx *)(obj.mPtr))->destroy();
	}

	String toString() { return HX_CSTRING("sslctx"); }
};

struct sslconf : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdSslConf };

	mbedtls_ssl_config *c;

	void create()
	{
		c = (mbedtls_ssl_config *)malloc(sizeof(mbedtls_ssl_config));
		mbedtls_ssl_config_init(c);
		_hx_set_finalizer(this, finalize);
	}

	void destroy()
	{
		if( c )
		{
			mbedtls_ssl_config_free( c );
			free(c);
			c = 0;
		}
	}

	static void finalize(Dynamic obj)
	{
		((sslconf *)(obj.mPtr))->destroy();
	}

	String toString() { return HX_CSTRING("sslconfig"); }
};

struct sslcert : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdSslCert };

	mbedtls_x509_crt *c;
	bool head;

	void create(const mbedtls_x509_crt *inC)
	{

		if( inC ){
			c = (mbedtls_x509_crt *)inC;
			head = false;
		}else{
			c = (mbedtls_x509_crt *)malloc(sizeof(mbedtls_x509_crt));
			mbedtls_x509_crt_init(c);
			head = true;
		}
		_hx_set_finalizer(this, finalize);
	}

	void destroy()
	{
		if( c && head )
		{
			mbedtls_x509_crt_free( c );
			free(c);
			head = 0;
		}
		c = 0;
	}

	static void finalize(Dynamic obj)
	{
		((sslcert *)(obj.mPtr))->destroy();
	}

	String toString() { return HX_CSTRING("sslcert"); }
};

struct sslpkey : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdSslKey };

	mbedtls_pk_context *k;

	void create()
	{
		k = (mbedtls_pk_context *)malloc(sizeof(mbedtls_pk_context));
		mbedtls_pk_init(k);
		_hx_set_finalizer(this, finalize);
	}

	void destroy()
	{
		if( k )
		{
			mbedtls_pk_free(k);
			free(k);
			k = 0;
		}
	}

	static void finalize(Dynamic obj)
	{
		((sslpkey *)(obj.mPtr))->destroy();
	}

	String toString() { return HX_CSTRING("sslpkey"); }
};

static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;


static void block_error() {
	#ifdef NEKO_WINDOWS
	int err = WSAGetLastError();
	if( err == WSAEWOULDBLOCK || err == WSAEALREADY || err == WSAETIMEDOUT )
	#else
	if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EALREADY )
	#endif
		hx::Throw(HX_CSTRING("Blocking"));
	hx::Throw(HX_CSTRING("ssl network error"));
}

static void ssl_error( int ret ){
	char buf[256];
	mbedtls_strerror(ret, buf, sizeof(buf));
	hx::Throw( String(buf) );
}

Dynamic _hx_ssl_new( Dynamic hconf ) {
	int ret;
	sslctx *ssl = new sslctx();
	ssl->create();
	sslconf *conf = val_conf(hconf);
	if( ret = mbedtls_ssl_setup(ssl->s, conf->c) != 0 ){
		ssl->destroy();
		ssl_error(ret);
	}
	return ssl;
}

void _hx_ssl_close( Dynamic hssl ) {
	sslctx *ssl = val_ssl(hssl);
	ssl->destroy();
}

void _hx_ssl_debug_set (int i) {
	mbedtls_debug_set_threshold(i);
}

void _hx_ssl_handshake( Dynamic hssl ) {
	int r;
	sslctx *ssl = val_ssl(hssl);
	POSIX_LABEL(handshake_again);
	r = mbedtls_ssl_handshake( ssl->s );
	if( r == SOCKET_ERROR ) {
		HANDLE_EINTR(handshake_again);
		block_error();
	}else if( r != 0 )
		ssl_error(r);
}

int net_read( void *fd, unsigned char *buf, size_t len ){
	hx::EnterGCFreeZone();
	int r = recv((SOCKET)(socket_int)fd, (char *)buf, len, 0);
	hx::ExitGCFreeZone();
	return r;
}

int net_write( void *fd, const unsigned char *buf, size_t len ){
	hx::EnterGCFreeZone();
	int r = send((SOCKET)(socket_int)fd, (char *)buf, len, 0);
	hx::ExitGCFreeZone();
	return r;
}

void _hx_ssl_set_socket( Dynamic hssl, Dynamic hsocket ) {
	sslctx *ssl = val_ssl(hssl);
	SocketWrapper *socket = (SocketWrapper *)hsocket.mPtr;
	mbedtls_ssl_set_bio( ssl->s, (void *)(socket_int)socket->socket, net_write, net_read, NULL );
}

void _hx_ssl_set_hostname( Dynamic hssl, String hostname ){
	int ret;
	sslctx *ssl = val_ssl(hssl);
	hx::strbuf buf;
	if( ret = mbedtls_ssl_set_hostname(ssl->s, hostname.utf8_str(&buf)) != 0 )
		ssl_error(ret);
}

Dynamic _hx_ssl_get_peer_certificate( Dynamic hssl ){
	sslctx *ssl = val_ssl(hssl);
	const mbedtls_x509_crt *crt = mbedtls_ssl_get_peer_cert(ssl->s);
	if( crt == NULL )
		return null();
	sslcert *cert = new sslcert();
	cert->create( crt );
	return cert;
}

bool _hx_ssl_get_verify_result( Dynamic hssl ){
	sslctx *ssl = val_ssl(hssl);
	int r = mbedtls_ssl_get_verify_result( ssl->s );
	if( r == 0 )
		return true;
	else if( r != -1 )
		return false;
	hx::Throw( HX_CSTRING("not available") );
	return false;
}

void _hx_ssl_send_char( Dynamic hssl, int c ) {
	if( c < 0 || c > 255 )
		hx::Throw( HX_CSTRING("invalid char") );
	sslctx *ssl = val_ssl(hssl);
	const unsigned char cc = c;
	mbedtls_ssl_write( ssl->s, &cc, 1 );
}

int _hx_ssl_send( Dynamic hssl, Array<unsigned char> buf, int p, int l ) {
	sslctx *ssl = val_ssl(hssl);
	int dlen = buf->length;
	if( p < 0 || l < 0 || p > dlen || p + l > dlen )
		hx::Throw( HX_CSTRING("ssl_send") );
	POSIX_LABEL(send_again);
	const unsigned char *base = (const unsigned char *)&buf[0];
	dlen = mbedtls_ssl_write( ssl->s, base + p, l );
	if( dlen == SOCKET_ERROR ) {
		HANDLE_EINTR(send_again);
		block_error();
	}
	return dlen;
}

void _hx_ssl_write( Dynamic hssl, Array<unsigned char> buf ) {
	sslctx *ssl = val_ssl(hssl);
	int len = buf->length;
	unsigned char *cdata = &buf[0];
	while( len > 0 ) {
		POSIX_LABEL( write_again );
		int slen = mbedtls_ssl_write( ssl->s, cdata, len );
		if( slen == SOCKET_ERROR ) {
			HANDLE_EINTR( write_again );
			block_error();
		}
		cdata += slen;
		len -= slen;
	}
}

int _hx_ssl_recv_char( Dynamic hssl ) {
	sslctx *ssl = val_ssl(hssl);
	unsigned char cc;
	int r = mbedtls_ssl_read( ssl->s, &cc, 1 );
	if( r <= 0 )
		hx::Throw( HX_CSTRING("ssl_recv_char") );
	return (int)cc;
}

int _hx_ssl_recv( Dynamic hssl, Array<unsigned char> buf, int p, int l ) {
	sslctx *ssl = val_ssl(hssl);
	int dlen = buf->length;
	if( p < 0 || l < 0 || p > dlen || p + l > dlen )
		hx::Throw( HX_CSTRING("ssl_recv") );

	unsigned char *base = &buf[0];
	POSIX_LABEL(recv_again);
	dlen = mbedtls_ssl_read( ssl->s, base + p, l );
	if( dlen == SOCKET_ERROR ) {
		HANDLE_EINTR(recv_again);
		block_error();
	}
	if( dlen < 0 ) {  
                if( dlen == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ) {
                  mbedtls_ssl_close_notify( ssl->s );
	  	  return 0;
                }
		hx::Throw( HX_CSTRING("ssl_recv") ); 
	}
	return dlen;
}

Array<unsigned char> _hx_ssl_read( Dynamic hssl ) {
	sslctx *ssl = val_ssl(hssl);
	Array<unsigned char> result = Array_obj<unsigned char>::__new();
	unsigned char buf[256];

	while( true ) {
		POSIX_LABEL(read_again);
		int len = mbedtls_ssl_read( ssl->s, buf, 256 );
		if( len == SOCKET_ERROR ) {
			HANDLE_EINTR(read_again);
			block_error();
		}
                if( len == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ) {
                  mbedtls_ssl_close_notify( ssl->s );
	  	  len = 0;
                }
		if( len == 0 )
			break;
		result->memcpy(result->length, buf, len);
	}
	return result;
}

Dynamic _hx_ssl_conf_new( bool server ) {
	int ret;
	sslconf *conf = new sslconf();
	conf->create();
	if( ret = mbedtls_ssl_config_defaults( conf->c,
		server ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
		MBEDTLS_SSL_TRANSPORT_STREAM, 0 ) != 0 ){
		conf->destroy();
		ssl_error( ret );
	}
	mbedtls_ssl_conf_rng( conf->c, mbedtls_ctr_drbg_random, &ctr_drbg );
	return conf;
}

void _hx_ssl_conf_close( Dynamic hconf ) {
	sslconf *conf = val_conf(hconf);
	conf->destroy();
}

void _hx_ssl_conf_set_ca( Dynamic hconf, Dynamic hcert ) {
	sslconf *conf = val_conf(hconf);
	if( hconf.mPtr ){
		sslcert *cert = val_cert(hcert);
		mbedtls_ssl_conf_ca_chain( conf->c, cert->c, NULL );
	}else{
		mbedtls_ssl_conf_ca_chain( conf->c, NULL, NULL );
	}
}

void _hx_ssl_conf_set_verify( Dynamic hconf, int mode ) {
	sslconf *conf = val_conf(hconf);
	if( mode == 2 )
		mbedtls_ssl_conf_authmode(conf->c, MBEDTLS_SSL_VERIFY_OPTIONAL);
	else if( mode == 1 )
		mbedtls_ssl_conf_authmode(conf->c, MBEDTLS_SSL_VERIFY_REQUIRED);
	else
		mbedtls_ssl_conf_authmode(conf->c, MBEDTLS_SSL_VERIFY_NONE);
}

void _hx_ssl_conf_set_cert( Dynamic hconf, Dynamic hcert, Dynamic hpkey ) {
	int r;
	sslconf *conf = val_conf(hconf);
	sslcert *cert = val_cert(hcert);
	sslpkey *pkey = val_pkey(hpkey);

	if( r = mbedtls_ssl_conf_own_cert(conf->c, cert->c, pkey->k) != 0 )
		ssl_error(r);
}

static int sni_callback( void *arg, mbedtls_ssl_context *ctx, const unsigned char *name, size_t len ){
	if( name && arg ){
		Dynamic cb = new Dynamic();
		cb.mPtr = (hx::Object*)arg;
		const char *n = (const char *)name;
		Dynamic ret = cb->__run( String(n,strlen(n)) );
		if( ret != null() ){
			// TODO authmode and ca
			Dynamic hcert = ret->__Field(HX_CSTRING("cert"), hx::paccDynamic);
			Dynamic hpkey = ret->__Field(HX_CSTRING("key"), hx::paccDynamic);
			sslcert *cert = val_cert(hcert);
			sslpkey *pk = val_pkey(hpkey);

			return mbedtls_ssl_set_hs_own_cert( ctx, cert->c, pk->k );
		}
	}
	return -1;
}

void _hx_ssl_conf_set_servername_callback( Dynamic hconf, Dynamic cb ){
	sslconf *conf = val_conf(hconf);
	mbedtls_ssl_conf_sni( conf->c, sni_callback, (void *)cb.mPtr );
}

Dynamic _hx_ssl_cert_load_defaults(){
#if defined(NEKO_WINDOWS)
	HCERTSTORE store;
	PCCERT_CONTEXT cert;
	sslcert *chain = NULL;
	if( store = CertOpenSystemStore(0, (LPCSTR)"Root") ){
		cert = NULL;
		while( cert = CertEnumCertificatesInStore(store, cert) ){
			if( chain == NULL ){
				chain = new sslcert();
				chain->create( NULL );
			}
			mbedtls_x509_crt_parse_der( chain->c, (unsigned char *)cert->pbCertEncoded, cert->cbCertEncoded );
		}
		CertCloseStore(store, 0);
	}
	if( chain != NULL )
		return chain;
#elif defined(NEKO_MAC) && !defined(IPHONE) && !defined(APPLETV)
	CFMutableDictionaryRef search;
	CFArrayRef result;
	SecKeychainRef keychain;
	SecCertificateRef item;
	CFDataRef dat;
	sslcert *chain = NULL;

	// Load keychain
	if( SecKeychainOpen("/System/Library/Keychains/SystemRootCertificates.keychain",&keychain) != errSecSuccess )
		return null();

	// Search for certificates
	search = CFDictionaryCreateMutable( NULL, 0, NULL, NULL );
	CFDictionarySetValue( search, kSecClass, kSecClassCertificate );
	CFDictionarySetValue( search, kSecMatchLimit, kSecMatchLimitAll );
	CFDictionarySetValue( search, kSecReturnRef, kCFBooleanTrue );
	CFDictionarySetValue( search, kSecMatchSearchList, CFArrayCreate(NULL, (const void **)&keychain, 1, NULL) );
	if( SecItemCopyMatching( search, (CFTypeRef *)&result ) == errSecSuccess ){
		CFIndex n = CFArrayGetCount( result );
		for( CFIndex i = 0; i < n; i++ ){
			item = (SecCertificateRef)CFArrayGetValueAtIndex( result, i );

			// Get certificate in DER format
			dat = SecCertificateCopyData( item );
			if( dat ){
				if( chain == NULL ){
					chain = new sslcert();
					chain->create( NULL );
				}
				mbedtls_x509_crt_parse_der( chain->c, (unsigned char *)CFDataGetBytePtr(dat), CFDataGetLength(dat) );
				CFRelease( dat );
			}
		}
	}
	CFRelease(keychain);
	if( chain != NULL )
		return chain;
#endif
	return null();
}

Dynamic _hx_ssl_cert_load_file( String file ){
	int r;
	sslcert *cert = new sslcert();
	cert->create( NULL );
   hx::strbuf buf;
	if( r = mbedtls_x509_crt_parse_file(cert->c, file.utf8_str(&buf)) != 0 ){
		cert->destroy();
		ssl_error(r);
	}
	return cert;
}

Dynamic _hx_ssl_cert_load_path( String path ){
	int r;
	sslcert *cert = new sslcert();
	cert->create( NULL );
   hx::strbuf buf;
	if( r = mbedtls_x509_crt_parse_path(cert->c, path.utf8_str(&buf)) != 0 ){
		cert->destroy();
		ssl_error(r);
	}
	return cert;
}

static String asn1_buf_to_string( mbedtls_asn1_buf *dat ){
	unsigned int i, c;
	HX_CHAR *result = hx::NewString( dat->len );
	for( i = 0; i < dat->len; i++ )  {
        c = dat->p[i];
        if( c < 32 || c == 127 || ( c > 128 && c < 160 ) )
             result[i] = '?';
        else
			result[i] = c;
    }
	result[i] = '\0';
	return String(result,dat->len);
}

String _hx_ssl_cert_get_subject( Dynamic hcert, String objname ){
	mbedtls_x509_name *obj;
	int r;
	const char *oname;
	sslcert *cert = val_cert(hcert);
	obj = &cert->c->subject;
	if( obj == NULL )
		hx::Throw( HX_CSTRING("cert_get_subject") );
   hx::strbuf buf;
	while( obj != NULL ){
		r = mbedtls_oid_get_attr_short_name( &obj->oid, &oname );
		if( r == 0 && strcmp( oname, objname.utf8_str(&buf) ) == 0 )
			return asn1_buf_to_string( &obj->val );
		obj = obj->next;
	}
	return String();
}

String _hx_ssl_cert_get_issuer( Dynamic hcert, String objname ){
	mbedtls_x509_name *obj;
	int r;
	const char *oname;
	sslcert *cert = val_cert(hcert);
	obj = &cert->c->issuer;
	if( obj == NULL )
		hx::Throw( HX_CSTRING("cert_get_issuer") );
   hx::strbuf buf;
	while( obj != NULL ){
		r = mbedtls_oid_get_attr_short_name( &obj->oid, &oname );
		if( r == 0 && strcmp( oname, objname.utf8_str(&buf) ) == 0 )
			return asn1_buf_to_string( &obj->val );
		obj = obj->next;
	}
	return String();
}

Array<String> _hx_ssl_cert_get_altnames( Dynamic hcert ){
	sslcert *cert = val_cert(hcert);
	mbedtls_asn1_sequence *cur;
	Array<String> result(0,1);
	if( cert->c->ext_types & MBEDTLS_X509_EXT_SUBJECT_ALT_NAME ){
		cur = &cert->c->subject_alt_names;

		while( cur != NULL ){
			result.Add( asn1_buf_to_string(&cur->buf) );
			cur = cur->next;
		}
	}
	return result;
}

static Array<int> x509_time_to_array( mbedtls_x509_time *t ){
	if( !t )
		hx::Throw( HX_CSTRING("x509_time_to_array") );
	Array<int> result(6,6);
	result[0] = t->year;
	result[1] = t->mon;
	result[2] = t->day;
	result[3] = t->hour;
	result[4] = t->min;
	result[5] = t->sec;
	return result;
}

Array<int> _hx_ssl_cert_get_notbefore( Dynamic hcert ){
	sslcert *cert = val_cert(hcert);
	if( !cert->c )
		hx::Throw( HX_CSTRING("cert_get_notbefore") );
	return x509_time_to_array( &cert->c->valid_from );
}

Array<int> _hx_ssl_cert_get_notafter( Dynamic hcert ){
	sslcert *cert = val_cert(hcert);
	if( !cert->c )
		hx::Throw( HX_CSTRING("cert_get_notafter") );
	return x509_time_to_array( &cert->c->valid_to );
}

Dynamic _hx_ssl_cert_get_next( Dynamic hcert ){
	sslcert *cert = val_cert(hcert);
	mbedtls_x509_crt *crt = cert->c->next;
	if( crt == NULL )
		return null();
	cert = new sslcert();
	cert->create(crt);
	return cert;
}

Dynamic _hx_ssl_cert_add_pem( Dynamic hcert, String data ){
   #ifdef HX_SMART_STRINGS
   if (data.isUTF16Encoded())
		hx::Throw( HX_CSTRING("Invalid data encoding") );
   #endif
	sslcert *cert = val_cert(hcert);
	int r;
	bool isnew = 0;
	if( !cert ){
		cert = new sslcert();
		cert->create( NULL );
		isnew = 1;
	}
	unsigned char *b = (unsigned char *)malloc((data.length+1) * sizeof(unsigned char));
	memcpy(b,data.raw_ptr(),data.length);
	b[data.length] = '\0';
	r = mbedtls_x509_crt_parse( cert->c, b, data.length+1 );
	free(b);
	if( r < 0 ){
		if( isnew )
			cert->destroy();
		ssl_error(r);
	}
	return cert;
}

Dynamic _hx_ssl_cert_add_der( Dynamic hcert, Array<unsigned char> buf ){
	sslcert *cert = val_cert(hcert);
	int r;
	bool isnew = 0;
	if( !cert ){
		cert = new sslcert();
		cert->create( NULL );
		isnew = 1;
	}
	if( (r = mbedtls_x509_crt_parse_der( cert->c, &buf[0], buf->length)) < 0 ){
		if( isnew )
			cert->destroy();
		ssl_error(r);
	}
	return cert;
}

Dynamic _hx_ssl_key_from_der( Array<unsigned char> buf, bool pub ){
	sslpkey *pk = new sslpkey();
	pk->create();
	int r;
	if( pub )
		r = mbedtls_pk_parse_public_key( pk->k, &buf[0], buf->length );
	else
		r = mbedtls_pk_parse_key( pk->k, &buf[0], buf->length, NULL, 0 );
	if( r != 0 ){
		pk->destroy();
		ssl_error(r);
	}
	return pk;
}

Dynamic _hx_ssl_key_from_pem( String data, bool pub, String pass ){
   #ifdef HX_SMART_STRINGS
   if (data.isUTF16Encoded())
		hx::Throw( HX_CSTRING("Invalid data encoding") );
   #endif
	sslpkey *pk = new sslpkey();
	pk->create();
	int r;
	unsigned char *b = (unsigned char *)malloc((data.length+1) * sizeof(unsigned char));
	memcpy(b,data.raw_ptr(),data.length);
	b[data.length] = '\0';
	if( pub ){
		r = mbedtls_pk_parse_public_key( pk->k, b, data.length+1 );
	}else if( pass == null() ){
		r = mbedtls_pk_parse_key( pk->k, b, data.length+1, NULL, 0 );
	}else{
      Array<unsigned char> pbytes(0,0);
      __hxcpp_bytes_of_string(pbytes,pass);
		r = mbedtls_pk_parse_key( pk->k, b, data.length+1, (const unsigned char *)pbytes->GetBase(), pbytes->length );
	}
	free(b);
	if( r != 0 ){
		pk->destroy();
		ssl_error(r);
	}
	return pk;
}

Array<unsigned char> _hx_ssl_dgst_make( Array<unsigned char> buf, String alg ){
   hx::strbuf ubuf;
	const mbedtls_md_info_t *md = mbedtls_md_info_from_string(alg.utf8_str(&ubuf));
	if( md == NULL )
		hx::Throw( HX_CSTRING("Invalid hash algorithm") );

	int size = mbedtls_md_get_size(md);
	Array<unsigned char> out = Array_obj<int>::__new(size,size);
	int r = -1;
	if( r = mbedtls_md( md, &buf[0], buf->length, &out[0] ) != 0 )
		ssl_error(r);

	return out;
}

Array<unsigned char> _hx_ssl_dgst_sign( Array<unsigned char> buf, Dynamic hpkey, String alg ){
	int r = -1;
	size_t olen = 0;
	unsigned char hash[32];
	sslpkey *pk = val_pkey(hpkey);

   hx::strbuf ubuf;
	const mbedtls_md_info_t *md = mbedtls_md_info_from_string( alg.utf8_str(&ubuf) );
	if( md == NULL )
		hx::Throw( HX_CSTRING("Invalid hash algorithm") );

	if( r = mbedtls_md( md, &buf[0], buf->length, hash ) != 0 )
		ssl_error(r);

	Array<unsigned char> result = Array_obj<unsigned char>::__new(MBEDTLS_MPI_MAX_SIZE,MBEDTLS_MPI_MAX_SIZE);
	if( r = mbedtls_pk_sign( pk->k, mbedtls_md_get_type(md), hash, 0, &result[0], &olen, mbedtls_ctr_drbg_random, &ctr_drbg ) != 0 )
		ssl_error(r);

	result[olen] = 0;
	result->__SetSize(olen);
	return result;
}

bool _hx_ssl_dgst_verify( Array<unsigned char> buf, Array<unsigned char> sign, Dynamic hpkey, String alg ){
	const mbedtls_md_info_t *md;
	int r = -1;
	unsigned char hash[32];
	sslpkey *pk = val_pkey(hpkey);

   hx::strbuf ubuf;
	md = mbedtls_md_info_from_string( alg.utf8_str(&ubuf) );
	if( md == NULL )
		hx::Throw( HX_CSTRING("Invalid hash algorithm") );

	if( r = mbedtls_md( md, &buf[0], buf->length, hash ) != 0 )
		ssl_error(r);

	if( r = mbedtls_pk_verify( pk->k, mbedtls_md_get_type(md), hash, 0, &sign[0], sign->length ) != 0 )
		return false;

	return true;
}

#if (_MSC_VER || defined(WIN32))

static void threading_mutex_init_alt( mbedtls_threading_mutex_t *mutex ){
	if( mutex == NULL )
		return;
	InitializeCriticalSection( &mutex->cs );
	mutex->is_valid = 1;
}

static void threading_mutex_free_alt( mbedtls_threading_mutex_t *mutex ){
    if( mutex == NULL || !mutex->is_valid )
        return;
	DeleteCriticalSection( &mutex->cs );
	mutex->is_valid = 0;
}

static int threading_mutex_lock_alt( mbedtls_threading_mutex_t *mutex ){
    if( mutex == NULL || !mutex->is_valid )
        return( MBEDTLS_ERR_THREADING_BAD_INPUT_DATA );

	EnterCriticalSection( &mutex->cs );
    return( 0 );
}

static int threading_mutex_unlock_alt( mbedtls_threading_mutex_t *mutex ){
    if( mutex == NULL || !mutex->is_valid )
        return( MBEDTLS_ERR_THREADING_BAD_INPUT_DATA );

    LeaveCriticalSection( &mutex->cs );
    return( 0 );
}

#endif

static bool _hx_ssl_inited = false;
void _hx_ssl_init() {
    if (_hx_ssl_inited) return;
    _hx_ssl_inited = true;

#if (_MSC_VER || defined(WIN32))
	mbedtls_threading_set_alt( threading_mutex_init_alt, threading_mutex_free_alt,
                           threading_mutex_lock_alt, threading_mutex_unlock_alt );
#endif

	// Init RNG
	mbedtls_entropy_init( &entropy );
	mbedtls_ctr_drbg_init( &ctr_drbg );
	mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0 );
}
