include lib/http/source/Makefile.am

httpincludedir = ${pkgincludedir}/http

httpinclude_HEADERS = \
  lib/http/buffer.h \
  lib/http/logproto-http-server.h \
  lib/http/http-parser/http_parser.h \
  lib/http/http-parser.h \
  lib/http/http-status.h \
  lib/http/http-message.h \
  lib/http/http-message-internal.h

http_sources = \
  lib/http/logproto-http-server.c \
  lib/http/http-parser/http_parser.c \
  lib/http/http-parser.c \
  lib/http/http-message.c \
  $(http_source_sources)

include lib/http/tests/Makefile.am
