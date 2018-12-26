
## liblf: A parser for the syntax used by Apache's LogFormat Directive

The LogFormat directive provides a printf-like syntax for specifying
user-defined custom formats for log messages. It's intended for logs
about HTTP traffic, and includes formatting for IP addresses, response
status, and other relevant things.

There's an [EBNF grammar for the syntax](doc/logfmt.ebnf).
[Apache's documentation](https://httpd.apache.org/docs/current/mod/mod_log_config.html#formats)
is the authoritative reference for the various format directives.

## What would I use it for?

Maybe you're writing a daemon that serves requests (not necessarily HTTP)
and you'd like to give your users a familiar-feeling syntax for
configuring what to log. Or perhaps you're writing a tool to convert
Apache configurations to another system.

You can disable the directives you don't need, and add custom ones.

## How do I use it?

This library aims to provide a complete implementation, presenting an
API independent of any particular source for data, and of any particular
output. The entry point is lf_parse(), which calls various callbacks as
it parses each part of the format string. See [<lf.h>](include/lf/lf.h)
for the API.

* As a parser: Have your callbacks store the data,
  and act on it later.
* As an interpreter: Parse log format strings whilst serving a request,
  and have your callbacks output the relevant items immediately.
* As a compiler: Parse log format strings ahead of time, and output
  generated code.

There's an example program which just prints out directives as they come.
You get pretty decent error messages:
```
; lfdump '%{}e %v %h %l %u %t \"%r\" %>s %b %{user-agent}i'
error: Empty name
at 1: '%{}e %v %h %l %u %t \"%r\" %>s %b %{user-agent}i'
--------^^
```

Related projects:

 * This C library is an independent implementation, and uses no code from
   [Apache's original implementation](https://github.com/apache/httpd/blob/trunk/modules/loggers/mod_log_config.c).
 * Kazeburo's [Apache::LogFormat::Compiler](https://github.com/kazeburo/Apache-LogFormat-Compiler),
   the inspiration for this project.
 * Miyagawa's [port of Apache::LogFormat::Compiler to Go](https://github.com/miyagawa/go-apache-logformat).


Clone with submodules (contains required .mk files):

    ; git clone --recursive https://github.com/katef/liblf.git

To build and install:

    ; pmake -r install

You can override a few things:

    ; CC=clang PREFIX=$HOME pmake -r install

Building depends on:

 * Any BSD make. This includes OpenBSD, FreeBSD and NetBSD make(1)
   and sjg's portable bmake (also packaged as pmake).

 * A C compiler. Any should do, but GCC and clang are best supported.

 * ar, ld, and a bunch of other stuff you probably already have.

Fuzzing depends on:

 * [Radamsa](https://gitlab.com/akihe/radamsa)
 * [Blab](https://github.com/aoh/blab)
 * [KGT](https://github.com/katef/kgt)

```
; pmake -r CC=gcc DEBUG=1 && pmake VERBOSE=1 -r fuzz
```

Ideas, comments or bugs: kate@elide.org

