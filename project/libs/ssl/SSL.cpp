#include <string.h>

#ifdef _MSC_VER
#include <winsock2.h>
#include <wincrypt.h>
#else
#include <sys/socket.h>
#include <strings.h>
typedef int SOCKET;
#endif


#ifndef STATIC_LINK
#define IMPLEMENT_API
#endif

#include <hx/CFFI.h>

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

#define val_ssl(o)	(mbedtls_ssl_context*)val_data(o)
#define val_conf(o)	(mbedtls_ssl_config*)val_data(o)
#define val_cert(o) (mbedtls_x509_crt*)val_data(o)
#define val_pkey(o) (mbedtls_pk_context*)val_data(o)

DEFINE_KIND( k_ssl_conf );
DEFINE_KIND( k_ssl );
DEFINE_KIND( k_cert );
DEFINE_KIND( k_pkey );

static vkind k_socket;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;


static void free_cert( value v ){
	mbedtls_x509_crt *x = val_cert(v);
	mbedtls_x509_crt_free(x);
	free(x);
	free_abstract(v);
}

static void free_pkey( value v ){
	mbedtls_pk_context *k = val_pkey(v);
	mbedtls_pk_free(k);
	free(k);
	free_abstract(v);
}

static value block_error() {
	#ifdef NEKO_WINDOWS
	int err = WSAGetLastError();
	if( err == WSAEWOULDBLOCK || err == WSAEALREADY || err == WSAETIMEDOUT )
	#else
	if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EALREADY )
	#endif
		val_throw(alloc_string("Blocking"));
	val_throw(alloc_string("ssl network error"));
	return val_true;
}

static value ssl_error( int ret ){
	char buf[256];
	mbedtls_strerror(ret, buf, sizeof(buf));
	val_throw(alloc_string(buf));
	return val_false;
}

static value ssl_new( value config ) {
	int ret;
	mbedtls_ssl_context *ssl;
	val_check_kind(config,k_ssl_conf);
	ssl = (mbedtls_ssl_context *)malloc(sizeof(mbedtls_ssl_context));
	mbedtls_ssl_init(ssl);
	if( ret = mbedtls_ssl_setup(ssl, val_conf(config)) != 0 ){
		mbedtls_ssl_free(ssl);
		free(ssl);
		return ssl_error(ret);
	}
	return alloc_abstract( k_ssl, ssl );
}

static value ssl_close( value ssl ) {
	mbedtls_ssl_context *s;
	val_check_kind(ssl,k_ssl);
	s = val_ssl(ssl);
	mbedtls_ssl_free( s );
	free(s);
	free_abstract(ssl);
	return val_true;
}

static value ssl_handshake( value ssl ) {
	int r;
	val_check_kind(ssl,k_ssl);
	POSIX_LABEL(handshake_again);
	r = mbedtls_ssl_handshake( val_ssl(ssl) );
	if( r == SOCKET_ERROR ) {
		HANDLE_EINTR(handshake_again);
		return block_error();
	}else if( r != 0 )
		return ssl_error(r);
	return val_true;
}

int net_read( void *fd, unsigned char *buf, size_t len ){
	return recv((SOCKET)(socket_int)fd, (char *)buf, len, 0);
}

int net_write( void *fd, const unsigned char *buf, size_t len ){
	return send((SOCKET)(socket_int)fd, (char *)buf, len, 0);
}

static value ssl_set_socket( value ssl, value socket ) {
	val_check_kind(ssl,k_ssl);
	//TODO val_check_kind(socket,k_socket);
	mbedtls_ssl_set_bio( val_ssl(ssl), val_data(socket), net_write, net_read, NULL );
	return val_true;
}

static value ssl_set_hostname( value ssl, value hostname ){
	int ret;
	val_check_kind(ssl,k_ssl);
	val_check(hostname,string);
	if( ret = mbedtls_ssl_set_hostname(val_ssl(ssl), val_string(hostname)) != 0 )
		return ssl_error(ret);
	return val_true;
}

