#include <benchmark/benchmark.h>
#include <jni.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "libbenchmark-runner.h"
#include "polyglot_scripts.h"

#ifdef DEBUG
  #define CHECK_EXCEPTION(env)                        \
    ({                                                \
      bool exceptionOccurred = env->ExceptionCheck(); \
      if (exceptionOccurred) {                        \
        env->ExceptionDescribe();                     \
      }                                               \
    })
#else
  #define CHECK_EXCEPTION(env) 0
#endif

enum class exec_mode : long {
  CENTRY_JAVA_DISTANCE = 0,
  CENTRY_RUBY_DISTANCE,
  CENTRY_POLYGLOT_DISTANCE,
  JNI_JAVA_DISTANCE,
  JNI_RUBY_DISTANCE,
  JNI_POLYGLOT_DISTANCE
};

BENCHMARK_MAIN();

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
    isolate_thread = create_isolate();
  }

  distance_ruby(isolate_thread, A_LAT, A_LONG, B_LAT, B_LONG);
  distance_polyglot(isolate_thread, (char*)"ruby",
                    (char*)RUBY_HAVERSINE_DISTANCE, A_LAT, A_LONG, B_LAT,
                    B_LONG);
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
    options[0].optionString = (char*)"-Djava.class.path=/usr/lib/java";

    vm_args.version = JNI_VERSION_10;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

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
  distance_ruby(isolate_thread, a_lat, a_long, b_lat, b_long);

  for (auto _ : state) {
    distance_ruby(isolate_thread, a_lat, a_long, b_lat, b_long);
  }
}

static void BM_CEntryPolyglotRubyDistance(benchmark::State& state, double a_lat,
                                          double a_long, double b_lat,
                                          double b_long) {
  distance_polyglot(isolate_thread, (char*)"ruby",
                    (char*)RUBY_HAVERSINE_DISTANCE, a_lat, a_long, b_lat,
                    b_long);

  for (auto _ : state) {
    distance_polyglot(isolate_thread, (char*)"ruby",
                      (char*)RUBY_HAVERSINE_DISTANCE, a_lat, a_long, b_lat,
                      b_long);
  }
}

static void BM_CEntryPolyglotJsDistance(benchmark::State& state, double a_lat,
                                        double a_long, double b_lat,
                                        double b_long) {
  distance_polyglot(isolate_thread, (char*)"js", (char*)JS_HAVERSINE_DISTANCE,
                    a_lat, a_long, b_lat, b_long);

  for (auto _ : state) {
    distance_polyglot(isolate_thread, (char*)"js", (char*)JS_HAVERSINE_DISTANCE,
                      a_lat, a_long, b_lat, b_long);
  }
}

