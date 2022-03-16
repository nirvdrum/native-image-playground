#ifndef __POLYGLOT_SCRIPTS_H
#define __POLYGLOT_SCRIPTS_H

const char* JS_HAVERSINE_DISTANCE =
  "(a_lat, a_long, b_lat, b_long) => {\n"
  "    const EARTH_RADIUS = 6371;\n"
  "    const a_lat_radians = a_lat * Math.PI / 180;\n"
  "    const a_long_radians = a_long * Math.PI / 180;\n"
  "    const b_lat_radians = b_lat * Math.PI / 180;\n"
  "    const b_long_radians = b_long * Math.PI / 180;\n"
  "    const angular_distance = Math.acos(\n"
  "        Math.sin(a_lat_radians) * Math.sin(b_lat_radians) +\n"
  "        Math.cos(a_lat_radians) * Math.cos(b_lat_radians) *\n"
  "        Math.cos(a_long_radians - b_long_radians));\n"
  "    return EARTH_RADIUS * angular_distance;\n"
  "}";

const char* RUBY_HAVERSINE_DISTANCE =
  "EARTH_RADIUS = 6371 unless defined?(EARTH_RADIUS)\n"
  "->(a_lat, a_long, b_lat, b_long) do\n"
  "    a_lat_radians = a_lat * Math::PI / 180\n"
  "    a_long_radians = a_long * Math::PI / 180\n"
  "    b_lat_radians = b_lat * Math::PI / 180\n"
  "    b_long_radians = b_long * Math::PI / 180\n"
  "    angular_distance = Math::acos(\n"
  "        Math::sin(a_lat_radians) * Math::sin(b_lat_radians) +\n"
  "        Math::cos(a_lat_radians) * Math::cos(b_lat_radians) *\n"
  "        Math::cos(a_long_radians - b_long_radians))\n"
  "    EARTH_RADIUS * angular_distance\n"
  "end";


#endif