static value ssl_get_peer_certificate( value ssl ){
	value v;
	const mbedtls_x509_crt *crt;
	val_check_kind(ssl,k_ssl);
	crt = mbedtls_ssl_get_peer_cert(val_ssl(ssl));
	if( crt == NULL )
		return val_null;
 	v = alloc_abstract( k_cert, (void *)crt );
	val_gc(v,free_abstract);
	return v;
}

static value ssl_get_verify_result( value ssl ){
	int r;
	val_check_kind(ssl,k_ssl);
	r = mbedtls_ssl_get_verify_result( val_ssl(ssl) );
	if( r == 0 )
		return val_true;
	else if( r != -1 )
		return val_false;
	val_throw(alloc_string("not available"));
	return val_false;
}

static value ssl_send_char( value ssl, value v ) {
	unsigned char cc;
	int c;
	val_check_kind(ssl,k_ssl);
	val_check(v,int);
	c = val_int(v);
	if( c < 0 || c > 255 )
		val_throw(alloc_string("invalid char"));
	cc = (unsigned char) c;
	mbedtls_ssl_write( val_ssl(ssl), &cc, 1 );
	return val_true;
}

static value ssl_send( value ssl, value data, value pos, value len ) {
	int p,l,dlen;
	buffer buf;
	val_check_kind(ssl,k_ssl);
	val_check(data,buffer);
	val_check(pos,int);
	val_check(len,int);
	p = val_int(pos);
	l = val_int(len);
	buf = val_to_buffer(data);
	dlen = buffer_size(buf);
	if( p < 0 || l < 0 || p > dlen || p + l > dlen )
		hx_failure("ssl_send");
	POSIX_LABEL(send_again);
	dlen = mbedtls_ssl_write( val_ssl(ssl), (const unsigned char *)buffer_data(buf) + p, l );
	if( dlen == SOCKET_ERROR ) {
		HANDLE_EINTR(send_again);
		return block_error();
	}
	return alloc_int(dlen);
}

static value ssl_write( value ssl, value data ) {
	int len, slen;
	const unsigned char *s;
	mbedtls_ssl_context *ctx;
	val_check_kind(ssl,k_ssl);
	val_check(data,string);
	s = (const unsigned char *)val_string( data );
	len = val_strlen( data );
	ctx = val_ssl(ssl);
	while( len > 0 ) {
		POSIX_LABEL( write_again );
		slen = mbedtls_ssl_write( ctx, s, len );
		if( slen == SOCKET_ERROR ) {
			HANDLE_EINTR( write_again );
			return block_error();
		}
		s += slen;
		len -= slen;
	}
	return val_true;
}

static value ssl_recv_char(value ssl) {
	unsigned char c;
	int r;
	val_check_kind(ssl,k_ssl);
	r = mbedtls_ssl_read( val_ssl(ssl), &c, 1 );
	if( r <= 0 )
		hx_failure("ssl_recv_char");
	return alloc_int( c );
}

static value ssl_recv( value ssl, value data, value pos, value len ) {
	int p,l,dlen;
	unsigned char *buf;
	val_check_kind(ssl,k_ssl);
	val_check(data,buffer);
	val_check(pos,int);
	val_check(len,int);
	p = val_int( pos );
	l = val_int( len );
	buf = (unsigned char *) (buffer_data(val_to_buffer(data)) + p);
	POSIX_LABEL(recv_again);
	dlen = mbedtls_ssl_read( val_ssl(ssl), buf, l );
	if( dlen == SOCKET_ERROR ) {
		HANDLE_EINTR(recv_again);
		return block_error();
	}
	if( dlen < 0 )
		hx_failure("ssl_recv");
	return alloc_int( dlen );
}

static  value ssl_read( value ssl ) {
	int len, bufsize = 256;
	buffer b;
	unsigned char buf[256];
	mbedtls_ssl_context *ctx;
	val_check_kind(ssl,k_ssl);
	ctx = val_ssl(ssl);
	b = alloc_buffer(NULL);
	while( true ) {
		POSIX_LABEL(read_again);
		len = mbedtls_ssl_read( ctx, buf, bufsize );
		if( len == SOCKET_ERROR ) {
			HANDLE_EINTR(read_again);
			return block_error();
		}
		if( len == 0 )
			break;
		buffer_append_sub(b,(char *)buf,len);
	}
	return buffer_to_string(b);
}

