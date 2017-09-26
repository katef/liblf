.include "../../share/mk/top.mk"

SRC += examples/lfdump/main.c

PROG += lfdump

LFLAGS.lfdump += ${BUILD}/lib/liblf.a

.for lib in ${LIB:Mliblf}
${BUILD}/bin/lfdump: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mexamples/lfdump/*.c}
${BUILD}/bin/lfdump: ${BUILD}/${src:R}.o
.endfor

