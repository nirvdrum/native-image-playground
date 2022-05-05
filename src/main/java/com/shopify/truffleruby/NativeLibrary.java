package com.shopify.truffleruby;

import org.apache.sis.distance.DistanceUtils;
import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.c.function.CEntryPoint;

public class NativeLibrary {
    public static void main(String[] args) {
        System.out.println("You called native-library-runner with: " + args.toString());
    }

    @CEntryPoint(name = "distance")
    private static double distance(IsolateThread thread,
            double a_lat, double a_long,
            double b_lat, double b_long) {
        return DistanceUtils.getHaversineDistance(a_lat, a_long, b_lat, b_long);
    }
}
