#include <polyglot_api.h>
#include <stdio.h>
#include <stdlib.h>

#include "polyglot_scripts.h"

#define TO_POLY_DOUBLE(thread, context, value)           \
  ({                                                     \
    poly_value result = NULL;                            \
    poly_create_double(thread, context, value, &result); \
    result;                                              \
  })

int main(int argc, char **argv) {
  if (argc != 6) {
    fprintf(stderr,
            "Usage: %s <language>[js|ruby] <lat1> <long1> <lat2> <long2>\n",
            argv[0]);
    exit(1);
  }

  char *language = argv[1];
  double a_lat = strtod(argv[2], NULL);
  double a_long = strtod(argv[3], NULL);
  double b_lat = strtod(argv[4], NULL);
  double b_long = strtod(argv[5], NULL);

  const char *code = NULL;
  switch (language[0]) {
    case 'j':
      code = JS_HAVERSINE_DISTANCE;
      break;
    case 'r':
      code = RUBY_HAVERSINE_DISTANCE;
      break;
    default:
      fprintf(stderr, "Haversine distance code is not provided for '%s'\n",
              language);
      exit(1);
  }

#ifdef DEBUG
  printf("Code fragment for '%s':\n%s\n", language, code);
#endif

  poly_isolate isolate = NULL;
  poly_thread thread = NULL;

  if (poly_create_isolate(NULL, &isolate, &thread) != poly_ok) {
    fprintf(stderr, "poly_create_isolate error\n");
    return 1;
  }

  poly_context context = NULL;

  if (poly_create_context(thread, NULL, 0, &context) != poly_ok) {
    fprintf(stderr, "poly_create_context error\n");
    goto exit_isolate;
  }

  poly_value haversine_distance_function = NULL;

  if (poly_context_eval(thread, context, language, "eval", code,
                        &haversine_distance_function) != poly_ok) {
    fprintf(stderr, "poly_context_eval error\n");

    const poly_extended_error_info *error;

    if (poly_get_last_error_info(thread, &error) != poly_ok) {
      fprintf(stderr, "poly_get_last_error_info error\n");
      goto exit_scope;
    }

    fprintf(stderr, "%s\n", error->error_message);
    goto exit_scope;
  }

  bool can_execute = false;
  poly_value_can_execute(thread, haversine_distance_function, &can_execute);

  if (can_execute) {
    poly_value haversine_distance_args[] = {
        TO_POLY_DOUBLE(thread, context, a_lat),
        TO_POLY_DOUBLE(thread, context, a_long),
        TO_POLY_DOUBLE(thread, context, b_lat),
        TO_POLY_DOUBLE(thread, context, b_long)};

    poly_value result = NULL;
    if (poly_value_execute(thread, haversine_distance_function,
                           haversine_distance_args, 4, &result) != poly_ok) {
      fprintf(stderr, "poly_value_execute error\n");

      const poly_extended_error_info *error;

      if (poly_get_last_error_info(thread, &error) != poly_ok) {
        fprintf(stderr, "poly_get_last_error_info error\n");
        goto exit_scope;
      }

      fprintf(stderr, "%s\n", error->error_message);
      goto exit_scope;
    }

    double distance;
    poly_value_as_double(thread, result, &distance);

    printf("%.2f km\n", distance);
  } else {
    fprintf(stderr,
            "The Haversine distance code block for '%s' is not executable. Did "
            "you return a function?\n",
            language);
    goto exit_scope;
  }

  if (poly_context_close(thread, context, true) != poly_ok) {
    fprintf(stderr, "poly_context_close error\n");
    goto exit_isolate;
  }

  if (poly_tear_down_isolate(thread) != poly_ok) {
    fprintf(stderr, "poly_tear_down_isolate error\n");
    return 1;
  }

  return 0;

exit_scope:
  poly_close_handle_scope(thread);
exit_context:
  poly_context_close(thread, context, true);
exit_isolate:
  poly_tear_down_isolate(thread);
  return 1;
}
