#include <benchmark/benchmark.h>
#include <jni.h>
#include <threads.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "benchmark-utils.h"
#include "graal_isolate.h"
#include "haversine.h"
#include "libbenchmark-runner.h"
#include "polyglot_scripts.h"

graal_isolate_t* isolate = nullptr;
graal_isolatethread_t* isolate_thread = nullptr;

JavaVM* jvm = nullptr;
JNIEnv* env = nullptr;
jobject context;
jclass javaDistanceClass;
jclass rubyDistanceClass;
jmethodID evalMethod;
jclass doubleClass;
jmethodID doubleValueOfMethod;
jmethodID asDoubleMethod;
jmethodID executeMethod;

volatile double A_LAT = 51.507222;
volatile double A_LONG = -0.1275;
volatile double B_LAT = 40.7127;
volatile double B_LONG = -74.0059;

static void DoCEntrySetup(const benchmark::State& state) {
  if (isolate_thread == nullptr) {
#ifdef DUMP_GRAAL_GRAPHS
    // This is a big hack. There is no guarantee that arguments can be passed
    // to the isolate this way. The `_reserved_` prefix on the field names is
    // a clear indication that none of this should be relied upon. With that
    // said, barring an official way of passing JVM options, this approach does
    // work with GraalVM 22.1.0 Community Edition.

    char* args[2] = {(char*)"-XX:+ParseRuntimeOptions",
                     (char*)"-XX:Dump=Truffle:1"};
    graal_create_isolate_params_t isolate_params;
    isolate_params.version = 3;
    isolate_params._reserved_1 = 2;
    isolate_params._reserved_2 = args;

    if (graal_create_isolate(&isolate_params, &isolate, &isolate_thread) != 0) {
      std::cerr << "initialization error\n";
      std::exit(1);
    }
#else
    if (graal_create_isolate(NULL, &isolate, &isolate_thread) != 0) {
      std::cerr << "initialization error\n";
      std::exit(1);
    }
#endif
  }
}

static void DoCEntryTeardown(const benchmark::State& state) {
#ifndef REUSE_CONTEXT
  tear_down_isolate(isolate_thread);
  isolate_thread = nullptr;
#endif
}

static void DoJNISetup(const benchmark::State& state) {
  if (jvm == nullptr) {
    JavaVMInitArgs vm_args;
    JavaVMOption* options = new JavaVMOption[1];
    options[0].optionString = (char*)"-XX:Dump=Truffle:1";

    vm_args.version = JNI_VERSION_10;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

#ifdef DUMP_GRAAL_GRAPHS
    vm_args.nOptions = 1;
#else
    vm_args.nOptions = 0;
#endif

    JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
    delete[] options;

    doubleClass = env->FindClass("java/lang/Double");
    jclass stringClass = env->FindClass("java/lang/String");
    jclass contextClass = env->FindClass("org/graalvm/polyglot/Context");
    jclass builderClass =
        env->FindClass("org/graalvm/polyglot/Context$Builder");
    jclass valueClass = env->FindClass("org/graalvm/polyglot/Value");
    javaDistanceClass = env->FindClass("com/shopify/truffleruby/NativeLibrary");
    rubyDistanceClass =
        env->FindClass("com/shopify/truffleruby/NativeLibraryRuby");

    // Create an empty java.lang.String[].
    jstring initialElement = env->NewStringUTF("");
    jobjectArray emptyArgs =
        env->NewObjectArray(0, stringClass, initialElement);

    // java.lang.Double methods.
    doubleValueOfMethod =
        env->GetStaticMethodID(doubleClass, "valueOf", "(D)Ljava/lang/Double;");

    // org.graalvm.polyglot.Context methods.
    evalMethod = env->GetMethodID(contextClass, "eval",
                                  "(Ljava/lang/String;Ljava/lang/"
                                  "CharSequence;)Lorg/graalvm/polyglot/Value;");
    jmethodID newBuilderMethod = env->GetStaticMethodID(
        contextClass, "newBuilder",
        "([Ljava/lang/String;)Lorg/graalvm/polyglot/Context$Builder;");

    // org.graalvm.polyglot.Context.Builder methods.
    jmethodID allowExperimentalOptionsMethod =
        env->GetMethodID(builderClass, "allowExperimentalOptions",
                         "(Z)Lorg/graalvm/polyglot/Context$Builder;");
    jmethodID buildMethod = env->GetMethodID(
        builderClass, "build", "()Lorg/graalvm/polyglot/Context;");
    jmethodID optionMethod =
        env->GetMethodID(builderClass, "option",
                         "(Ljava/lang/String;Ljava/lang/String;)Lorg/graalvm/"
                         "polyglot/Context$Builder;");

    // org.graalvm.polyglot.Value methods.
    asDoubleMethod = env->GetMethodID(valueClass, "asDouble", "()D");
    executeMethod =
        env->GetMethodID(valueClass, "execute",
                         "([Ljava/lang/Object;)Lorg/graalvm/polyglot/Value;");

    // Build a polyglot context.
    jobject builder =
        env->CallStaticObjectMethod(contextClass, newBuilderMethod, emptyArgs);
    builder = env->CallObjectMethod(builder, allowExperimentalOptionsMethod,
                                    JNI_TRUE);
    builder = env->CallObjectMethod(builder, optionMethod,
                                    env->NewStringUTF("ruby.no-home-provided"),
                                    env->NewStringUTF("true"));
    context = env->CallObjectMethod(builder, buildMethod);
  }
}