static void BM_JNIJavaDistance(benchmark::State& state, double a_lat,
                               double a_long, double b_lat, double b_long) {
  jmethodID javaDistanceMethod =
      env->GetStaticMethodID(javaDistanceClass, "distance",
                             "(Lorg/graalvm/nativeimage/IsolateThread;DDDD)D");

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

  for (auto _ : state) {
    jobject truffle_result =
        env->CallObjectMethod(truffle_distance, executeMethod, distanceArgs);
    CHECK_EXCEPTION(env);
    env->CallDoubleMethod(truffle_result, asDoubleMethod);
  }
}

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
BENCHMARK_CAPTURE(BM_CEntryPolyglotRubyDistance, placeholder, A_LAT, A_LONG,
                  B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (Ruby)")
    ->Setup(DoCEntrySetup)
    ->Teardown(DoCEntryTeardown);
BENCHMARK_CAPTURE(BM_CEntryPolyglotJsDistance, placeholder, A_LAT, A_LONG,
                  B_LAT, B_LONG)
    ->Name("@CEntryPoint: Polyglot (JS)")
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

void centry_function(const exec_mode mode, const char* language, const char* code,
                     double a_lat, double a_long, double b_lat, double b_long) {
  graal_isolatethread_t* thread = create_isolate();

  switch (mode) {
    case CENTRY_JAVA_DISTANCE: {
      std::printf("%.2f km\n", distance(thread, a_lat, a_long, b_lat, b_long));
      break;
    }

    case CENTRY_RUBY_DISTANCE: {
      std::printf("%.2f km\n", distance_ruby(thread, a_lat, a_long, b_lat, b_long));
      break;
    }

    case CENTRY_POLYGLOT_DISTANCE: {
      std::printf("%.2f km\n",
             distance_polyglot(thread, (char*)language, (char*)code, a_lat,
                               a_long, b_lat, b_long));
      break;
    }

    default: {
      std::fprintf(std::stderr, "Unexpected mode encountered for CEntry calls: %d\n",
              mode);
      std::exit(1);
    }
  }

  tear_down_isolate(thread);
}

void jni_function(const exec_mode mode, const char* language, const char* code,
                  double a_lat, double a_long, double b_lat, double b_long) {
  JavaVM* jvm = nullptr;
  JNIEnv* env = nullptr;
  JavaVMInitArgs vm_args;
  JavaVMOption* options = new JavaVMOption[1];
  options[0].optionString = (char*)"-Djava.class.path=/usr/lib/java";

  vm_args.version = JNI_VERSION_10;
  vm_args.nOptions = 1;
  vm_args.options = options;
  vm_args.ignoreUnrecognized = false;

  JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
  delete[] options;

  jclass doubleClass = env->FindClass("java/lang/Double");
  jclass stringClass = env->FindClass("java/lang/String");
  jclass contextClass = env->FindClass("org/graalvm/polyglot/Context");
  jclass builderClass = env->FindClass("org/graalvm/polyglot/Context$Builder");
  jclass valueClass = env->FindClass("org/graalvm/polyglot/Value");
  javaDistanceClass = env->FindClass("com/shopify/truffleruby/NativeLibrary");
  rubyDistanceClass =
      env->FindClass("com/shopify/truffleruby/NativeLibraryRuby");

  // Create an empty java.lang.String[].
  jstring initialElement = env->NewStringUTF("");
  jobjectArray emptyArgs = env->NewObjectArray(0, stringClass, initialElement);

  // java.lang.Double methods.
  jmethodID doubleValueOfMethod =
      env->GetStaticMethodID(doubleClass, "valueOf", "(D)Ljava/lang/Double;");

  // org.graalvm.polyglot.Context methods.
  jmethodID evalMethod =
      env->GetMethodID(contextClass, "eval",
                       "(Ljava/lang/String;Ljava/lang/CharSequence;)Lorg/"
                       "graalvm/polyglot/Value;");
  jmethodID newBuilderMethod = env->GetStaticMethodID(
      contextClass, "newBuilder",
      "([Ljava/lang/String;)Lorg/graalvm/polyglot/Context$Builder;");

  // org.graalvm.polyglot.Context.Builder methods.
  jmethodID allowExperimentalOptionsMethod =
      env->GetMethodID(builderClass, "allowExperimentalOptions",
                       "(Z)Lorg/graalvm/polyglot/Context$Builder;");
  jmethodID buildMethod = env->GetMethodID(builderClass, "build",
                                           "()Lorg/graalvm/polyglot/Context;");
  jmethodID optionMethod =
      env->GetMethodID(builderClass, "option",
                       "(Ljava/lang/String;Ljava/lang/String;)Lorg/graalvm/"
                       "polyglot/Context$Builder;");

  // org.graalvm.polyglot.Value methods.
  jmethodID asDoubleMethod = env->GetMethodID(valueClass, "asDouble", "()D");
  jmethodID executeMethod =
      env->GetMethodID(valueClass, "execute",
                       "([Ljava/lang/Object;)Lorg/graalvm/polyglot/Value;");

  // Build a polyglot context.
  jobject builder =
      env->CallStaticObjectMethod(contextClass, newBuilderMethod, emptyArgs);

  builder =
      env->CallObjectMethod(builder, allowExperimentalOptionsMethod, JNI_TRUE);
  builder = env->CallObjectMethod(builder, optionMethod,
                                  env->NewStringUTF("ruby.no-home-provided"),
                                  env->NewStringUTF("true"));
  context = env->CallObjectMethod(builder, buildMethod);

  CHECK_EXCEPTION(env);

#ifdef DEBUG
  std::cout << "Language: " << language << "\n";
  std::cout << "Code: " << code << "\n";
#endif

  switch (mode) {
    case JNI_JAVA_DISTANCE: {
      jmethodID javaDistanceMethod = env->GetStaticMethodID(
          javaDistanceClass, "distance",
          "(Lorg/graalvm/nativeimage/IsolateThread;DDDD)D");

      jdouble distance =
          env->CallStaticDoubleMethod(javaDistanceClass, javaDistanceMethod,
                                      nullptr, a_lat, a_long, b_lat, b_long);
      std::printf("%.2f km\n", distance);

      break;
    }

    case JNI_RUBY_DISTANCE: {
      jmethodID rubyDistanceMethod = env->GetStaticMethodID(
          rubyDistanceClass, "distance",
          "(Lorg/graalvm/nativeimage/IsolateThread;DDDD)D");

      jdouble distance =
          env->CallStaticDoubleMethod(rubyDistanceClass, rubyDistanceMethod,
                                      nullptr, a_lat, a_long, b_lat, b_long);
      CHECK_EXCEPTION(env);
      std::printf("%.2f km\n", distance);

      break;
    }

    case JNI_POLYGLOT_DISTANCE: {
      std::printf("HERE I AM\n");
      jobject truffle_distance = env->CallObjectMethod(
          context, evalMethod, env->NewStringUTF(language),
          env->NewStringUTF(code));

      jobjectArray distance_args = env->NewObjectArray(4, doubleClass, 0);
      env->SetObjectArrayElement(
          distance_args, 0,
          env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, a_lat));
      env->SetObjectArrayElement(distance_args, 1,
                                 env->CallStaticObjectMethod(
                                     doubleClass, doubleValueOfMethod, a_long));
      env->SetObjectArrayElement(
          distance_args, 2,
          env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, b_lat));
      env->SetObjectArrayElement(distance_args, 3,
                                 env->CallStaticObjectMethod(
                                     doubleClass, doubleValueOfMethod, b_long));

      for (int i = 0; i < 10; i++) {
        jobject truffle_result = env->CallObjectMethod(
            truffle_distance, executeMethod, distance_args);

        CHECK_EXCEPTION(env);

        jdouble distance =
            env->CallDoubleMethod(truffle_result, asDoubleMethod);
        std::printf("%.2f km\n", distance);
      }

      break;
    }

    default: {
      std::fprintf(std::stderr, "Unexpected mode encountered for CEntry calls: %d\n",
              mode);
      std::exit(1);
    }
  }

  jvm->DestroyJavaVM();
}

