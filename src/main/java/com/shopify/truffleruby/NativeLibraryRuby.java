package com.shopify.truffleruby;

import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.c.function.CEntryPoint;
import org.graalvm.polyglot.Context;
import org.graalvm.polyglot.Value;

public class NativeLibraryRuby {
    public static void main(String[] args) {
        System.out.println("You called native-library-ruby-runner with: " + args.toString());
    }

    @CEntryPoint(name = "distance_ruby")
    public static double distance(IsolateThread thread,
            double a_lat, double a_long,
            double b_lat, double b_long) {
        try (Context context = Context.newBuilder()
                .allowExperimentalOptions(true)
                .option("ruby.no-home-provided", "true")
                .build()) {
            final Value haversteinDistance = context.eval("ruby", PolyglotScripts.getHaversteinRuby());

            final Value ret = haversteinDistance.execute(a_lat, a_long, b_lat, b_long);

            return ret.asDouble();
        }
    }

}
