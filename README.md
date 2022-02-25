# Native Image Playground

## Prerequisites

In order to build this project you will need to have clang and clang++ installed, along with a [GraalVM distribution](https://graalvm.org/)
and [Apache Maven](https://maven.apache.org/). For those on Apple M1 hardware, you will have to run everything listed
here in a Rosetta session. GraalVM does not currently support M1 natively. Both GraalVM and Maven can be installed with
[SDKMan!](https://sdkman.io/):

```
$ sdk install java 22.0.0.2.r17-grl
$ sdk use java 22.0.0.2.r17-grl
$ sdk install maven
```

If you don't want to use SDKMan!, you can fetch a GraalVM distribution and set the `JAVA_HOME` environment variable
to the directory that contains the unpackaged distribution's `bin/` dir. On macOS, the `bin/` directory is nested under
the `Contents/Home/` directory, leading to something like:

```
export JAVA_HOME=$HOME/dev/bin/graalvm-ce-java17-22.0.0.2/Contents/Home/
```

You can also manage the Maven installation on your own. Many popular package managers have a way of installing it.

The rest of the documentation assumes that you have both the GraalVM `bin/` directory and Maven available on your
`PATH`. If you do not, you will need to adjust commands accordingly to point at the correct binaries.

The GraalVM distribution does not include _native-image_ out of the box. Instead, it provides a utility called the
GraalVM Component Updater with which you can install _native-image_ and other GraalVM components. The utility is
packaged as a binary called `gu`. To see which components are available for installation, you can run `gu available`:

```
$ gu available
Downloading: Release index file from oca.opensource.oracle.com
Downloading: Component catalog for GraalVM Enterprise Edition 22.0.0.1 on jdk17 from oca.opensource.oracle.com
Downloading: Component catalog for GraalVM Enterprise Edition 22.0.0 on jdk17 from oca.opensource.oracle.com
Downloading: Component catalog from www.graalvm.org
ComponentId              Version             Component name                Stability                     Origin
---------------------------------------------------------------------------------------------------------------------------------
espresso                 22.0.0.2            Java on Truffle               Experimental                  github.com
llvm-toolchain           22.0.0.2            LLVM.org toolchain            Experimental                  github.com
native-image             22.0.0.2            Native Image                  Experimental                  github.com
nodejs                   22.0.0.2            Graal.nodejs                  Experimental                  github.com
python                   22.0.0.2            Graal.Python                  Experimental                  github.com
R                        22.0.0.2            FastR                         Experimental                  github.com
ruby                     22.0.0.2            TruffleRuby                   Experimental                  github.com
wasm                     22.0.0.2            GraalWasm                     Experimental                  github.com
```

This repository will require the _native-image_ and _ruby_ components.

```
$ gu install native-image ruby
Downloading: Release index file from oca.opensource.oracle.com
Downloading: Component catalog for GraalVM Enterprise Edition 22.0.0.1 on jdk17 from oca.opensource.oracle.com
Downloading: Component catalog for GraalVM Enterprise Edition 22.0.0 on jdk17 from oca.opensource.oracle.com
Downloading: Component catalog from www.graalvm.org
Processing Component: Native Image
Processing Component: TruffleRuby
Processing Component: LLVM.org toolchain
Additional Components are required:
    LLVM.org toolchain (org.graalvm.llvm-toolchain, version 22.0.0.2), required by: TruffleRuby (org.graalvm.ruby)
Downloading: Component native-image: Native Image  from github.com
Downloading: Component ruby: TruffleRuby  from github.com
Downloading: Component org.graalvm.llvm-toolchain: LLVM.org toolchain  from github.com
Installing new component: LLVM.org toolchain (org.graalvm.llvm-toolchain, version 22.0.0.2)
Installing new component: Native Image (org.graalvm.native-image, version 22.0.0.2)
Installing new component: TruffleRuby (org.graalvm.ruby, version 22.0.0.2)

IMPORTANT NOTE:
---------------
The Ruby openssl C extension needs to be recompiled on your system to work with the installed libssl.
First, make sure TruffleRuby's dependencies are installed, which are described at:
  https://github.com/oracle/truffleruby/blob/master/README.md#dependencies
Then run the following command:
        /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/languages/ruby/lib/truffle/post_install_hook.sh


IMPORTANT NOTE:
---------------
Set of GraalVM components that provide language implementations have changed. The Polyglot native image and polyglot native C library may be out of sync:
- new languages may not be accessible
- removed languages may cause the native binary to fail on missing resources or libraries.
To rebuild and refresh the native binaries, use the following command:
        /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/bin/gu rebuild-images
```

Please pay attention to the output. There are two things of note here. The first is that the TruffleRuby native image
needs to recompile the OpenSSL extension for your machine. If you skip this step and try to use the _openssl_ gem in
that native image, you'll likely get a confusing error message telling you the OpenSSL library could not be found. In
the output above, we're prompted to run:

```
$ $HOME/.sdkman/candidates/java/22.0.0.2.r17-grl/languages/ruby/lib/truffle/post_install_hook.sh
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

As a warning, this rebuild requires ~8 GB RAM and takes ~15 minutes to build on an Apple M1 Pro 8-core machine running
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

The _native-library_ profile builds a Java application that computes the Haversine distance between two coordinates. The
resulting executable must be called with the four coordinates (aLatitute, aLongitude, bLatitude, bLongitude).

```
$ mvn -P native-image -D skipTests=true package
$ ./target/native-image-playground 51.507222 -0.1275 40.7127 -74.0059
5570.248509 km
```

### Profile: native-library

The _native-library_ profile builds a Java class file into a native shared library along with a corresponding set
of C header files to compile against or serve as documentation for any sort of dynamic loading (e.g., Ruby's FFI).
The native library includes a single function named `distance`, which computes the Haverstein distance between two
coordinates. The profile also compiles a launcher program written in C which loads in the shared library and passes
arguments from the launcher to the library to computer the Haverstein distance.

```
$ mvn -P native-library -D skipTests=true clean package
$ > ./target/native-library-runner 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```

### Profile: native-library-ruby

The _native-library-ruby_ profiles builds a Java class file into a native shared library along with a corresponding set
of C header files to compile against or serve as documentation for any sort of dynamic loading (e.g., Ruby's FFI).
The native library includes a single function named `distance`, which computes the Haverstein distance between two
coordinates. The `distance` function uses Truffle's polyglot API to call into TruffleRuby, which is what ultimately
implements the Haverstein distance function. The profile also compiles a launcher program written in C which loads in
the shared library and passes arguments from the launcher to the library to computer the Haverstein distance.

```
$ mvn -P native-library-ruby -D skipTests=true clean package
$ ./target/native-library-runner-ruby 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```

### Profile: native-polyglot

The _native-polyglot_ profile builds a C launcher that uses GraalVM's _libpolyglot_ and the native polyglot API to call
JavaScript and Ruby implementations of the Haverstein distance algorithm. _libpolyglot_ must be rebuilt when a new
GraalVM language is installed using `gu install`. The build is not automatically triggered and while JavaScript is
installed in GraalVM by default, _libpolyglot_ is not built by default. To rebuild _libpolyglot_, run:

```
$ gu rebuild libpolyglot
Building libpolyglot...
Warning: Option 'EnableAllSecurityServices' is deprecated and might be removed from future versions
========================================================================================================================
GraalVM Native Image: Generating 'libpolyglot'...
========================================================================================================================
[1/7] Initializing...                                                                                   (13.4s @ 0.50GB)
 Version info: 'GraalVM 22.0.0.2 Java 17 CE'
 1 user-provided feature(s)
  - org.graalvm.polyglot.nativeapi.PolyglotNativeAPIFeature
[2/7] Performing analysis...  [**********]                                                             (698.3s @ 7.57GB)
  48,046 (98.96%) of 48,551 classes reachable
  74,387 (70.59%) of 105,382 fields reachable
 236,226 (88.41%) of 267,190 methods reachable
  39,792 (14.89%) of 267,190 methods included for runtime compilation
   1,786 classes,   103 fields, and 2,493 methods registered for reflection
      67 classes,    82 fields, and    74 methods registered for JNI access
[3/7] Building universe...                                                                              (22.6s @ 4.91GB)
[4/7] Parsing methods...      [*******]                                                                 (52.1s @ 6.08GB)
[5/7] Inlining methods...     [*****]                                                                  (122.2s @ 6.64GB)
[6/7] Compiling methods...    [***************]                                                        (243.8s @ 7.78GB)
[7/7] Creating image...                                                                                 (90.4s @ 5.76GB)
 116.03MB (27.68%) for code area:  199,725 compilation units
 277.63MB (66.24%) for image heap:  34,016 classes and 2,487,972 objects
  25.45MB ( 6.07%) for other data
 419.11MB in total
------------------------------------------------------------------------------------------------------------------------
Top 10 packages in code area:                               Top 10 object types in image heap:
   6.23MB com.oracle.graal.python.builtins.modules           105.74MB byte[] for general heap data
   4.62MB com.oracle.truffle.js.builtins                      30.69MB byte[] for graph encodings
   2.32MB com.oracle.graal.python.builtins.objects.cext.capi  14.39MB java.lang.Class
   2.13MB com.oracle.graal.python.runtime                      9.11MB byte[] for java.lang.String
   1.73MB sun.security.ssl                                     8.30MB java.lang.String
   1.72MB com.oracle.truffle.js.nodes.binary                   5.89MB java.lang.Object[]
   1.70MB com.oracle.truffle.js.nodes.access                   5.45MB com.oracle.svm.graal.meta.SubstrateMethod
   1.50MB com.oracle.graal.python.builtins.modules.io          4.45MB char[]
   1.41MB com.oracle.truffle.polyglot                          4.17MB com.oracle.svm.graal.meta.SubstrateField
   1.41MB com.oracle.graal.python.builtins.objects.common      3.13MB int[]
      ... 828 additional packages                                 ... 13417 additional object types
                                           (use GraalVM Dashboard to see all)
------------------------------------------------------------------------------------------------------------------------
                      268.3s (20.1% of total time) in 193 GCs | Peak RSS: 4.03GB | CPU load: 2.67
------------------------------------------------------------------------------------------------------------------------
Produced artifacts:
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/libpolyglot.dylib (shared_lib)
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/polyglot_isolate.h (header)
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/libpolyglot.h (header)
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/polyglot_api.h (header)
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/graal_isolate.h (header)
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/polyglot_isolate_dynamic.h (header)
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/libpolyglot_dynamic.h (header)
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/polyglot_api_dynamic.h (header)
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/graal_isolate_dynamic.h (header)
 /Users/nirvdrum/.sdkman/candidates/java/22.0.0.2.r17-grl/lib/polyglot/libpolyglot.build_artifacts.txt
========================================================================================================================
Finished generating 'libpolyglot' in 22m 9s.
```

As a warning, building _libpolyglot_ is a computationally heavy process. It will take a long time and you may need to
close applications in order to afford the builder more memory to operate. Fortunately, the image only needs to rebuilt
when installing a new language or upgrading GraalVM. In contract, the other profiles need to be rebuilt every time
there's a change to the `@CEntryPoint` set of methods.

Once _libployglot_ is built, you can compile and invoke the C launcher. There is no Java code or build process necessary
for this profile. _libpolyglot_ exposes all of the functionality needed to call into a Truffle language. Moreover, the
native polyglot API allows sending polyglot messages to a Truffle language without needing to use Truffle Java-based API.
The launcher script provides implementations of the Haverstein distance formula in JavaScript and Ruby, mostly to
illustrate how _libpolyglot_ supports multiple languages. When invoking the launcher you must provide the language to
use as the first argument. Acceptable values are "js" and "ruby" (case-sensitive).

```
$ mvn -P native-polyglot -D skipTests=true clean package
$ ./target/native-polyglot ruby 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
$ ./target/native-polyglot js 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```

### Profile: jni-libjvm-polyglot

The _jni-libjvm-polyglot_ profile builds a C++ launcher that uses JNI to load a copy of the JVM as a shared
library. Once loaded, the launcher program uses Truffle's Java-based polyglot API to call JavaScript and Ruby
implementations of the Haverstein distance algorithm. While this build does not use code built by GraalVM's native image
tool, it still requires using `gu install ruby` to install the TruffleRuby JAR, which is then loaded for execution of
any Ruby code.

```
$ mvn -P jni-libjvm-polyglot -D skipTests=true clean package
$ ./target/jni-runner ruby 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
$ ./target/jni-runner js 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```

### Profile: jni-native

The _jni-native_ profile builds a shared library using native image that embeds both Graal.js and TruffleRuby, allowing
execution of code in both languages using the Truffle polyglot API. The profile also builds a C++ launcher that uses
JNI to execute code within that library just as if it were libjvm, the standard approach to invoking Java code from an
external application. Once loaded, the launcher program uses Truffle's Java-based polyglot API to call JavaScript and Ruby
implementations of the Haverstein distance algorithm. While this build does not use code built by GraalVM's native image
tool, it still requires using `gu install ruby` to install the TruffleRuby JAR, which is then loaded for execution of
any Ruby code.

```
$ mvn -P jni-native -D skipTests=true clean package
$ ./target/jni-runner ruby 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
$ ./target/jni-runner js 51.507222 -0.1275 40.7127 -74.0059
5570.25 km
```
