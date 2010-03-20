include config.mk

all:	${LIBNAME}${LIBEXT}

clean:
	@rm -f ${LIBNAME}${LIBEXT}

gdb:
	gdb `which lua` lua.core

test: ${LIBNAME}${LIBEXT}
	${LUA} ${TESTSUITE}

install: ${LIBNAME}${LIBEXT}
	cp ${INST_LIB} ${LUA_DEST_LIB}

${LIBNAME}.c: ${LIBNAME}.h

${LIBNAME}${LIBEXT}: ${LIBNAME}.c
	${CC} -o $@ $> ${CFLAGS} ${LUA_FLAGS} ${INC} ${LIB_PATHS} ${LIBS}
