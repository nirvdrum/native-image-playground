#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "polyglot_scripts.h"

#ifdef DEBUG
  #define CHECK_EXCEPTION(env) ({ bool exceptionOccurred = env->ExceptionCheck(); if (exceptionOccurred) { env->ExceptionDescribe(); } })
#else
  #define CHECK_EXCEPTION(env) 0
#endif

using namespace std;

int main(int argc, char** argv) {
  if (argc != 6) {
    fprintf(stderr, "Usage: %s <language>[js|ruby] <lat1> <long1> <lat2> <long2>\n", argv[0]);
    exit(1);
  }

  char* language = argv[1];
  double a_lat   = strtod(argv[2], NULL);
  double a_long  = strtod(argv[3], NULL);
  double b_lat   = strtod(argv[4], NULL);
  double b_long  = strtod(argv[5], NULL);

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

  jvm->DestroyJavaVM();

  return 0;
}