static value conf_new( value server ) {
	int ret;
	mbedtls_ssl_config *conf;
	val_check(server,bool);
	conf = (mbedtls_ssl_config *)malloc(sizeof(mbedtls_ssl_config));
	mbedtls_ssl_config_init(conf);
	if( ret = mbedtls_ssl_config_defaults( conf, val_bool(server) ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
		MBEDTLS_SSL_TRANSPORT_STREAM, 0 ) != 0 ){
		mbedtls_ssl_config_free(conf);
		free(conf);
		return ssl_error( ret );
	}
	mbedtls_ssl_conf_rng( conf, mbedtls_ctr_drbg_random, &ctr_drbg );
	return alloc_abstract( k_ssl_conf, conf );
}

static value conf_close( value config ) {
	mbedtls_ssl_config *conf;
	val_check_kind(config,k_ssl_conf);
	conf = val_conf(config);
	mbedtls_ssl_config_free(conf);
	free(conf);
	free_abstract(config);
	return val_true;
}

static value conf_set_ca( value config, value cert ) {
	val_check_kind(config,k_ssl_conf);
	if( !val_is_null(cert) ) val_check_kind(cert,k_cert);
	mbedtls_ssl_conf_ca_chain( val_conf(config), val_is_null(cert) ? NULL : val_cert(cert), NULL );
	return val_true;
}

static value conf_set_verify( value config, value b ) {
	val_check_kind(config, k_ssl_conf);
	if( !val_is_null(b) ) val_check(b, bool);
	if( val_is_null(b) )
		mbedtls_ssl_conf_authmode(val_conf(config), MBEDTLS_SSL_VERIFY_OPTIONAL);
	else if( val_bool(b) )
		mbedtls_ssl_conf_authmode(val_conf(config), MBEDTLS_SSL_VERIFY_REQUIRED);
	else
		mbedtls_ssl_conf_authmode(val_conf(config), MBEDTLS_SSL_VERIFY_NONE);
	return val_true;
}

static value conf_set_cert( value config, value cert, value key ) {
	int r;
	val_check_kind(config,k_ssl_conf);
	val_check_kind(cert,k_cert);
	val_check_kind(key,k_pkey);

	if( r = mbedtls_ssl_conf_own_cert(val_conf(config), val_cert(cert), val_pkey(key)) != 0 )
		return ssl_error(r);

	return val_true;
}

static int sni_callback( void *arg, mbedtls_ssl_context *ctx, const unsigned char *name, size_t len ){
	if( name && arg ){
	 	value ret = val_call1((value)arg, alloc_string((const char*)name)) ;
		if( !val_is_null(ret) ){
			// TODO authmode and ca
			return mbedtls_ssl_set_hs_own_cert( ctx, val_cert(val_field(ret, val_id("cert"))), val_pkey(val_field(ret, val_id("key"))) );
		}
	}
	return -1;
}

static value conf_set_servername_callback( value config, value cb ){
	val_check_kind(config,k_ssl_conf);
	val_check_function(cb,1);
	mbedtls_ssl_conf_sni( val_conf(config), sni_callback, (void *)cb );
	return val_true;
}

