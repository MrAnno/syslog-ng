# Message generator source

This example source module generates a message every `freq()` seconds based on its `template()` option.

Please note that `template()` is a source-side template, only message-independent macros can be used.

### Usage

```
source s_generator {
  msg-generator(
    template("-- ${ISODATE} ${LOGHOST} ${SEQNUM} Generated message. --")
    freq(0.5)
  );
};

log {
  source(s_generator);

  destination {
    file("/dev/stdout");
  };
};
```
