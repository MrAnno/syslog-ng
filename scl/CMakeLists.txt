set(SCL_DIRS
    apache
    cim
    elasticsearch
    graphite
    hdfs
    kafka
    loggly
    logmatic
    mbox
    nodejs
    pacct
    rewrite
    solaris
    syslogconf
    system
)

install(DIRECTORY ${SCL_DIRS} DESTINATION share/syslog-ng/include/scl)
install(FILES syslog-ng.conf scl.conf DESTINATION etc)
