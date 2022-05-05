#include <stdio.h>
#include <stdlib.h>

#include "libnative-library-runner-ruby.h"

int main(int argc, char** argv) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s <lat1> <long1> <lat2> <long2>\n", argv[0]);
    exit(1);
  }

  graal_isolatethread_t* thread = create_isolate();

  double a_lat = strtod(argv[1], NULL);
  double a_long = strtod(argv[2], NULL);
  double b_lat = strtod(argv[3], NULL);
  double b_long = strtod(argv[4], NULL);

  printf("%.2f km\n", distance_ruby(thread, a_lat, a_long, b_lat, b_long));

  tear_down_isolate(thread);
}
