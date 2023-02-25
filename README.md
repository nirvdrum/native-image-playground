# Native Image Playground

## Overview

The Native Image Playground project is a series of examples for illustrating how to invoke Java code, including
Truffle-based languages, in a variety of embedding scenarios. There are two primary approaches taken: 1) load the JVM
in a process via libjvm and execute code using the Java Native Interface (JNI) [Invocation API](https://docs.oracle.com/en/java/javase/17/docs/specs/jni/invocation.html);
and 2) use the GraalVM Native Image tool to create native shared libraries of ahead-of-time compiled Java code (including
Truffle-based languages), which can then be called from an application using either JNI or exported function symbols.

To keep things consistent, the same workload is used in all examples: a geospatial algorithm known as the [Haversine distance](https://en.wikipedia.org/wiki/Haversine_formula).
This particular algorithm was chosen because the examples here are a spiritual successor to the [Top 10 Things To Do With GraalVM](https://medium.com/graalvm/graalvm-ten-things-12d9111f307d)
article written by [Chris Seaton](https://chrisseaton.com/). As a minor note, the Haversine distance formula used here
comes from the Apache SIS geospatial library. In the time since Chris wrote his post, it was discovered that the implementation
in Apache SIS was incorrect. The library has deprecated the function in favor of a more complex solution. Despite that,
this project continues to use the older, incorrect implementation. It makes for a good comparison to Chris's article
and the original Haversine distance formula was a compact, mathematical equation that can be represented compactly with
machine code.

## Prerequisites

In order to build this project you will need to have clang and clang++ installed, along with a [GraalVM distribution](https://graalvm.org/)
and [Apache Maven](https://maven.apache.org/). Both GraalVM and Maven can be installed with [SDKMan!](https://sdkman.io/):

```
$ sdk install java 22.3.1.r17-grl
$ sdk use java 22.3.1.r17-grl
$ sdk install maven
```

If you don't want to use SDKMan!, you can fetch a GraalVM distribution and set the `JAVA_HOME` environment variable
to the directory that contains the unpackaged distribution's `bin/` dir. On macOS, the `bin/` directory is nested under
the `Contents/Home/` directory, leading to something like:

```
export JAVA_HOME=$HOME/dev/bin/graalvm-ce-java17-22.3.1/Contents/Home/
```

You can also manage the Maven installation on your own. Many popular package managers have a way of installing it.

The rest of the documentation assumes that you have both the GraalVM `bin/` directory and Maven available on your
`PATH`. If you do not, you will need to adjust commands accordingly to point at the correct binaries.

The GraalVM distribution does not include _native-image_ out of the box. Instead, it provides a utility called the
GraalVM Component Updater with which you can install _native-image_ and other GraalVM components. The utility is
packaged as a binary called `gu`. To see which components are available for installation, you can run `gu available`:

```
$ gu available
Downloading: Component catalog from www.graalvm.org
ComponentId              Version             Component name                Stability                     Origin
---------------------------------------------------------------------------------------------------------------------------------
espresso                 22.3.1              Java on Truffle               Supported                     github.com
espresso-llvm            22.3.1              Java on Truffle LLVM Java librSupported                     github.com
js                       22.3.1              Graal.js                      Supported                     github.com
llvm                     22.3.1              LLVM Runtime Core             Supported                     github.com
llvm-toolchain           22.3.1              LLVM.org toolchain            Supported                     github.com
native-image             22.3.1              Native Image                  Early adopter                 github.com
native-image-llvm-backend22.3.1              Native Image LLVM Backend     Early adopter (experimental)  github.com
nodejs                   22.3.1              Graal.nodejs                  Supported                     github.com
python                   22.3.1              GraalVM Python                Experimental                  github.com
R                        22.3.1              FastR                         Experimental                  github.com
ruby                     22.3.1              TruffleRuby                   Experimental                  github.com
visualvm                 22.3.1              VisualVM                      Experimental                  github.com
wasm                     22.3.1              GraalWasm                     Experimental                  github.com
```

This repository will require the _native-image_ and _ruby_ components.

```
$ gu install native-image ruby
Downloading: Component catalog from www.graalvm.org
Processing Component: Native Image
Processing Component: TruffleRuby
Processing Component: LLVM.org toolchain
Processing Component: LLVM Runtime Core
Additional Components are required:
    LLVM.org toolchain (org.graalvm.llvm-toolchain, version 22.3.1), required by: TruffleRuby (org.graalvm.ruby)
    LLVM Runtime Core (org.graalvm.llvm, version 22.3.1), required by: TruffleRuby (org.graalvm.ruby)
Downloading: Component native-image: Native Image from github.com
Downloading: Component ruby: TruffleRuby from github.com
Downloading: Component org.graalvm.llvm-toolchain: LLVM.org toolchain from github.com
Downloading: Component org.graalvm.llvm: LLVM Runtime Core from github.com
Installing new component: LLVM Runtime Core (org.graalvm.llvm, version 22.3.1)
Installing new component: LLVM.org toolchain (org.graalvm.llvm-toolchain, version 22.3.1)
Installing new component: Native Image (org.graalvm.native-image, version 22.3.1)
Installing new component: TruffleRuby (org.graalvm.ruby, version 22.3.1)

IMPORTANT NOTE:
---------------
The Ruby openssl C extension needs to be recompiled on your system to work with the installed libssl.
First, make sure TruffleRuby's dependencies are installed, which are described at:
  https://github.com/oracle/truffleruby/blob/master/README.md#dependencies
Then run the following command:
        /home/nirvdrum/.sdkman/candidates/java/22.3.1.r17-grl/languages/ruby/lib/truffle/post_install_hook.sh
```

Please pay attention to the output. There are two things of note here. The first is that the TruffleRuby native image
needs to recompile the OpenSSL extension for your machine. If you skip this step and try to use the _openssl_ gem in
that native image, you'll likely get a confusing error message telling you the OpenSSL library could not be found. In
the output above, we're prompted to run:

```
$ $HOME/.sdkman/candidates/java/22.3.1.r17-grl/languages/ruby/lib/truffle/post_install_hook.sh
```

The recompilation of the OpenSSL extension will take a few minutes. If you're certain you won't be using the _openssl_
library within the TruffleRuby native image, you can skip this step. But, if you can afford the time, doing the
recompilation of the native extension now will save you some confusion later should you decide to use the _openssl_
library.

The second note in the output above is that _libpolyglot_ is now out of date. _libpolyglot_ is a native shared library
that allows using Truffle's polyglot API from other languages. It was not listed in the `gu available` output earlier
because it comes pre-installed, but each time you add a new Truffle language that you would like to access through
_libpolyglot_, you need to rebuild it. In our case here, we've just installed the Ruby native image and would like to
be able to use TruffleRuby through _libpolyglot_. In order to do so, we run:

```
$ gu rebuild libpolyglot
```

As a warning, this rebuild requires ~8 GB RAM and takes ~5 minutes to build on an Apple M1 Pro 8-core machine running
in Rosetta.

## Building the Native Image Playground

The project's Maven configuration is split into multiple profiles, allowing you to pick the type of image you would
like. The list of profiles are:

    * native-image: A standalone executable written in Java
    * native-library: A shared library written in Java
    * native-library-ruby: A shared library written in Java + Ruby
    * native-polyglot: Uses libpolyglot to execute JS and Ruby from a launcher program
    * jni-libjvm-polyglot: Uses JNI and Truffle polyglot API to execute JS and Ruby via libjvm loaded from a launcher program
    * jni-native: Uses JNI and Truffle polyglot API to execute JS and Ruby via a native image shared library loaded from a launcher program
    * benchmark-setup: Fetches and builds the Google Benchmark library for running benchmarks
    * benchmark: Runs benchmarks comparing `@CEntryPoint` and JNI Invocation API calls

To build any of these profiles, you run:

```
$ mvn -P <profile_name> -D skipTests=true clean package
```

Maven stores the compiled artifacts in the `target/` directory. The `clean` option in the build command will delete
the `target/` directory before building the new package with _native-image_. This will avoid any leftover artifacts
from other profiles conflicting with your latest build. But, if you need to keep the old `target/` around for some
reason, you can simply remove the `clean` option from the above command. In Maven, the `target/` directory is treated
as a tool-controlled directory and Maven can and will do whatever it wants to that directory. You should never store
code in there that you're not comfortable losing.

### Profile: native-image

The _native-image_ profile builds a Java application that calls into the Apache SIS library to computer the Haversine
distance of two coordinates. All functionality is written in Java and is compiled to machine code in a static binary.

```
$ mvn -P native-image -D skipTests=true package
$ ./target-native-image/native-image-playground 51.507222 -0.1275 40.7127 -74.0059
5570.248509 km
```

### Profile: native-library

The _native-library_ profile builds a Java class file into a native shared library. Much like with the _native-image_
profile, the Haversine distance function comes from the Apache SIS library. However, this time the function is exported
and available to external callers. To make calling the function easier, Native Image will also generate a header file
with a function declaration for each function declared with `@CEntryPoint`. In this case, the exported function of note
is named `distance`.

Additionally, the Native Image Playground includes a launcher application written in C to demonstrate how to call that
function. The resulting binary is named `native-library-runner`.

```
$ mvn -P native-library -D skipTests=true clean package
$ ./target-native-library/native-library-runner 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```

### Profile: native-library-ruby

The _native-library-ruby_ profile is quite similar to the _native-library_ profile. In this case, the profile builds a
native shared library that exports a `distance_ruby` function, which calls a port of Apache SIS's Haversine formula to
Ruby. The Ruby code is executed using TruffleRuby, which is embedded in the shared library (i.e., there is no external
dependency). TruffleRuby is invoked via Truffle's polyglot API in Java and the function is exposed with `@CEntryPoint`.

```
$ mvn -P native-library-ruby -D skipTests=true clean package
$ ./target-native-library-ruby/native-library-runner-ruby 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```

### Profile: native-polyglot

The _native-polyglot_ profile builds a C launcher that uses GraalVM's _libpolyglot_ and the native polyglot API to call
JavaScript and Ruby implementations of the Haversine distance algorithm. _libpolyglot_ must be manually rebuilt when a
new GraalVM language is installed using `gu install`. While JavaScript is installed in GraalVM by default, _libpolyglot_
still must be built before its first usage. To rebuild _libpolyglot_, run:

```
$ gu rebuild libpolyglot
Building libpolyglot...
========================================================================================================================
GraalVM Native Image: Generating 'libpolyglot' (shared library)...
========================================================================================================================
[1/7] Initializing...                                                                                    (6.1s @ 0.32GB)
 Version info: 'GraalVM 22.3.1 Java 17 CE'
 Java version info: '17.0.6+10-jvmci-22.3-b13'
 C compiler: gcc (linux, x86_64, 12.2.0)
 Garbage collector: Serial GC
 4 user-specific feature(s)
 - com.oracle.svm.truffle.TruffleBaseFeature: Provides base support for Truffle
 - com.oracle.svm.truffle.TruffleFeature: Enables compilation of Truffle ASTs to machine code
 - org.graalvm.home.HomeFinderFeature: Finds GraalVM paths and its version number
 - org.graalvm.polyglot.nativeapi.PolyglotNativeAPIFeature
[2/7] Performing analysis...  [*********]                                                               (43.1s @ 4.54GB)
  28,540 (96.68%) of 29,520 classes reachable
  43,215 (65.74%) of 65,738 fields reachable
 138,638 (73.16%) of 189,505 methods reachable
  17,223 ( 9.09%) of 189,505 methods included for runtime compilation
     408 classes,   102 fields, and 2,113 methods registered for reflection
      65 classes,    75 fields, and    56 methods registered for JNI access
       4 native libraries: dl, pthread, rt, z
[3/7] Building universe...                                                                               (3.6s @ 2.34GB)
[4/7] Parsing methods...      [***]                                                                      (6.9s @ 5.31GB)
[5/7] Inlining methods...     [****]                                                                     (4.6s @ 3.21GB)
[6/7] Compiling methods...    [*****]                                                                   (31.6s @ 3.83GB)
[7/7] Creating image...                                                                                 (14.9s @ 4.39GB)
  85.45MB (39.16%) for code area:   110,354 compilation units
 125.48MB (57.50%) for image heap:1,368,750 objects and 658 resources
   7.30MB ( 3.35%) for other data
 218.24MB in total
------------------------------------------------------------------------------------------------------------------------
Top 10 packages in code area:                               Top 10 object types in image heap:
   2.26MB org.truffleruby.core.string                         24.61MB byte[] for code metadata
   2.09MB org.truffleruby.core.array                          11.47MB byte[] for graph encodings
   1.69MB com.oracle.truffle.polyglot                          8.09MB java.lang.Class
   1.67MB org.truffleruby.core.numeric                         6.77MB byte[] for general heap data
   1.63MB com.oracle.truffle.llvm.runtime.nodes.cast           6.62MB byte[] for java.lang.String
   1.59MB sun.security.ssl                                     6.38MB java.lang.String
   1.59MB com.oracle.truffle.llvm.runtime.nodes.op             4.45MB char[]
   1.58MB com.oracle.truffle.api.strings                       3.51MB int[]
   1.57MB org.truffleruby.interop                              3.04MB byte[] for embedded resources
   1.33MB com.oracle.truffle.llvm.runtime.nodes.asm            2.48MB java.lang.Object[]
  67.62MB for 672 more packages                               47.58MB for 8361 more object types
------------------------------------------------------------------------------------------------------------------------
                        9.5s (7.5% of total time) in 75 GCs | Peak RSS: 8.87GB | CPU load: 7.95
------------------------------------------------------------------------------------------------------------------------
Produced artifacts:
 /home/nirvdrum/.sdkman/candidates/java/22.3.1.r17-grl/lib/polyglot/graal_isolate.h (header)
 /home/nirvdrum/.sdkman/candidates/java/22.3.1.r17-grl/lib/polyglot/graal_isolate_dynamic.h (header)
 /home/nirvdrum/.sdkman/candidates/java/22.3.1.r17-grl/lib/polyglot/libpolyglot.build_artifacts.txt (txt)
 /home/nirvdrum/.sdkman/candidates/java/22.3.1.r17-grl/lib/polyglot/libpolyglot.so (shared_lib)
 /home/nirvdrum/.sdkman/candidates/java/22.3.1.r17-grl/lib/polyglot/polyglot_api.h (header)
 /home/nirvdrum/.sdkman/candidates/java/22.3.1.r17-grl/lib/polyglot/polyglot_api_dynamic.h (header)
 /home/nirvdrum/.sdkman/candidates/java/22.3.1.r17-grl/lib/polyglot/polyglot_isolate.h (header)
 /home/nirvdrum/.sdkman/candidates/java/22.3.1.r17-grl/lib/polyglot/polyglot_isolate_dynamic.h (header)
========================================================================================================================
Finished generating 'libpolyglot' in 2m 5s.
```

As a warning, building _libpolyglot_ is a computationally heavy process. It will take a long time and you may need to
close applications in order to afford the builder more memory to operate. Fortunately, the image only needs to rebuilt
when installing a new language or upgrading GraalVM. In contract, the other profiles need to be rebuilt every time
there's a change to the `@CEntryPoint` set of methods.

Once _libployglot_ is built, you can compile and invoke the C launcher. There is no Java code or build process necessary
for this profile. _libpolyglot_ exposes all of the functionality needed to call into a Truffle language. Moreover, the
native polyglot API allows sending polyglot messages to a Truffle language without needing to use Truffle Java-based API.
The launcher script provides implementations of the Haversine distance formula in JavaScript and Ruby, mostly to
illustrate how _libpolyglot_ supports multiple languages. When invoking the launcher you must provide the language to
use as the first argument. Acceptable values are "js" and "ruby" (case-sensitive).

```
$ mvn -P native-polyglot -D skipTests=true clean package
$ ./target-native-polyglot/native-polyglot ruby 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
$ ./target-native-polyglot/native-polyglot js 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```

### Profile: jni-libjvm-polyglot

The _jni-libjvm-polyglot_ profile builds a C++ launcher that uses JNI to load a copy of the JVM as a shared library.
Once loaded, the launcher program uses Truffle's Java-based polyglot API to call JavaScript and Ruby implementations of
the Haversine distance algorithm (both ports of the Apache SIS Java implementation). While this profile does not use code
built by GraalVM's native image tool, it still requires using `gu install ruby` to install the TruffleRuby JAR, which is
then loaded for execution of any Ruby code. Graal.js is bundled with the GraalVM distribution, so it is available
out-of-the box.

```
$ mvn -P jni-libjvm-polyglot -D skipTests=true clean package
$ ./target-jni-libjvm/jni-runner ruby 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
$ ./target-jni-libjvm/jni-runner js 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```

### Profile: jni-native

The _jni-native_ profile is very similar to the _jni-libjvm-polyglot_. In fact, they share the same C++-based launcher.
What changes is the libraries the launcher is linked against and consequently, the execution environment. Whereas the
_jni-libjvm-polyglot_ profile loads a standard JVM into memory, the _jni-native_ profile uses a shared library built
using the GraalVM Native Image tool, just like was used in the _native-library-ruby_ profile. While not a JVM, the
shared library still adheres to the Java Native Interface (NI) Invocation API. Here, creating a JVM corresponds to
creating a Graal isolate. In contrast to the other library examples that rely on functions exported with `@CEntryPoint`,
this launcher uses JNI to call JavaScript and Ruby implementations of the Haversine distance algorithm using the
Java-based Truffle polyglot API.

As with other shared libraries using Truffle languages, the Truffle languages must already be installed via `gu install`
in order for them to be linked into the library. If you haven't already done so, you will need to run `gu install ruby`
(JavaScript support is provided out-of-the box in the GraalVM distribution).

```
$ mvn -P jni-native -D skipTests=true clean package
$ ./target-jni-native/jni-runner ruby 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
$ ./target-jni-native/jni-runner js 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```

### Profile: benchmark-setup

The _benchmark-setup_ profile fetches and builds a copy of the [Google Benchmark](https://github.com/google/benchmark)
library. The compiled library will be linked into the benchmark runner when the _benchmark_ profile is built. Since the
benchmark library only needs to be built once and because it's an expensive process, it was broken out from the _benchmark_
profile.

The _build-google-benchmark.sh_ script at the root of this project does most of the work.  If you want to change the
version of the library being used, you'll need to modify that script. The generated files will be in the _target/benchmark_
directory, but the files there are for internal use only.

```
$ mvn -P benchmark-setup -D skipTests=true clean package
```

### Profile: benchmark

The _benchmark_ profile builds a benchmark runner that uses the [Google Benchmark](https://github.com/google/benchmark)
framework to explore performance differences in the various approaches taken in this project to call Java code &mdash;
and by extension, Truffle interpreters &mdash; from external processes. To do so, the profile invokes Native Image to
build a native shared library, which is then loaded by the benchmark runner (a C++ application).

In order to build the profile, you must first build the _benchmark-setup_ profile. If you forget to do so, building
the _benchmark_ profile will error with a message like `fatal error: 'benchmark/benchmark.h' file not found`. The
_benchmark-setup_ profile only needs to be built once. Its artifacts do not live in the same target directory as the
benchmarks, so you can clean and rebuild the benchmarks without trashing the benchmark library artifacts.

To build the benchmark runner, you run:

```
$ mvn -P benchmark -D skipTests=true clean package
```

The benchmark runner supports several options that influence what is run and how. To see the full list, you can use
the `--help` option:

```
$ ./target-benchmark/benchmark-runner --help
benchmark [--benchmark_list_tests={true|false}]
          [--benchmark_filter=<regex>]
          [--benchmark_min_time=<min_time>]
          [--benchmark_repetitions=<num_repetitions>]
          [--benchmark_enable_random_interleaving={true|false}]
          [--benchmark_report_aggregates_only={true|false}]
          [--benchmark_display_aggregates_only={true|false}]
          [--benchmark_format=<console|json|csv>]
          [--benchmark_out=<filename>]
          [--benchmark_out_format=<json|console|csv>]
          [--benchmark_color={auto|true|false}]
          [--benchmark_counters_tabular={true|false}]
          [--benchmark_context=<key>=<value>,...]
          [--benchmark_time_unit={ns|us|ms|s}]
          [--v=<verbosity>]
```

The options are documented in the Google Benchmark [User Guide](https://github.com/google/benchmark/blob/8d86026c67e41b1f74e67c1b20cc8f73871bc76e/docs/user_guide.md).
The `--benchmark-filter` option is particularly helpful if you only want to run a sub-set of the benchmarks.

To run all the benchmarks, you can use:

```
$ ./target-benchmark/benchmark-runner
```

The simple benchmark invocation is fairly quick and can give you  ballpark figures. When I want to collect performance
numbers that I want to share, I use a more rigorous configuration. In this case, I always run the benchmarks on dedicated
hardware running Linux. To help eliminate issues related to CPU throttling, I disable CPU frequency scaling before
starting the benchmarks. For reasons outlined in the "A Note about Warm-Up" section, I also add some flags to the
benchmark runner to help ensure each benchmark has warmed up. Finally, I run each benchmark multiple times to help
diminish the effects of random system events. As an added bonus, the benchmark library will compute and report descriptive
statistics when multiple benchmark repetitions are used.

```
$ sudo cpupower frequency-set --governor performance # Disable CPU frequency scaling on Linux
$ ./target-benchmark/benchmark-runner --benchmark_enable_random_interleaving=true --benchmark_repetitions=3 --benchmark_min_time=30
$ sudo cpupower frequency-set --governor schedutil # Re-enable CPU frequency scaling on Linux
```

#### A Note about Warm-Up

The Google Benchmark library has limited control over warming up a benchmark, which is problematic when benchmarking
something with a JIT compiler as we're doing here. The library runs each benchmark for a specified amount of time and
tries to determine the ideal number of iterations for that benchmark by running it in a loop for a fixed number of
iterations. After each round, the library checks if time is still remaining in the window and if so, it bumps the number
of iterations and runs again, this time for a longer time-slice. Once it determines how many iterations to use for that
benchmark, it then runs the benchmark again with that iteration count and takes performance measurements. The iteration
discovery only occurs once &mdash; if you specify multiple repetitions for a benchmark to get more stable values, each
repetition will use the same discovered iteration count. Alternatively, the benchmark harness can be configured for a
fixed number of iterations, but this option is not exposed as a runtime configuration value; it must be configured directly
in the benchmark runner's source code.

The Google Benchmark library allows running setup & destroy functions before each benchmark, which the benchmark runner
uses to initialize a Graal Isolate. Unfortunately, the setup & destroy methods are run before each iteration in the
discovery process and then again before each repetition. To help keep everything isolated, the benchmark runner will
destroy the Graal Isolate in its teardown method. If the library needs to take eight rounds of execution to determine
the ideal iteration count, then the Graal Isolate will be created and destroyed eight times. Naturally, that complicates
the JIT process because compiled methods are constantly being discarded. Rather than fight the library, we configure a
minimum execution time(specified with the `--benchmark_min_time` option with a unit of seconds), which ultimately extends
the iteration count discovery process and results in a high enough iteration count that should be sufficient for each
benchmark to fully warm up, even if earlier compiled methods are trashed. Just to reiterate, however, the benchmark library
is not running a separate warm-up pass and makes no guarantees about benchmark stability. As such, we specify a conservative
value for the benchmark's minimum execution time, but even then it's possible that the benchmark has not fully warmed up.

Through many executions of the benchmarks with GraalVM 22.1.0, I've decided to move ahead with a 30 second minimum
execution time. This number is quite conservative as the performance difference in 10s vs 30s is virtually non-existent.
Since the goal of the benchmarks is to evaluate different mechanisms for calling code packaged up in a Native Image
shared library, we do want to ensure fairness for each technique. By choosing a larger minimum execution time value, we
allow for reasonable variance in the warm-up process. On the other hand, if a benchmark is unable to warm up in our
conservative time window, that's good to know as well. In the case of Native Image binaries, we're concerned with start-up
time and overall throughput, not just peak performance. If peak performance were all that mattered, we could call into a
JVM-based GraalVM distribution using the JNI Invocation API.

With all of that said, if the total benchmark time is intolerable, you can rebuild the benchmark runner and set the
`REUSE_CONTEXT` `#define` value. Enabling that setting will share the Graal Isolate across all runs. It should allow
methods to warm up in a much shorter amount of time, but benchmark results may be impacted by compilations from other
benchmarks. You'll probably want to use the `--benchmark_enable_random_interleaving=true` option and run multiple times
to avoid execution order impacting results. To enable the setting you'll have to modifythe benchmark code or the build
command in the project's _pom.xml_.
