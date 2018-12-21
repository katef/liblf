.include "../share/mk/top.mk"

test:: ${BUILD}/test ${BUILD}/bin/lfdump test/err.txt
	for fmt in `cat test/err.fmt`; do \
		${BUILD}/bin/lfdump "$$fmt" 2>&1 || true; \
	done \
	> ${BUILD}/test/err.got
	diff -u test/err.txt ${BUILD}/test/err.got

