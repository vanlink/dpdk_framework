DPDK_CFLAGS=-Wall -march=native -I${DKFW_SDK}/include/ -I${RTE_SDK}/${RTE_TARGET}/include/ -I${DKFW_SDK}/include/ -include ${RTE_SDK}/${RTE_TARGET}/include/rte_config.h
DPDK_LDFLAGS=-L${RTE_SDK}/${RTE_TARGET}/lib -Wl,--whole-archive,-ldpdk,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma -fpic

all: app0 app1 distr0

app0:
	gcc -g -O2 ${DPDK_CFLAGS} ${DPDK_LDFLAGS} app0.c common.c ${DKFW_SDK}/libdkfw.a -o test-app0

app1:
	gcc -g -O2 ${DPDK_CFLAGS} ${DPDK_LDFLAGS} app1.c common.c ${DKFW_SDK}/libdkfw.a -o test-app1

distr0:
	gcc -g -O2 ${DPDK_CFLAGS} ${DPDK_LDFLAGS} distr0.c common.c ${DKFW_SDK}/libdkfw.a -o test-distr0

.PHONY: clean

clean:
	rm -f test*