static value cert_load_defaults(){
#if defined(NEKO_WINDOWS)
	value v;
	HCERTSTORE store;
	PCCERT_CONTEXT cert;
	mbedtls_x509_crt *chain = NULL;
	if( store = CertOpenSystemStore(0, (LPCSTR)"Root") ){
		cert = NULL;
		while( cert = CertEnumCertificatesInStore(store, cert) ){
			if( chain == NULL ){
				chain = (mbedtls_x509_crt *)malloc(sizeof(mbedtls_x509_crt));
				mbedtls_x509_crt_init( chain );
			}
			mbedtls_x509_crt_parse_der( chain, (unsigned char *)cert->pbCertEncoded, cert->cbCertEncoded );
		}
		CertCloseStore(store, 0);
	}
	if( chain != NULL ){
		v = alloc_abstract(k_cert, chain);
		val_gc(v,free_cert);
		return v;
	}
#elif defined(NEKO_MAC) && !defined(IPHONE) && !defined(APPLETV)
	CFMutableDictionaryRef search;
	CFArrayRef result;
	SecKeychainRef keychain;
	SecCertificateRef item;
	CFDataRef dat;
	value v;
	mbedtls_x509_crt *chain = NULL;

	// Load keychain
	if( SecKeychainOpen("/System/Library/Keychains/SystemRootCertificates.keychain",&keychain) != errSecSuccess )
		return val_null;

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
					chain = (mbedtls_x509_crt *)malloc(sizeof(mbedtls_x509_crt));
					mbedtls_x509_crt_init( chain );
				}
				mbedtls_x509_crt_parse_der( chain, (unsigned char *)CFDataGetBytePtr(dat), CFDataGetLength(dat) );
				CFRelease( dat );
			}
		}
	}
	CFRelease(keychain);
	if( chain != NULL ){
		v = alloc_abstract(k_cert, chain);
		val_gc(v,free_cert);
		return v;
	}
#endif
	return val_null;
}

static value cert_load_file(value file){
	int r;
	mbedtls_x509_crt *x;
	value v;
	val_check(file,string);
	x = (mbedtls_x509_crt *)malloc(sizeof(mbedtls_x509_crt));
	mbedtls_x509_crt_init( x );
	if( r = mbedtls_x509_crt_parse_file(x, val_string(file)) != 0 ){
		free(x);
		return ssl_error(r);
	}
	v = alloc_abstract(k_cert, x);
	val_gc(v,free_cert);
	return v;
}

static value cert_load_path(value path){
	int r;
	mbedtls_x509_crt *x;
	value v;
	val_check(path,string);
	x = (mbedtls_x509_crt *)malloc(sizeof(mbedtls_x509_crt));
	mbedtls_x509_crt_init( x );
	if( r = mbedtls_x509_crt_parse_path(x, val_string(path)) != 0 ){
		free(x);
		return ssl_error(r);
	}
	v = alloc_abstract(k_cert, x);
	val_gc(v,free_cert);
	return v;
}

static value asn1_buf_to_string( mbedtls_asn1_buf *dat ){
	unsigned int i, c;
	char *b;
	buffer buf = alloc_buffer_len( dat->len );
	b = buffer_data(buf);
	for( i = 0; i < dat->len; i++ )  {
        c = dat->p[i];
        if( c < 32 || c == 127 || ( c > 128 && c < 160 ) )
             b[i] = '?';
        else
			b[i] = c;
    }
	return buffer_to_string(buf);
}

static value cert_get_subject( value cert, value objname ){
	mbedtls_x509_crt *crt;
	mbedtls_x509_name *obj;
	int r;
	const char *oname, *rname;
	val_check_kind(cert,k_cert);
	val_check(objname, string);
	crt = val_cert(cert);
	obj = &crt->subject;
	if( obj == NULL )
		hx_failure("cert_get_subject");
	rname = val_string(objname);
	while( obj != NULL ){
		r = mbedtls_oid_get_attr_short_name( &obj->oid, &oname );
		if( r == 0 && strcmp( oname, rname ) == 0 )
			return asn1_buf_to_string( &obj->val );
		obj = obj->next;
	}
	return val_null;
}

static value cert_get_issuer(value cert, value objname){
	mbedtls_x509_crt *crt;
	mbedtls_x509_name *obj;
	int r;
	const char *oname, *rname;
	val_check_kind(cert,k_cert);
	val_check(objname, string);
	crt = val_cert(cert);
	obj = &crt->issuer;
	if( obj == NULL )
		hx_failure("cert_get_issuer");
	rname = val_string(objname);
	while( obj != NULL ){
		r = mbedtls_oid_get_attr_short_name( &obj->oid, &oname );
		if( r == 0 && strcmp( oname, rname ) == 0 )
			return asn1_buf_to_string( &obj->val );
		obj = obj->next;
	}
	return val_null;
}


