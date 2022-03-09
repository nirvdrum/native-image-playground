#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <benchmark/benchmark.h>
#include "polyglot_scripts.h"
#include "libbenchmark-runner.h"

#ifdef DEBUG
  #define CHECK_EXCEPTION(env) ({ bool exceptionOccurred = env->ExceptionCheck(); if (exceptionOccurred) { env->ExceptionDescribe(); } })
#else
  #define CHECK_EXCEPTION(env) 0
#endif

using namespace std;

enum exec_mode {
  CENTRY_JAVA_DISTANCE,
  CENTRY_RUBY_DISTANCE,
  CENTRY_POLYGLOT_DISTANCE,
  JNI_JAVA_DISTANCE,
  JNI_RUBY_DISTANCE,
  JNI_POLYGLOT_DISTANCE
};

BENCHMARK_MAIN();

graal_isolatethread_t *isolate_thread;

static void DoCEntrySetup(const benchmark::State& state) {
  cout << "Setup\n";
  if (NULL == isolate_thread) {
    isolate_thread = create_isolate();
  }
  distance_ruby(isolate_thread, 51.507222, -0.1275, 40.7127, -74.0059);
  distance_polyglot(isolate_thread, (char *) "ruby", (char *) RUBY_HAVERSINE_DISTANCE, 51.507222, -0.1275, 40.7127, -74.0059);
}

static void DoCEntryTeardown(const benchmark::State& state) {
  //cout << "Teardown\n";
  //tear_down_isolate(isolate_thread);
}

static void BM_CEntryJavaDistance(benchmark::State& state) {
  for (auto _ : state) {
    distance(isolate_thread, 51.507222, -0.1275, 40.7127, -74.0059);
  }
}

static void BM_CEntryRubyDistance(benchmark::State& state) {
  distance_ruby(isolate_thread, 51.507222, -0.1275, 40.7127, -74.0059);

  for (auto _ : state) {
    distance_ruby(isolate_thread, 51.507222, -0.1275, 40.7127, -74.0059);
  }
}

static void BM_CEntryPolyglotRubyDistance(benchmark::State& state) {
  //distance_polyglot(isolate_thread, (char *) "ruby", (char *) RUBY_HAVERSINE_DISTANCE, 51.507222, -0.1275, 40.7127, -74.0059);

  for (auto _ : state) {
    distance_polyglot(isolate_thread, (char *) "ruby", (char *) RUBY_HAVERSINE_DISTANCE, 51.507222, -0.1275, 40.7127, -74.0059);
  }
}

static void BM_CEntryPolyglotJsDistance(benchmark::State& state) {
  //distance_polyglot(isolate_thread, (char *) "js", (char *) JS_HAVERSINE_DISTANCE, 51.507222, -0.1275, 40.7127, -74.0059);

  for (auto _ : state) {
    distance_polyglot(isolate_thread, (char *) "js", (char *) JS_HAVERSINE_DISTANCE, 51.507222, -0.1275, 40.7127, -74.0059);
  }
}

BENCHMARK(BM_CEntryJavaDistance)->Setup(DoCEntrySetup)->Teardown(DoCEntryTeardown);
BENCHMARK(BM_CEntryRubyDistance)->MinTime(30.0)->Setup(DoCEntrySetup)->Teardown(DoCEntryTeardown);
BENCHMARK(BM_CEntryPolyglotRubyDistance)->MinTime(30.0)->Setup(DoCEntrySetup)->Teardown(DoCEntryTeardown);
BENCHMARK(BM_CEntryPolyglotJsDistance)->MinTime(30.0)->Setup(DoCEntrySetup)->Teardown(DoCEntryTeardown);

void centry_function(exec_mode mode, const char* language, const char* code, double a_lat, double a_long, double b_lat, double b_long) {
  graal_isolatethread_t *thread = create_isolate();

  switch(mode) {
    case CENTRY_JAVA_DISTANCE: {
      printf("%.2f km\n", distance(thread, a_lat, a_long, b_lat, b_long));
      break;
    }

    case CENTRY_RUBY_DISTANCE: {
      printf("%.2f km\n", distance_ruby(thread, a_lat, a_long, b_lat, b_long));
      break;
    }

    case CENTRY_POLYGLOT_DISTANCE: {
      printf("%.2f km\n", distance_polyglot(thread, (char *) language, (char *) code, a_lat, a_long, b_lat, b_long));
      break;
    }

    default: {
      fprintf(stderr, "Unexpected mode encountered for CEntry calls: %d\n", mode);
      exit(1); 
    }
  }

  tear_down_isolate(thread);
}

