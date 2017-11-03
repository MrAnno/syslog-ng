HTTP server framework
=====================

For reviewers
-------------

### HTTP server class diagram

![HTTP server class diagram](/lib/http/images/HTTPServer.svg)

### LogProtoHTTPServer state machine

![LogProtoHTTPServer state machine](/lib/http/images/LogProtoHTTPServer.svg)

Q
---

- flow control for HTTP Handler?
- max-connections, IW problems with HTTP: keep-alive timeout is required + better defaults
- HTTP is not really a long-running TCP connection: 1 Handler + 1 Parser allocation for all connections?? save a few malloc
  - a low-memory-footprint fast parser is used, but it is wrapped around with heap-allocated buffers, etc. (per connection)

Framework
---------

- https is supported (TODO: peer-verify(optional-untrusted) as default)
- http upgrade is not supported
- http pipelining is supported
- http keep-alive is supported (performance will be terrible without keep-alive)
- http 0.9, 1.0 and 1.1 is supported
- http 2 is not supported
- the framework won't react on special headers, only upgrade, connection-length. The user of the framework is responsible for everything else.

- automatic error response when the request can not be parsed
- automatic internal error response if no response handler or response is specified

Performance
-----------

- Pretty good with keep-alive (~120K request/sec compared to nginx's default config: ~100K) [on my laptop]
- Pretty bad without keep-alive (as bad as nginx's default config)
- LogMessage processing slows down the proto (dramatically?)

```
# syslog-ng
$ wrk -t4 -c400 -d40s http://127.0.0.1:4444/

Running 40s test @ http://127.0.0.1:4444/
  4 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     3.48ms    1.32ms  39.64ms   87.45%
    Req/Sec    28.64k     3.95k   42.10k    81.11%
  4561797 requests in 40.10s
Requests/sec: 113771.36

# nginx
$ wrk -t4 -c400 -d40s http://127.0.0.1/                                                                                                                  40s 107ms
Running 40s test @ http://127.0.0.1/
  4 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    12.92ms   34.13ms 626.66ms   93.13%
    Req/Sec    21.48k     8.53k   56.14k    67.64%
  3375004 requests in 40.10s
Requests/sec:  84169.04
```

HL TODO
-------

- rename SrcDriver, LogSource, LogReader options if necessary (for example: log-msg-size to http-max-msg-size)
