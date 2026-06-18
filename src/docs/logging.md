Wisp logging
===============

Wisp has a large number of internal diagnostic messages which can
be viewed by the developer (or user if they wish)

Each message has a category and a level which can be used to control
which messages are displayed.

The message category is used to allow filters to separate messages of
the same level from different sources.

The logging levels, from low to high, are:

  - DEEPDEBUG
  - DEBUG
  - VERBOSE
  - INFO
  - WARNING
  - ERROR
  - CRITICAL

Compilation control
-------------------

At compilation time the logging behaviour can be controlled by using
CMake variables. The parameters are:

  - WISP_LOG_LEVEL  
  This controls what level of message is compiled into the Wisp
  binary. Supported values: ERROR, WARNING, INFO, DEBUG, VERBOSE,
  DEEPDEBUG, CRITICAL.
  The default value is DEEPDEBUG for Debug builds and INFO for others.
  
  - WISP_DISABLE_LOGGING
  This is automatically set for non-Debug builds to disable all logging
  and tracing for performance.

  - WISP_BUILTIN_LOG_FILTER (via options.h)
  Sets the default non-verbose filter.

  - WISP_BUILTIN_VERBOSE_FILTER (via options.h)
  Sets the default verbose filter. The verbose level is selected from
  the commandline with the -v switch.

Command line
------------

The main command line switches that control logging are:

  - -v
  switches between the normal and verbose levels

  - -V <file>
  Send the logging to a file instead of standard output 
  
  - --log_filter=<filter>
  Set the non verbose filter

  - --verbose_filter=<filter>
  Set the verbose filter

Examples:

    ./nsgtk --log_filter="level:INFO"
    ./nsgtk -v --verbose_filter="(cat:layout && level:DEBUG)"
    ./nsgtk -v --verbose_filter="((cat:layout && level:DEBUG) || level:INFO)"

Options
-------

The logging filters can be configured by setting the log_filter and
log_verbose_filter options.

Adding messages
---------------

Messages can be easily added by including the utils/log.h header and
he NSLOG() macro. for example

    NSLOG(wisp, INFO, "An example message %d", example_func());

nslog
-----

If the nslog library is used it allows for application of a filter to
control which messages are output. The nslog filter syntax is best
viewed in its [documentation](http://source.netsurf-browser.org/libnslog.git/tree/docs/mainpage.md)
