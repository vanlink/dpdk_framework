PKGCONF=pkg-config

LIBNAME=libdkfw.a

LIBS=$(shell $(PKGCONF) --libs libdpdk) -lrt -lm -ldl -lcrypto -pthread -lnuma -lcjson -fpic
INCLUDE=-I. -I/usr/include -I/usr/local/include -I./include
DEPS=-Wall -Wextra $(shell $(PKGCONF) --cflags libdpdk) -DALLOW_EXPERIMENTAL_API

ifeq ($(D),1)
OPTIMIZE=-O0
else
OPTIMIZE=-O2
endif

.PHONY: build

build: ${LIBNAME}

OBJS=dpdkframework.o dpdkframework_intf.o dpdkframework_core.o dpdkframework_profile.o \
      dpdkframework_ipc.o dpdkframework_timer.o dpdkframework_memory.o dpdkframework_stats.o dpdkframework_cps.o \
      dpdkframework_mempool.o

${LIBNAME}: ${OBJS}
	ar crv $@ ${OBJS}

dpdkframework.o: dpdkframework.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework.c

dpdkframework_intf.o: dpdkframework_intf.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework_intf.c

dpdkframework_core.o: dpdkframework_core.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework_core.c

dpdkframework_profile.o: dpdkframework_profile.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework_profile.c

dpdkframework_ipc.o: dpdkframework_ipc.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework_ipc.c

dpdkframework_timer.o: dpdkframework_timer.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework_timer.c

dpdkframework_memory.o: dpdkframework_memory.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework_memory.c

dpdkframework_stats.o: dpdkframework_stats.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework_stats.c

dpdkframework_cps.o: dpdkframework_cps.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework_cps.c

dpdkframework_mempool.o: dpdkframework_mempool.c
	gcc -g ${OPTIMIZE} ${DEPS} ${INCLUDE} ${LIBS} -c dpdkframework_mempool.c

clean:
	rm -f *.o ${LIBNAME}
