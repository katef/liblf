.include "../share/mk/top.mk"

FMT += test/pass.fmt
FMT += test/fail.fmt

.for fmt in ${FMT}

test:: ${BUILD}/test ${BUILD}/bin/lfdump ${fmt}
	cat ${fmt} \
	| while read -r fmt; do \
		${BUILD}/bin/lfdump "$$fmt" \
		|| true; \
	done \
	>  ${BUILD}/${fmt:R}.out \
	2> ${BUILD}/${fmt:R}.err
	diff -u ${fmt:R}.err ${BUILD}/${fmt:R}.err
	diff -u ${fmt:R}.out ${BUILD}/${fmt:R}.out

.endfor

fuzz:: ${BUILD}/test ${BUILD}/bin/lfdump ${fmt}
.if defined(VERBOSE)
	BUILD=${BUILD} test/fuzz.sh -v ${FMT}
.else
	BUILD=${BUILD} test/fuzz.sh ${FMT}
.endif

