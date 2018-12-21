.MAKEFLAGS: -r -m share/mk

# targets
all::  mkdir .WAIT dep .WAIT lib prog
dep::
gen::
test::
fuzz::
install:: all
uninstall::
clean::

# things to override
CC      ?= gcc
BUILD   ?= build
PREFIX  ?= /usr/local

# layout
SUBDIR += examples/lfdump
SUBDIR += examples
SUBDIR += src
SUBDIR += pc
SUBDIR += test

INCDIR += include

.include <subdir.mk>
.include <pkgconf.mk>
.include <pc.mk>
.include <obj.mk>
.include <dep.mk>
.include <ar.mk>
.include <so.mk>
.include <part.mk>
.include <prog.mk>
.include <mkdir.mk>
.include <install.mk>
.include <clean.mk>

