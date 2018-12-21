#!/bin/sh

# https://gitlab.com/akihe/radamsa

# radamsa -n produces multiple outputs in one go. However that doesn't suit
# the use here, because we're inspecting output for certian properties for
# each output, and concatenating them all would tend to miss the latter texts
# by bailing out on earlier errors.
#
# I think using -n would make sense for testing in parallel, if lfdump were
# to support that.

die() {
	echo $* >&2
	exit 1
}

verbose=0
if [ "$1" = '-v' ]; then
	verbose=1
	shift
fi

while true; do
	cat $* | sort -R | head -1 | radamsa -o $BUILD/test/fuzz.fmt
	$BUILD/bin/lfdump "`cat $BUILD/test/fuzz.fmt`" \
		>  $BUILD/test/fuzz.out \
		2> $BUILD/test/fuzz.err
	r=$?

	if [ $verbose -eq 1 ]; then
		echo -e '\e[HJ\e[2J'
		cat $BUILD/test/fuzz.fmt | head -5
		echo "-- stdout --"; cat $BUILD/test/fuzz.out | head -10
		echo "-- stderr --"; cat $BUILD/test/fuzz.err | head -20
	fi

	if [ `stat -c '%s' $BUILD/test/fuzz.out` -gt 0 ]; then
		grep -qvi '^error: ' $BUILD/test/fuzz.out || die "unexpected error"
	fi

	if [ $r -eq 1 ]; then
		grep -qvi '^error: success' $BUILD/test/fuzz.err      || die "success error"
		grep -q   '^error: ' $BUILD/test/fuzz.err             || die "missing error"
		grep -q   "^at [0-9]\+: '.\+'\$" $BUILD/test/fuzz.err || die "ill-formed line"
		grep -q   '^-*\^\+$' $BUILD/test/fuzz.err             || die "ill-formed marker"
	fi

	if [ $r -eq 0 ]; then
		[ `stat -c '%s' $BUILD/test/fuzz.err` -eq 0 ] || die "unexpected stderr output"
	fi

	if [ $r -gt 127 ]; then
		if [ $verbose -ne 1 ]; then
			echo -e '\e[HJ\e[2J'
			echo "-- input --";  cat $BUILD/test/fuzz.fmt
			echo "-- \$?=$r"
			echo "-- stdout --"; cat $BUILD/test/fuzz.out
			echo "-- stderr --"; cat $BUILD/test/fuzz.err
		fi

		break
	fi
done

