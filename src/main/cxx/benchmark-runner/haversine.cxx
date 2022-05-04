#include <math.h>

static const int EARTH_RADIUS = 6371;  // in km

double degrees_to_radians(double degrees) { return degrees * (M_PI / 180.0L); }

double haversine_distance(double a_lat, double a_long, double b_lat,
                          double b_long) {
  double a_lat_radians = degrees_to_radians(a_lat);
  double a_long_radians = degrees_to_radians(a_long);
  double b_lat_radians = degrees_to_radians(b_lat);
  double b_long_radians = degrees_to_radians(b_long);

  double angular_distance = acos(sin(a_lat_radians) * sin(b_lat_radians) +
                                 cos(a_lat_radians) * cos(b_lat_radians) *
                                     cos(a_long_radians - b_long_radians));

  return EARTH_RADIUS * angular_distance;
}
