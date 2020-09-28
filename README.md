Intro
=====

Hvylya is a software-defined radio framework implemented in C++.

Its main distinguishing features are:
* Performance:
  * Most operations are implemented in terms of SIMD vectors and utilize SIMD intrinsics with SSSE3 and AVX2 support.
  * The framework automatically uses multiple threads to schedule the computations.
  * For performance-critical operators like FIRs multiple implementations are provided so that the one with the fastest performance for particular configuration can be used.
  * Compared to GNU Radio, some pipelines are executed several times faster.
* Functionality:
  * While not as extensive as GNU Radio operations set, lots of basic operations are still covered.
  * As one of the target use cases, the framework includes full FM receiver implementation with RDS support.
  * The framework also provides quite robust [Parks-McClellan FIR filter design algorithm](https://en.wikipedia.org/wiki/Parks%E2%80%93McClellan_filter_design_algorithm) implementation that can handle filters with multiple thousands of taps and design parameters that other frameworks often fail on.
* Design:
  * The operations support arbitrary numbers and types of inputs and outputs while being fully type-safe with compile-time checks.
  * There are several interfaces for implementing new operations starting with a simple "lambda-like" one and all the way to full control of all the inputs / outputs data flows.
  * The framework takes care of all the data flows and scheduling, but still provides the necessary control to the operations for prefetching / synchronizing the streams.

License
=======

Copyright (C) 2019 - 2020 Alexander Tsvyashchenko <sdr@endl.ch>

The license is [GPL v3](https://www.gnu.org/licenses/gpl-3.0.html).

Why this name?
==============

"Hvylya" means "the wave" in Ukrainian.

Possible improvements
=====================

Lots and lots of them :-) Just some basic ideas:
* Better type-safety when casting the data to / from SIMD vectors. It should be possible to remove the majority of "reinterpret\_cast" instances currently used in the code.
* More supported architectures: in addition to SSSE3 and AVX2 it would be great to also support NEON for fast execution on ARM CPUs. Offloading the calculations to GPUs for some of the most expensive operations would also be very useful.
* More flexible scheduling: there's some rudimentary support for tuning the scheduling parameters in the current implementation but ideally we should have several different schedulers optimized for batch vs real-time processing.
* Other use cases besides FM receiver, e.g. DAB+ and the necessary basic operations to support these.
* More SDR hardware APIs supported, preferably via some existing stacks rather than implementing their full support "from scratch".
* More tests, documentation, ...