static value cert_get_altnames( value cert ){
	mbedtls_x509_crt *crt;
	mbedtls_asn1_sequence *cur;
	value l = NULL, first = NULL;
	val_check_kind(cert, k_cert);
	crt = val_cert(cert);
	if( crt->ext_types & MBEDTLS_X509_EXT_SUBJECT_ALT_NAME ){
		cur = &crt->subject_alt_names;

		while( cur != NULL ){
			value l2 = alloc_array(2);
			val_array_set_i( l2, 0, asn1_buf_to_string(&cur->buf) );
			val_array_set_i( l2, 1, val_null );
			if (first == NULL)
				first = l2;
			else
				val_array_set_i( l, 1, l2);
			l = l2;

			cur = cur->next;
		}
	}
	return (first==NULL)?val_null:first;
}

static value x509_time_to_array( mbedtls_x509_time *t ){
	if( !t )
		hx_failure("x509_time_to_array");
	value v = alloc_array(6);
	val_array_set_i(v,0,alloc_int(t->year));
	val_array_set_i(v,1,alloc_int(t->mon));
	val_array_set_i(v,2,alloc_int(t->day));
	val_array_set_i(v,3,alloc_int(t->hour));
	val_array_set_i(v,4,alloc_int(t->min));
	val_array_set_i(v,5,alloc_int(t->sec));
	return v;
}

static value cert_get_notbefore(value cert){
	mbedtls_x509_crt *crt;
	val_check_kind(cert, k_cert);
	crt = val_cert(cert);
	if( !crt )
		hx_failure("cert_get_notbefore");
	return x509_time_to_array( &crt->valid_from );
}

static value cert_get_notafter(value cert){
	mbedtls_x509_crt *crt;
	val_check_kind(cert, k_cert);
	crt = val_cert(cert);
	if( !crt )
		hx_failure("cert_get_notbefore");
	return x509_time_to_array( &crt->valid_to );
}

static value cert_get_next( value cert ){
	mbedtls_x509_crt *crt;
	value v;
	val_check_kind(cert,k_cert);
	crt = (mbedtls_x509_crt *)val_cert(cert);
	crt = crt->next;
	if( crt == NULL )
		return val_null;
	v = alloc_abstract(k_cert, crt);
	val_gc(v,free_abstract);
	return v;
}

static value cert_add_pem( value cert, value data ){
	mbedtls_x509_crt *crt;
	int r, len;
	unsigned char *buf;
	bool isnew = 0;
	val_check(data,string);
	if( !val_is_null(cert) ){
		val_check_kind(cert,k_cert);
		crt = val_cert(cert);
		if( !crt )
			hx_failure("cert_add_pem");
	}else{
		crt = (mbedtls_x509_crt *)malloc(sizeof(mbedtls_x509_crt));
		mbedtls_x509_crt_init( crt );
		isnew = 1;
	}
	len = val_strlen(data)+1;
	buf = (unsigned char *)malloc(len);
	memcpy(buf, val_string(data), len-1);
	buf[len-1] = '\0';
	r = mbedtls_x509_crt_parse(crt, buf, len);
	free(buf);
	if( r < 0 ){
		if( isnew ){
			mbedtls_x509_crt_free(crt);
			free(crt);
		}
		return ssl_error(r);
	}
	if( isnew ){
		cert = alloc_abstract(k_cert, crt);
		val_gc(cert,free_cert);
	}
	return cert;
}

static value cert_add_der( value cert, value data ){
	mbedtls_x509_crt *crt;
	int r;
	buffer bdata;
	bool isnew = 0;
	val_check(data,buffer);
	if( !val_is_null(cert) ){
		val_check_kind(cert,k_cert);
		crt = val_cert(cert);
		if( !crt )
			hx_failure("cert_add_der");
	}else{
		crt = (mbedtls_x509_crt *)malloc(sizeof(mbedtls_x509_crt));
		mbedtls_x509_crt_init( crt );
		isnew = 1;
	}
	bdata = val_to_buffer(data);
	if( (r = mbedtls_x509_crt_parse_der(crt, (const unsigned char*)buffer_data(bdata), buffer_size(bdata))) < 0 ){
		if( isnew ){
			mbedtls_x509_crt_free(crt);
			free(crt);
		}
		return ssl_error(r);
	}
	if( isnew ){
		cert = alloc_abstract(k_cert, crt);
		val_gc(cert,free_cert);
	}
	return cert;
}

