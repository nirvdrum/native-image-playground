package com.shopify.truffleruby;

import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.c.function.CEntryPoint;
import org.graalvm.nativeimage.c.type.CCharPointer;
import org.graalvm.nativeimage.c.type.CTypeConversion;
import org.graalvm.nativeimage.c.type.VoidPointer;
import org.graalvm.nativeimage.c.type.WordPointer;
import org.graalvm.polyglot.Context;
import org.graalvm.polyglot.Value;

public class NativeLibraryPolyglot {
    public static void main(String[] args) {
        System.out.println("You called native-library-polyglot-runner with: " + args.toString());
    }

    @CEntryPoint(name = "distance_polyglot")
    public static double distance(IsolateThread thread,
            CCharPointer cLanguage,
            CCharPointer cCode,
            double aLat, double aLong,
            double bLat, double bLong) {
        try (Context context = Context.newBuilder()
                .allowExperimentalOptions(true)
                .option("ruby.no-home-provided", "true")
                .build()) {
            final String language = CTypeConversion.toJavaString(cLanguage);
            final String code = CTypeConversion.toJavaString(cCode);

            final Value function = context.eval(language, code);

            return function.execute(aLat, aLong, bLat, bLong).asDouble();
        }
    }

}
