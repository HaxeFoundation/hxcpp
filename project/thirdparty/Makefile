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

clean:
	rm -rf $(MBEDTLS_NEW)