static value key_from_der( value data, value pub ){
	mbedtls_pk_context *pk;
	int r;
	value v;
	buffer buf;
	val_check(data, buffer);
	val_check(pub, bool);
	buf = val_to_buffer(data);
	pk = (mbedtls_pk_context *)malloc(sizeof(mbedtls_pk_context));
	mbedtls_pk_init(pk);
	if( val_bool(pub) )
		r = mbedtls_pk_parse_public_key( pk, (const unsigned char*)buffer_data(buf), buffer_size(buf) );
	else
		r = mbedtls_pk_parse_key( pk, (const unsigned char*)buffer_data(buf), buffer_size(buf), NULL, 0 );
	if( r != 0 ){
		mbedtls_pk_free(pk);
		free(pk);
		return ssl_error(r);
	}
	v = alloc_abstract(k_pkey, pk);
	val_gc(v,free_pkey);
	return v;
}

static value key_from_pem(value data, value pub, value pass){
	mbedtls_pk_context *pk;
	int r, len;
	value v;
	unsigned char *buf;
	val_check(data, string);
	val_check(pub, bool);
	if (!val_is_null(pass)) val_check(pass, string);
	len = val_strlen(data)+1;
	buf = (unsigned char *)malloc(len);
	memcpy(buf, val_string(data), len-1);
	buf[len-1] = '\0';
	pk = (mbedtls_pk_context *)malloc(sizeof(mbedtls_pk_context));
	mbedtls_pk_init(pk);
	if( val_bool(pub) )
		r = mbedtls_pk_parse_public_key( pk, buf, len );
	else if( val_is_null(pass) )
		r = mbedtls_pk_parse_key( pk, buf, len, NULL, 0 );
	else
		r = mbedtls_pk_parse_key( pk, buf, len, (const unsigned char*)val_string(pass), val_strlen(pass) );
	free(buf);
	if( r != 0 ){
		mbedtls_pk_free(pk);
		free(pk);
		return ssl_error(r);
	}
	v = alloc_abstract(k_pkey,pk);
	val_gc(v,free_pkey);
	return v;
}

static value dgst_make(value data, value alg){
	const mbedtls_md_info_t *md;
	int r = -1;
	buffer databuf, out;
	val_check(data, buffer);
	val_check(alg, string);

	md = mbedtls_md_info_from_string(val_string(alg));
	if( md == NULL ){
		val_throw(alloc_string("Invalid hash algorithm"));
		return val_null;
	}

	databuf = val_to_buffer(data);
	out = alloc_buffer_len( mbedtls_md_get_size(md) );
	if( r = mbedtls_md( md, (const unsigned char *)buffer_data(databuf), buffer_size(databuf), (unsigned char *)buffer_data(out) ) != 0 )
		return ssl_error(r);

	return buffer_val(out);
}

static value dgst_sign(value data, value key, value alg){
	const mbedtls_md_info_t *md;
	int r = -1;
	size_t olen = 0;
	buffer databuf, out;
	unsigned char hash[32];
	val_check(data, buffer);
	val_check_kind(key, k_pkey);
	val_check(alg, string);

	md = mbedtls_md_info_from_string(val_string(alg));
	if( md == NULL ){
		val_throw(alloc_string("Invalid hash algorithm"));
		return val_null;
	}

	databuf = val_to_buffer(data);
	if( r = mbedtls_md( md, (const unsigned char *)buffer_data(databuf), buffer_size(databuf), hash ) != 0 )
		return ssl_error(r);

	out = alloc_buffer_len(MBEDTLS_MPI_MAX_SIZE);
	if( r = mbedtls_pk_sign( val_pkey(key), mbedtls_md_get_type(md), hash, 0, (unsigned char *)buffer_data(out), &olen, mbedtls_ctr_drbg_random, &ctr_drbg ) != 0 )
		return ssl_error(r);

	buffer_data(out)[olen] = 0;
	buffer_set_size(out, olen);
	return buffer_val(out);
}

