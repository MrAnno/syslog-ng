Experimental unit test framework integration for syslog-ng
=================================

Criterion
---------

![Criterion](https://github.com/Snaipe/Criterion/raw/bleeding/doc/screencast.gif)

### Prerequisites

WARNING: The library is supported on Linux, OS X, FreeBSD, and Windows.

### Installation

```sh
git clone https://github.com/Snaipe/Criterion.git
cd Criterion
mkdir b
cd b
cmake ..
make install
```

Documentation: http://criterion.readthedocs.io/en/master/index.html

syslog-ng + CMake
-----------------

### Building syslog-ng with CMake (DEBUG mode)

More info: https://github.com/balabit/syslog-ng/pull/1051

```sh
git clone https://github.com/balabit/syslog-ng.git
cd syslog-ng
mkdir b
cd b
cmake -DCMAKE_INSTALL_PREFIX=/syslog-ng-install -DCMAKE_BUILD_TYPE=Debug ..
```

Running tests
-------------

```
$ make test # or ctest

Test project /root/dev-syslog-ng/b
      Start  1: test_clone_logmsg
 1/17 Test  #1: test_clone_logmsg ................   Passed    0.04 sec
      Start  2: test_dnscache
 2/17 Test  #2: test_dnscache ....................   Passed    7.63 sec
      Start  3: test_findcrlf
 3/17 Test  #3: test_findcrlf ....................   Passed    0.25 sec
      Start  4: test_hostid
 4/17 Test  #4: test_hostid ......................   Passed    0.04 sec
      Start  5: test_logqueue
 5/17 Test  #5: test_logqueue ....................   Passed    0.37 sec
      Start  6: test_logwriter
 6/17 Test  #6: test_logwriter ...................   Passed    0.03 sec
      Start  7: test_matcher
 7/17 Test  #7: test_matcher .....................   Passed    0.09 sec
      Start  8: test_msgparse
 8/17 Test  #8: test_msgparse ....................   Passed    0.19 sec
      Start  9: test_msgsdata
 9/17 Test  #9: test_msgsdata ....................   Passed    0.03 sec
      Start 10: test_nvtable
10/17 Test #10: test_nvtable .....................   Passed    1.61 sec
      Start 11: test_persist_state
11/17 Test #11: test_persist_state ...............   Passed    0.11 sec
      Start 12: test_ringbuffer
12/17 Test #12: test_ringbuffer ..................   Passed    0.10 sec
      Start 13: test_serialize
13/17 Test #13: test_serialize ...................   Passed    0.04 sec
      Start 14: test_tags
14/17 Test #14: test_tags ........................   Passed    0.20 sec
      Start 15: test_value_pairs
15/17 Test #15: test_value_pairs .................   Passed    0.05 sec
      Start 16: test_value_pairs_walk
16/17 Test #16: test_value_pairs_walk ............   Passed    0.03 sec
      Start 17: test_zone
17/17 Test #17: test_zone ........................   Passed    0.11 sec

100% tests passed, 0 tests failed out of 17

Total Test time (real) =  10.99 sec
```

Command line arguments
----------------------
[Criterion Docs - Environment and CLI](http://criterion.readthedocs.io/en/master/env.html)

```
$ tests/unit/test_msgparse

[====] Synthesis: Tested: 16 | Passing: 16 | Failing: 0 | Crashing: 0
```

```
$ tests/unit/test_ringbuffer --jobs 2 --verbose

[----] Criterion v2.2.1
[====] Running 13 tests from ringbuffer:
[RUN ] ringbuffer::test_broken_continual_range
[PASS] ringbuffer::test_broken_continual_range: (0.00s)
[RUN ] ringbuffer::test_continual_range
[PASS] ringbuffer::test_continual_range: (0.00s)
[RUN ] ringbuffer::test_drop_elements
[PASS] ringbuffer::test_drop_elements: (0.00s)
[RUN ] ringbuffer::test_element_at
[PASS] ringbuffer::test_element_at: (0.00s)
[RUN ] ringbuffer::test_elements_ordering
[RUN ] ringbuffer::test_init_buffer_state
[PASS] ringbuffer::test_init_buffer_state: (0.00s)
[PASS] ringbuffer::test_elements_ordering: (0.00s)
[RUN ] ringbuffer::test_pop_all_pushed_element_in_correct_order
[PASS] ringbuffer::test_pop_all_pushed_element_in_correct_order: (0.00s)
[RUN ] ringbuffer::test_pop_from_empty_buffer
[PASS] ringbuffer::test_pop_from_empty_buffer: (0.00s)
[RUN ] ringbuffer::test_push_after_pop
[PASS] ringbuffer::test_push_after_pop: (0.00s)
[RUN ] ringbuffer::test_push_to_full_buffer
[PASS] ringbuffer::test_push_to_full_buffer: (0.00s)
[RUN ] ringbuffer::test_ring_buffer_is_full
[RUN ] ringbuffer::test_tail
[PASS] ringbuffer::test_ring_buffer_is_full: (0.00s)
[PASS] ringbuffer::test_tail: (0.00s)
[RUN ] ringbuffer::test_zero_length_continual_range
[PASS] ringbuffer::test_zero_length_continual_range: (0.00s)
[====] Synthesis: Tested: 13 | Passing: 13 | Failing: 0 | Crashing: 0
```

```
$ tests/unit/test_ringbuffer --pattern "*pop*" --verbose

[----] Criterion v2.2.1
[====] Running 13 tests from ringbuffer:
[RUN ] ringbuffer::test_pop_all_pushed_element_in_correct_order
[PASS] ringbuffer::test_pop_all_pushed_element_in_correct_order: (0.00s)
[RUN ] ringbuffer::test_pop_from_empty_buffer
[PASS] ringbuffer::test_pop_from_empty_buffer: (0.00s)
[RUN ] ringbuffer::test_push_after_pop
[PASS] ringbuffer::test_push_after_pop: (0.00s)
[SKIP] ringbuffer::test_zero_length_continual_range: Test is disabled
...
[====] Synthesis: Tested: 3 | Passing: 3 | Failing: 0 | Crashing: 0
```

Environment Variables (for automake integration)
------------------------------------------------

```
$ CRITERION_JOBS=1 CRITERION_VERBOSITY_LEVEL=1 tests/unit/test_persist_state

[----] Criterion v2.2.1
[====] Running 11 tests from persist_state:
[RUN ] persist_state::test_persist_state_foreach_entry
[PASS] persist_state::test_persist_state_foreach_entry: (0.00s)
...
[====] Synthesis: Tested: 11 | Passing: 11 | Failing: 0 | Crashing: 0
```

[Debugging](https://github.com/Snaipe/Criterion/issues/127)
---------

*Note: You must set the build type to "Debug" ( `cmake -DCMAKE_BUILD_TYPE=Debug ...` )*

#### "Simple mode" (with the `--simple` Criterion flag)
[![asciicast](https://asciinema.org/a/8qkanezmgpu4zdfss17m4g8ql.png)](https://asciinema.org/a/8qkanezmgpu4zdfss17m4g8ql?t=44)

#### "Inferior" (standard, following child processes)
[![asciicast](https://asciinema.org/a/03w55wzdgniygbkdx5d6qwz4k.png)](https://asciinema.org/a/03w55wzdgniygbkdx5d6qwz4k?t=13)

#### syslog-ng RingBuffer debug demo
[![asciicast](https://asciinema.org/a/07fsy88cjm4mn35wd94wa75fd.png)](https://asciinema.org/a/07fsy88cjm4mn35wd94wa75fd)
