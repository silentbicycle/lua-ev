include config.mk

all:	${LIBFILE}

${LIBNAME}.c: ${LIBNAME}.h

include lualib.mk