static value dgst_verify( value data, value sign, value key, value alg ){
	const mbedtls_md_info_t *md;
	int r = -1;
	buffer databuf, signbuf;
	unsigned char hash[32];
	val_check(data, buffer);
	val_check(sign, buffer);
	val_check_kind(key, k_pkey);
	val_check(alg, string);

	md = mbedtls_md_info_from_string(val_string(alg));
	if( md == NULL ){
		val_throw(alloc_string("Invalid hash algorithm"));
		return val_null;
	}

	databuf = val_to_buffer(data);
	if( r = mbedtls_md( md, (const unsigned char *)buffer_data(databuf), buffer_size(databuf), hash ) != 0 )
		return ssl_error(r);

	signbuf = val_to_buffer(sign);
	if( r = mbedtls_pk_verify( val_pkey(key), mbedtls_md_get_type(md), hash, 0, (unsigned char *)buffer_data(signbuf), buffer_size(signbuf) ) != 0 )
		return val_false;

	return val_true;
}

#if _MSC_VER

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

extern "C" {
void ssl_main() {
#if _MSC_VER
	mbedtls_threading_set_alt( threading_mutex_init_alt, threading_mutex_free_alt, 
                           threading_mutex_lock_alt, threading_mutex_unlock_alt );
#endif

	// Init RNG
	mbedtls_entropy_init( &entropy );
	mbedtls_ctr_drbg_init( &ctr_drbg );
	mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0 );

	k_ssl_conf = alloc_kind();
	k_ssl = alloc_kind();
	k_cert = alloc_kind();
	k_pkey = alloc_kind();
}

int ssl_register_prims() {
	static bool init = false;
    if (init) return 0;
    init = true;

    ssl_main();
 	return 0;
}
}

DEFINE_PRIM( ssl_new, 1 );
DEFINE_PRIM( ssl_close, 1 );
DEFINE_PRIM( ssl_handshake, 1 );
DEFINE_PRIM( ssl_set_socket, 2 );
DEFINE_PRIM( ssl_set_hostname, 2 );
DEFINE_PRIM( ssl_get_peer_certificate, 1 );
DEFINE_PRIM( ssl_get_verify_result, 1 );

DEFINE_PRIM( ssl_send_char, 2 );
DEFINE_PRIM( ssl_send, 4 );
DEFINE_PRIM( ssl_write, 2 );
DEFINE_PRIM( ssl_recv_char, 1 );
DEFINE_PRIM( ssl_recv, 4 );
DEFINE_PRIM( ssl_read, 1 );

DEFINE_PRIM( conf_new, 1 );
DEFINE_PRIM( conf_close, 1 );
DEFINE_PRIM( conf_set_ca, 2 );
DEFINE_PRIM( conf_set_verify, 2 );
DEFINE_PRIM( conf_set_cert, 3 );
DEFINE_PRIM( conf_set_servername_callback, 2 );

DEFINE_PRIM( cert_load_defaults, 0 );
DEFINE_PRIM( cert_load_file, 1 );
DEFINE_PRIM( cert_load_path, 1 );
DEFINE_PRIM( cert_get_subject, 2 );
DEFINE_PRIM( cert_get_issuer, 2 );
DEFINE_PRIM( cert_get_altnames, 1 );
DEFINE_PRIM( cert_get_notbefore, 1 );
DEFINE_PRIM( cert_get_notafter, 1 );
DEFINE_PRIM( cert_get_next, 1 );
DEFINE_PRIM( cert_add_pem, 2 );
DEFINE_PRIM( cert_add_der, 2 );

DEFINE_PRIM( key_from_pem, 3 );
DEFINE_PRIM( key_from_der, 2 );

DEFINE_PRIM( dgst_make, 2 );
DEFINE_PRIM( dgst_sign, 3 );
DEFINE_PRIM( dgst_verify, 4 );

DEFINE_ENTRY_POINT(ssl_main);
