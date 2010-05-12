include config.mk

all:	flaghash.h backendhash.h ${LIBNAME}${LIBEXT}

clean:
	@rm -f ${LIBNAME}${LIBEXT} {flag,backend}hash.h

gdb:
	gdb `which lua` lua.core

tags:
	etags *.c *.h *.lua

test: ${LIBNAME}${LIBEXT}
	${LUA} ${TESTSUITE}

install: ${LIBNAME}${LIBEXT}
	cp ${INST_LIB} ${LUA_DEST_LIB}

flaghash.h: gen_perfhash.lua
	${LUA} gen_perfhash.lua "EV_" ${EV_FLAGS} > $@

backendhash.h: gen_perfhash.lua
	${LUA} gen_perfhash.lua "EVBACKEND_" ${EV_BACKENDS} > $@

${LIBNAME}.c: ${LIBNAME}.h flaghash.h backendhash.h

${LIBNAME}${LIBEXT}: ${LIBNAME}.c
	${CC} -o $@ $> ${CFLAGS} ${LUA_FLAGS} ${INC} \
		${LIB_PATHS} ${LIBS}

lint:
	${LINT} ${INC} ${LUA_INC} ${LIBNAME}.c