static void DoJNITeardown(const benchmark::State& state) {
#ifndef REUSE_CONTEXT
  if (jvm != nullptr) {
    jvm->DestroyJavaVM();
    jvm = nullptr;
    env = nullptr;
  }
#endif
}

static void BM_CEntryJavaDistance(benchmark::State& state, double a_lat,
                                  double a_long, double b_lat, double b_long) {
  for (auto _ : state) {
    distance(isolate_thread, a_lat, a_long, b_lat, b_long);
  }
}

static void BM_CEntryRubyDistance(benchmark::State& state, double a_lat,
                                  double a_long, double b_lat, double b_long) {
  // Parse and evaluate the guest code once before entering the timing loop.
  distance_ruby(isolate_thread, a_lat, a_long, b_lat, b_long);

  for (auto _ : state) {
    distance_ruby(isolate_thread, a_lat, a_long, b_lat, b_long);
  }
}

static void BM_CEntryPolyglotDistance(benchmark::State& state,
                                      const char* language, const char* code,
                                      double a_lat, double a_long, double b_lat,
                                      double b_long) {
  // Parse and evaluate the guest code once before entering the timing loop.
  distance_polyglot_no_cache(isolate_thread, (char*)language, (char*)code,
                             a_lat, a_long, b_lat, b_long);

  for (auto _ : state) {
    distance_polyglot_no_cache(isolate_thread, (char*)language, (char*)code,
                               a_lat, a_long, b_lat, b_long);
  }
}

static void BM_CEntryPolyglotDistanceNoParseCache(benchmark::State& state,
                                                  const char* language,
                                                  const char* code,
                                                  double a_lat, double a_long,
                                                  double b_lat, double b_long) {
  // Parse and evaluate the guest code once before entering the timing loop.
  distance_polyglot_no_parse_cache(isolate_thread, (char*)language, (char*)code,
                                   a_lat, a_long, b_lat, b_long);

  for (auto _ : state) {
    distance_polyglot_no_parse_cache(isolate_thread, (char*)language,
                                     (char*)code, a_lat, a_long, b_lat, b_long);
  }
}

static void BM_CEntryPolyglotDistanceThreadSafeParseCache(
    benchmark::State& state, const char* language, const char* code,
    double a_lat, double a_long, double b_lat, double b_long) {
  // Parse and evaluate the guest code once before entering the timing loop.
  distance_polyglot_thread_safe_parse_cache(isolate_thread, (char*)language,
                                            (char*)code, a_lat, a_long, b_lat,
                                            b_long);

  for (auto _ : state) {
    distance_polyglot_thread_safe_parse_cache(isolate_thread, (char*)language,
                                              (char*)code, a_lat, a_long, b_lat,
                                              b_long);
  }
}

static void BM_CEntryPolyglotDistanceThreadUnsafeParseCache(
    benchmark::State& state, const char* language, const char* code,
    double a_lat, double a_long, double b_lat, double b_long) {
  // Parse and evaluate the guest code once before entering the timing loop.
  distance_polyglot_thread_unsafe_parse_cache(isolate_thread, (char*)language,
                                              (char*)code, a_lat, a_long, b_lat,
                                              b_long);

  for (auto _ : state) {
    distance_polyglot_thread_unsafe_parse_cache(isolate_thread, (char*)language,
                                                (char*)code, a_lat, a_long,
                                                b_lat, b_long);
  }
}

static void BM_JNIJavaDistance(benchmark::State& state, double a_lat,
                               double a_long, double b_lat, double b_long) {
  jmethodID javaDistanceMethod =
      env->GetStaticMethodID(javaDistanceClass, "distance",
                             "(Lorg/graalvm/nativeimage/IsolateThread;DDDD)D");

  // Parse and evaluate the guest code once before entering the timing loop.
  env->CallStaticDoubleMethod(javaDistanceClass, javaDistanceMethod, nullptr,
                              a_lat, a_long, b_lat, b_long);

  for (auto _ : state) {
    env->CallStaticDoubleMethod(javaDistanceClass, javaDistanceMethod, nullptr,
                                a_lat, a_long, b_lat, b_long);
  }
}

