Self
====

Experimental 64 Bit Version
===========================

This is an experimental branch of the Self VM.

It is a port of the Self VM to 64 bit systems.

It is downstream from the main Self repo at https://github.com/russellallen/self

At the moment the interpreter is working, but the JIT has not been ported.

You can build the VM by:

```
cmake -S vm64 -B build -DCMAKE_BUILD_TYPE=Release 
cmake --build build -j$(nproc)   
```

It should build on MacOS (arm64) and Linux (amd64 and arm64).


AI Disclosure Statement
=======================

This port is LLM asssisted, using primarily Claude Anthropic.


About Self
==========

The Self homepage is at http://selflanguage.org

Please go there for prebuilt binaries for Linux, NetBSD and FreeBSD, documentation,
blogs and forums.

For information on:

  * Using Self, read the [Self Handbook][1]
  * Building a Self VM, read the Handbook's [VM building instructions][2]
  * Building a Self World, read the [world building instructions][3]
  
[1]: http://handbook.selflanguage.org/2017.1/
[2]: http://handbook.selflanguage.org/2017.1/buildvm.html
[3]: http://handbook.selflanguage.org/2017.1/buildworld.html