void jni_function(exec_mode mode, const char* language, const char* code, double a_lat, double a_long, double b_lat, double b_long) {
  JavaVM *jvm;
  JNIEnv *env;
  JavaVMInitArgs vm_args;
  JavaVMOption* options = new JavaVMOption[1];
  options[0].optionString = (char * ) "-Djava.class.path=/usr/lib/java";

  vm_args.version = JNI_VERSION_10;
  vm_args.nOptions = 1;
  vm_args.options = options;
  vm_args.ignoreUnrecognized = false;

  JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
  delete[] options;

  jclass doubleClass  = env->FindClass("java/lang/Double");
  jclass stringClass  = env->FindClass("java/lang/String");
  jclass contextClass = env->FindClass("org/graalvm/polyglot/Context");
  jclass builderClass = env->FindClass("org/graalvm/polyglot/Context$Builder");
  jclass valueClass   = env->FindClass("org/graalvm/polyglot/Value");
  jclass javaDistanceClass = env->FindClass("com/shopify/truffleruby/NativeLibrary");
  jclass rubyDistanceClass = env->FindClass("com/shopify/truffleruby/NativeLibraryRuby");

  // Create an empty java.lang.String[].
  jstring initialElement = env->NewStringUTF("");
  jobjectArray emptyArgs = env->NewObjectArray(0, stringClass, initialElement);

  // java.lang.Double methods.
  jmethodID doubleValueOfMethod = env->GetStaticMethodID(doubleClass, "valueOf", "(D)Ljava/lang/Double;");

  // org.graalvm.polyglot.Context methods.
  jmethodID evalMethod = env->GetMethodID(contextClass, "eval", "(Ljava/lang/String;Ljava/lang/CharSequence;)Lorg/graalvm/polyglot/Value;");
  jmethodID newBuilderMethod = env->GetStaticMethodID(contextClass, "newBuilder", "([Ljava/lang/String;)Lorg/graalvm/polyglot/Context$Builder;");

  // org.graalvm.polyglot.Context.Builder methods.
  jmethodID allowExperimentalOptionsMethod = env->GetMethodID(builderClass, "allowExperimentalOptions", "(Z)Lorg/graalvm/polyglot/Context$Builder;");
  jmethodID buildMethod = env->GetMethodID(builderClass, "build", "()Lorg/graalvm/polyglot/Context;");
  jmethodID optionMethod = env->GetMethodID(builderClass, "option", "(Ljava/lang/String;Ljava/lang/String;)Lorg/graalvm/polyglot/Context$Builder;");

  // org.graalvm.polyglot.Value methods.
  jmethodID asDoubleMethod = env->GetMethodID(valueClass, "asDouble", "()D");
  jmethodID executeMethod = env->GetMethodID(valueClass, "execute", "([Ljava/lang/Object;)Lorg/graalvm/polyglot/Value;");

  // com.shopify.truffleruby.NativeLibraryRuby methods.
  //jmethodID rubyDistanceMethod = env->GetMethodID(rubyDistanceClass, "distance", "(Lorg/graalvm/nativeimage/IsolateThread;DDDD)D");

  // Build a polyglot context.
  jobject builder = env->CallStaticObjectMethod(contextClass, newBuilderMethod, emptyArgs);

  builder = env->CallObjectMethod(builder, allowExperimentalOptionsMethod, JNI_TRUE);
  builder = env->CallObjectMethod(builder, optionMethod, env->NewStringUTF("ruby.no-home-provided"), env->NewStringUTF("true"));
  jobject context = env->CallObjectMethod(builder, buildMethod);

  CHECK_EXCEPTION(env);

#ifdef DEBUG
  cout << "Language: " << language << "\n";
  cout << "Code: " << code << "\n";
#endif

  switch(mode) {
    case JNI_JAVA_DISTANCE: {
      jmethodID javaDistanceMethod = env->GetStaticMethodID(javaDistanceClass, "distance", "(Lorg/graalvm/nativeimage/IsolateThread;DDDD)D");

      jdouble distance = env->CallStaticDoubleMethod(javaDistanceClass, javaDistanceMethod, NULL, a_lat, a_long, b_lat, b_long);
      printf("%.2f km\n", distance);
            
      break;
    }

    case JNI_RUBY_DISTANCE: {
      jmethodID rubyDistanceMethod = env->GetStaticMethodID(rubyDistanceClass, "distance", "(Lorg/graalvm/nativeimage/IsolateThread;DDDD)D");

      jdouble distance = env->CallStaticDoubleMethod(rubyDistanceClass, rubyDistanceMethod, NULL, a_lat, a_long, b_lat, b_long);
      CHECK_EXCEPTION(env);
      printf("%.2f km\n", distance);

      break;
    }

    case JNI_POLYGLOT_DISTANCE: {
      jobject rubyProc = env->CallObjectMethod(context, evalMethod, env->NewStringUTF(language), env->NewStringUTF(code));

      jobjectArray distanceArgs = env->NewObjectArray(4, doubleClass, 0);
      env->SetObjectArrayElement(distanceArgs, 0, env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, a_lat));
      env->SetObjectArrayElement(distanceArgs, 1, env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, a_long));
      env->SetObjectArrayElement(distanceArgs, 2, env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, b_lat));
      env->SetObjectArrayElement(distanceArgs, 3, env->CallStaticObjectMethod(doubleClass, doubleValueOfMethod, b_long));

      jobject rubyResult = env->CallObjectMethod(rubyProc, executeMethod, distanceArgs);

      CHECK_EXCEPTION(env);

      jdouble distance = env->CallDoubleMethod(rubyResult, asDoubleMethod);
      printf("%.2f km\n", distance);

      break;
    }

    default: {
      fprintf(stderr, "Unexpected mode encountered for CEntry calls: %d\n", mode);
      exit(1); 
    }
  }

  jvm->DestroyJavaVM();
}

int main2(int argc, char** argv) {
  if (argc != 7) {
    fprintf(stderr, "Usage: %s <exec_mode> <language>[js|ruby] <lat1> <long1> <lat2> <long2>\n", argv[0]);
    exit(1);
  }

  auto mode = static_cast<exec_mode>(strtol(argv[1], NULL, 10));
  char* language = argv[2];
  double a_lat   = strtod(argv[3], NULL);
  double a_long  = strtod(argv[4], NULL);
  double b_lat   = strtod(argv[5], NULL);
  double b_long  = strtod(argv[6], NULL);

  const char* code = NULL;
  switch (language[0]) {
    case 'j':
      code = JS_HAVERSINE_DISTANCE;
      break;
    case 'r':
      code = RUBY_HAVERSINE_DISTANCE;
      break;
    default:
      fprintf(stderr, "Haversine distance code is not provided for '%s'\n", language);
      exit(1);
  }

  switch(mode) {
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
