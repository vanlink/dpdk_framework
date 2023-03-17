PKGCONF=pkg-config
DPDK_CFLAGS=-Wall $(shell $(PKGCONF) --cflags libdpdk) -I/usr/local/include -I${DKFW_SDK}/include
DPDK_LDFLAGS=$(shell $(PKGCONF) --libs libdpdk) -lrt -lm -ldl -lcrypto -pthread -lnuma -fpic

all: guard-DKFW_SDK app0 app1 distr0 mix0 mix1 oneprocess

guard-%:
	@ if [ "${${*}}" = "" ]; then echo "Environment variable $* not set"; exit 1; fi

oneprocess:
	gcc -g -O2 ${DPDK_CFLAGS} oneprocess.c ${DKFW_SDK}/libdkfw.a ${DPDK_LDFLAGS} -o test-oneprocess

mix0:
	gcc -g -O2 ${DPDK_CFLAGS} mix0.c common.c ${DKFW_SDK}/libdkfw.a ${DPDK_LDFLAGS} -o test-mix0

mix1:
	gcc -g -O2 ${DPDK_CFLAGS} mix1.c common.c ${DKFW_SDK}/libdkfw.a ${DPDK_LDFLAGS} -o test-mix1

app0:
	gcc -g -O2 ${DPDK_CFLAGS} app0.c common.c ${DKFW_SDK}/libdkfw.a ${DPDK_LDFLAGS} -o test-app0

app1:
	gcc -g -O2 ${DPDK_CFLAGS} app1.c common.c ${DKFW_SDK}/libdkfw.a ${DPDK_LDFLAGS} -o test-app1

distr0:
	gcc -g -O2 ${DPDK_CFLAGS} distr0.c common.c ${DKFW_SDK}/libdkfw.a ${DPDK_LDFLAGS} -o test-distr0

.PHONY: clean

clean:
	rm -f test*
