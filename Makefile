include config.mk

all:	${LIBFILE}

clean: cleanhash

${LIBFILE}: flaghash.h backendhash.h

cleanhash:
	rm -f flaghash.h backendhash.h

flaghash.h:
	${LUA} gen_perfhash.lua "EV_" ${EV_FLAGS} > $@

backendhash.h:
	${LUA} gen_perfhash.lua "EVBACKEND_" ${EV_BACKENDS} > $@

${LIBNAME}.c: ${LIBNAME}.h flaghash.h backendhash.h

include lualib.mk
