LIBNAME=libdkfw.a

LIBS=-L${RTE_SDK}/${RTE_TARGET}/lib -Wl,--whole-archive,-ldpdk,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma -fpic
INCLUDE=-I. -I/usr/include -I${RTE_SDK}/${RTE_TARGET}/include -I./include
DEPS=-Wall -march=native

ifeq ($(D),1)
OPTIMIZE=-O0
else
OPTIMIZE=-O2
endif

.PHONY: build

build: ${LIBNAME}

OBJS=dpdkframework.o dpdkframework_intf.o dpdkframework_core.o dpdkframework_profile.o

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

clean:
	rm -f *.o ${LIBNAME}
