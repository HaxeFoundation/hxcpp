MBEDTLS_OLD = mbedtls-2.2.1
MBEDTLS_NEW = mbedtls-2.9.0
MBEDTLS_URL = https://tls.mbed.org/download/mbedtls-2.9.0-apache.tgz

all: purge upgrade

purge:
	rm -rf $(MBEDTLS_OLD)

upgrade: $(MBEDTLS_NEW)

$(MBEDTLS_NEW):
	curl $(MBEDTLS_URL) | gunzip | tar xf -
	rm -rf $(MBEDTLS_NEW)/DartConfiguration.tcl \
	       $(MBEDTLS_NEW)/circle.yml \
	       $(MBEDTLS_NEW)/configs \
	       $(MBEDTLS_NEW)/doxygen \
	       $(MBEDTLS_NEW)/programs \
	       $(MBEDTLS_NEW)/scripts \
	       $(MBEDTLS_NEW)/tests \
	       $(MBEDTLS_NEW)/visualc \
	       $(MBEDTLS_NEW)/yotta
	{ echo 'diff --git include/mbedtls/config.h include/mbedtls/config.h'; \
	  echo 'index 9585e692..5aa405bc 100644'; \
	  echo '--- include/mbedtls/config.h'; \
	  echo '+++ include/mbedtls/config.h'; \
	  echo '@@ -1480,6 +1480,9 @@'; \
	  echo '  * Uncomment this to allow your own alternate threading implementation.'; \
	  echo '  */'; \
	  echo ' //#define MBEDTLS_THREADING_ALT'; \
	  echo '+#ifdef HX_WINDOWS'; \
	  echo '+#define MBEDTLS_THREADING_ALT'; \
	  echo '+#endif'; \
	  echo ' '; \
	  echo ' /**'; \
	  echo '  * \def MBEDTLS_THREADING_PTHREAD'; \
	  echo '@@ -1491,6 +1494,9 @@'; \
	  echo '  * Uncomment this to enable pthread mutexes.'; \
	  echo '  */'; \
	  echo ' //#define MBEDTLS_THREADING_PTHREAD'; \
	  echo '+#ifndef HX_WINDOWS'; \
	  echo '+#define MBEDTLS_THREADING_PTHREAD'; \
	  echo '+#endif'; \
	  echo ' '; \
	  echo ' /**'; \
	  echo '  * \def MBEDTLS_VERSION_FEATURES'; \
	  echo '@@ -2206,7 +2212,7 @@'; \
	  echo '  *'; \
	  echo '  * This module provides networking routines.'; \
	  echo '  */'; \
	  echo '-#define MBEDTLS_NET_C'; \
	  echo '+//#define MBEDTLS_NET_C'; \
	  echo ' '; \
	  echo ' /**'; \
	  echo '  * \def MBEDTLS_OID_C'; \
	  echo '@@ -2570,7 +2576,7 @@'; \
	  echo '  *'; \
	  echo '  * Enable this layer to allow use of mutexes within mbed TLS'; \
	  echo '  */'; \
	  echo '-//#define MBEDTLS_THREADING_C'; \
	  echo '+#define MBEDTLS_THREADING_C'; \
	  echo ' '; \
	  echo ' /**'; \
	  echo '  * \def MBEDTLS_TIMING_C'; \
	  echo 'diff --git include/mbedtls/threading_alt.h include/mbedtls/threading_alt.h'; \
	  echo 'new file mode 100644'; \
	  echo 'index 00000000..71c2179d'; \
	  echo '--- /dev/null'; \
	  echo '+++ include/mbedtls/threading_alt.h'; \
	  echo '@@ -0,0 +1,7 @@'; \
	  echo '+#include <windows.h>'; \
	  echo '+'; \
	  echo '+typedef struct'; \
	  echo '+{'; \
	  echo '+    CRITICAL_SECTION cs;'; \
	  echo '+    char is_valid;'; \
	  echo '+} mbedtls_threading_mutex_t;'; \
	} | patch -p0 -d $(MBEDTLS_NEW)

clean:
	rm -rf $(MBEDTLS_NEW)
