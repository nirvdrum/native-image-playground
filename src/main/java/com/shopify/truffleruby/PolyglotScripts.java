package com.shopify.truffleruby;

public class PolyglotScripts {
    public static String getHaversineRuby() {
        return """
                EARTH_RADIUS = 6371 unless defined?(EARTH_RADIUS)
                            
                ->(a_lat, a_long, b_lat, b_long) do
                    a_lat_radians = a_lat * Math::PI / 180
                    a_long_radians = a_long * Math::PI / 180
                    b_lat_radians = b_lat * Math::PI / 180
                    b_long_radians = b_long * Math::PI / 180
                    
                    angular_distance = Math::acos(
                        Math::sin(a_lat_radians) * Math::sin(b_lat_radians) +
                        Math::cos(a_lat_radians) * Math::cos(b_lat_radians) *
                        Math::cos(a_long_radians - b_long_radians))
                        
                    EARTH_RADIUS * angular_distance
                end
                """;
    }
}
