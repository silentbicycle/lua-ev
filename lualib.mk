# Makefile for Lua libraries written in C.

all:	${LIBFILE}

clean:
	rm -f ${LIBNAME}${LIBEXT}* ${ARCHNAME}*.tar.gz ${ARCHNAME}*.zip *.core

${LIBFILE}: ${LIBPREFIX}${LIBNAME}.c
	${CC} -o $@ ${LIBPREFIX}${LIBNAME}.c ${CFLAGS} ${SHARED} \
		${LUA_FLAGS} ${INC} ${LIB_PATHS} ${LIBS} \
		${LUA_INC} ${LUA_LIBPATH} ${LUA_LIBS}

test: ${LIBFILE}
	${LUA} ${TESTSUITE}

lint: ${LIBPREFIX}${LIBNAME}.c
	${LINT} ${INC} ${LUA_INC} $>

tags:
	etags *c *h *.lua

tar:
	git archive --format=tar --prefix=${ARCHNAME}-${LIBVER}/ HEAD^{tree} \
		| gzip > ${ARCHNAME}-${LIBVER}.tar.gz

zip:
	git archive --format=zip --prefix=${ARCHNAME}-${LIBVER}/ HEAD^{tree} \
		> ${ARCHNAME}-${LIBVER}.zip

gdb:
	gdb `which lua` lua.core

install: ${LIBFILE}
	cp ${LIBFILE} ${LUA_DEST_LIB}
	cd ${LUA_DEST_LIB} && ln -s ${LIBFILEp} ${LIBNAME}${LIBEXT}

uninstall: 
	rm -f ${LUA_DEST_LIB}${LIBNAME}${LIBEXT}*
