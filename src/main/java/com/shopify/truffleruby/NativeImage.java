package com.shopify.truffleruby;

import org.apache.sis.distance.DistanceUtils;

public class NativeImage {
    public static void main(String[] args) {
        final double aLat = Double.parseDouble(args[0]);
        final double aLong = Double.parseDouble(args[1]);
        final double bLat = Double.parseDouble(args[2]);
        final double bLong = Double.parseDouble(args[3]);

        System.out.printf("%f km%n", DistanceUtils.getHaversineDistance(aLat, aLong, bLat, bLong));
    }
}
