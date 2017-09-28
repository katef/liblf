.include "../share/mk/top.mk"

SRC        += src/lf.c

LIB        += liblf
SYMS.liblf += src/liblf.syms

.for src in ${SRC:Msrc/*.c}
${BUILD}/lib/liblf.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/liblf.opic: ${BUILD}/${src:R}.opic
.endfor

