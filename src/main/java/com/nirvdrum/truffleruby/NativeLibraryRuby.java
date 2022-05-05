package com.nirvdrum.truffleruby;

import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.c.function.CEntryPoint;
import org.graalvm.polyglot.Context;
import org.graalvm.polyglot.Value;

public class NativeLibraryRuby {
    private static final Context context = Context.newBuilder()
            .allowExperimentalOptions(true)
            .option("ruby.no-home-provided", "true")
            .build();
    private static final Value haversineDistance = context.eval("ruby", PolyglotScripts.getHaversineRuby());

    public static void main(String[] args) {
        System.out.println("You called native-library-ruby-runner with: " + args.toString());
    }

    @CEntryPoint(name = "distance_ruby")
    public static double distance(IsolateThread thread,
            double a_lat, double a_long,
            double b_lat, double b_long) {

            final Value ret = haversineDistance.execute(a_lat, a_long, b_lat, b_long);

            return ret.asDouble();
    }

}
