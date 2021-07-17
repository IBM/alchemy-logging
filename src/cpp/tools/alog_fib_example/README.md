# ALOG Example
This sample program is designed to provide an in-depth tutorial of all of the features offered by `ALOG`. The program itself computes fibonacci sequences and has lots of verbose logging throughout. The example can simply be perused, or you can follow the tutorial as outlined in this readme.

Before exploring this example, you should be familiar with the concepts of [Channels and Levels](https://github.ibm.com/watson-nlu/alog-cpp/tree/mastermaster_nlu#channels-and-levels) outlined in the top-level readme.

1. Setup:
    * [main.cpp:42](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/main.cpp#L42)
1. Channels:
    1. Class Channels: [fibonacci.h:42](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/include/fibonacci.h#L42)
    1. Free Channels: [fibonacci.cpp:28](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/src/fibonacci.cpp#L28)
1. Log Statements:
    1. Basic: [main.cpp:72](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/main.cpp#L72)
    1. Channel (i.e. `this`): [fibonacci.cpp:157](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/src/fibonacci.cpp#L157)
    1. Map Data: [fibonacci.cpp:68](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/src/fibonacci.cpp#L68)
    1. All-In-One: [fibonacci.cpp:95](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/src/fibonacci.cpp#L95)
    1. Fatal Errors [main.cpp:101](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/main.cpp#L101)
1. Log Scopes:
    1. Scoped Block: [main.cpp:84](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/main.cpp#L84)
    1. Function Block: [fibonacci.cpp:132](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/src/fibonacci.cpp#L132)
    1. Detail Function Block: [fibonacci.cpp:41](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/src/fibonacci.cpp#L41)
    1. Timers: [fibonacci.cpp:49](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/src/fibonacci.cpp#L49) [fibonacci.cpp:107](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/src/fibonacci.cpp#L107)
1. Metadata:
    1. [fibonacci.cpp:124](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/src/fibonacci.cpp#L124)
1. Enabled Blocks:
    * [main.cpp:142](https://github.ibm.com/watson-nlu/alog-cpp/blob/master_nlu/tools/alog_fib_example/main.cpp#L142)
