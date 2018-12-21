.include "../share/mk/top.mk"

test:: ${BUILD}/test ${BUILD}/bin/lfdump test/err.fmt test/err.txt
	for fmt in `cat test/err.fmt`; do \
		${BUILD}/bin/lfdump "$$fmt" 2>&1 || true; \
	done \
	> ${BUILD}/test/err.got
	diff -u test/err.txt ${BUILD}/test/err.got

fuzz:: ${BUILD}/test ${BUILD}/bin/lfdump test/err.fmt
.if defined(VERBOSE)
	BUILD=${BUILD} test/fuzz.sh -v test/err.fmt
.else
	BUILD=${BUILD} test/fuzz.sh test/err.fmt
.endif