static void BM_JNIRubyDistance(benchmark::State& state, double a_lat,
                               double a_long, double b_lat, double b_long) {
  jmethodID rubyDistanceMethod =
      env->GetStaticMethodID(rubyDistanceClass, "distance",
                             "(Lorg/graalvm/nativeimage/IsolateThread;DDDD)D");

  // Parse and evaluate the guest code once before entering the timing loop.
  env->CallStaticDoubleMethod(rubyDistanceClass, rubyDistanceMethod, nullptr,
                              a_lat, a_long, b_lat, b_long);

  for (auto _ : state) {
    env->CallStaticDoubleMethod(rubyDistanceClass, rubyDistanceMethod, nullptr,
                                a_lat, a_long, b_lat, b_long);
  }
}

static void BM_JNIPolyglotDistance(benchmark::State& state,
                                   const char* language, const char* code,
                                   double a_lat, double a_long, double b_lat,
                                   double b_long) {
  jobject truffle_distance =
      env->CallObjectMethod(context, evalMethod, env->NewStringUTF(language),
                            env->NewStringUTF(code));
  CHECK_EXCEPTION(env);

  jobjectArray distanceArgs = env->NewObjectArray(4, doubleClass, 0);
  env->SetObjectArrayElement(
      distanceArgs, 0,
      env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, a_lat));
  env->SetObjectArrayElement(
      distanceArgs, 1,
      env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, a_long));
  env->SetObjectArrayElement(
      distanceArgs, 2,
      env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, b_lat));
  env->SetObjectArrayElement(
      distanceArgs, 3,
      env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, b_long));

  // Parse and evaluate the guest code once before entering the timing loop.
  jobject truffle_result =
      env->CallObjectMethod(truffle_distance, executeMethod, distanceArgs);
  CHECK_EXCEPTION(env);
  env->CallDoubleMethod(truffle_result, asDoubleMethod);

  for (auto _ : state) {
    jobject truffle_result =
        env->CallObjectMethod(truffle_distance, executeMethod, distanceArgs);
    CHECK_EXCEPTION(env);
    env->CallDoubleMethod(truffle_result, asDoubleMethod);
  }
}

static void BM_CppDistance(benchmark::State& state, double a_lat, double a_long,
                           double b_lat, double b_long) {
  for (auto _ : state) {
    haversine_distance(a_lat, a_long, b_lat, b_long);
  }
}

BENCHMARK_CAPTURE(BM_CppDistance, placeholder, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("C++");

BENCHMARK_CAPTURE(BM_CEntryJavaDistance, placeholder, A_LAT, A_LONG, B_LAT,
                  B_LONG)
    ->Name("@CEntryPoint: Java")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_CEntryRubyDistance, placeholder, A_LAT, A_LONG, B_LAT,
                  B_LONG)
    ->Name("@CEntryPoint: Ruby")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_CEntryPolyglotDistance, placeholder, "ruby",
                  RUBY_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (Ruby)")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_CEntryPolyglotDistance, placeholder, "js",
                  JS_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (JS)")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_CEntryPolyglotDistanceNoParseCache, placeholder, "ruby",
                  RUBY_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (Ruby) - No Parse Cache")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_CEntryPolyglotDistanceNoParseCache, placeholder, "js",
                  JS_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (JS) - No Parse Cache")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_CEntryPolyglotDistanceThreadSafeParseCache, placeholder,
                  "ruby", RUBY_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (Ruby) - Safe Parse Cache")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_CEntryPolyglotDistanceThreadSafeParseCache, placeholder,
                  "js", JS_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (JS) - Safe Parse Cache")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_CEntryPolyglotDistanceThreadUnsafeParseCache, placeholder,
                  "ruby", RUBY_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (Ruby) - Unsafe Parse Cache")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_CEntryPolyglotDistanceThreadUnsafeParseCache, placeholder,
                  "js", JS_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (JS) - Unsafe Parse Cache")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);

BENCHMARK_CAPTURE(BM_JNIJavaDistance, placeholder, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("JNI: Java")
    ->Setup(DoJNISetup)
    ->Teardown(DoJNITeardown);

BENCHMARK_CAPTURE(BM_JNIRubyDistance, placeholder, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("JNI: Ruby")
    ->Setup(DoJNISetup)
    ->Teardown(DoJNITeardown);

BENCHMARK_CAPTURE(BM_JNIPolyglotDistance, placeholder, "ruby",
                  RUBY_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("JNI: Polyglot (Ruby)")
    ->Setup(DoJNISetup)
    ->Teardown(DoJNITeardown);

BENCHMARK_CAPTURE(BM_JNIPolyglotDistance, placeholder, "js",
                  JS_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT, B_LONG)
    ->Name("JNI: Polyglot (JS)")
    ->Setup(DoJNISetup)
    ->Teardown(DoJNITeardown);

BENCHMARK_MAIN();