#include <stdio.h>
#include <stdlib.h>

#include "libnative-library-runner.h"

int main(int argc, char** argv) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s <lat1> <long1> <lat2> <long2>\n", argv[0]);
    exit(1);
  }

  graal_isolate_t *isolate = NULL;
  graal_isolatethread_t *thread = NULL;

  if (graal_create_isolate(NULL, &isolate, &thread) != 0) {
    fprintf(stderr, "initialization error\n");
    return 1;
  }

  graal_isolatethread_t *myThread = create_isolate();

  double a_lat   = strtod(argv[1], NULL);
  double a_long  = strtod(argv[2], NULL);
  double b_lat   = strtod(argv[3], NULL);
  double b_long  = strtod(argv[4], NULL);

  printf("%.2f km\n", distance(thread, a_lat, a_long, b_lat, b_long));

  graal_tear_down_isolate(thread);
  graal_tear_down_isolate(myThread);
}