int main2(int argc, char** argv) {
  if (argc != 7) {
    std::fprintf(std::stderr,
            "Usage: %s <exec_mode> <language>[js|ruby] <lat1> <long1> <lat2> "
            "<long2>\n",
            argv[0]);
    std::exit(1);
  }

  const long mode = static_cast<exec_mode>(std::strtol(argv[1], nullptr, 10));
  char* language = argv[2];
  double a_lat = std::strtod(argv[3], nullptr);
  double a_long = std::strtod(argv[4], nullptr);
  double b_lat = std::strtod(argv[5], nullptr);
  double b_long = std::strtod(argv[6], nullptr);

  const char* code = nullptr;
  switch (language[0]) {
    case 'j':
      code = JS_HAVERSINE_DISTANCE;
      break;
    case 'r':
      code = RUBY_HAVERSINE_DISTANCE;
      break;
    default:
      std::fprintf(std::stderr, "Haversine distance code is not provided for '%s'\n",
              language);
      std::exit(1);
  }

  switch (mode) {
    case CENTRY_JAVA_DISTANCE:
    case CENTRY_RUBY_DISTANCE:
    case CENTRY_POLYGLOT_DISTANCE: {
      centry_function(mode, language, code, a_lat, a_long, b_lat, b_long);
      break;
    }

    case JNI_JAVA_DISTANCE:
    case JNI_RUBY_DISTANCE:
    case JNI_POLYGLOT_DISTANCE: {
      jni_function(mode, language, code, a_lat, a_long, b_lat, b_long);
      break;
    }
  }

  return 0;
}