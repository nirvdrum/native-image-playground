#ifndef __BENCHMARK_UTILS_H
#define __BENCHMARK_UTILS_H

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

#endif