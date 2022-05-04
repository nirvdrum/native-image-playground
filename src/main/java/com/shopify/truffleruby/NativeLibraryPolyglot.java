package com.shopify.truffleruby;

import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.c.function.CEntryPoint;
import org.graalvm.nativeimage.c.type.CCharPointer;
import org.graalvm.nativeimage.c.type.CTypeConversion;
import org.graalvm.polyglot.Context;
import org.graalvm.polyglot.Value;

import java.util.concurrent.ConcurrentHashMap;

public class NativeLibraryPolyglot {
    private static final ConcurrentHashMap<String, Value> codeMap = new ConcurrentHashMap<>();
    private static final Context context = Context.newBuilder()
            .allowExperimentalOptions(true)
            .option("ruby.no-home-provided", "true")
            .build();

    public static void main(String[] args) {
        System.out.println("You called native-library-polyglot-runner with: " + args.toString());
    }

    @CEntryPoint(name = "distance_polyglot")
    public static double distance(IsolateThread thread,
            CCharPointer cLanguage,
            CCharPointer cCode,
            double aLat, double aLong,
            double bLat, double bLong) {
        final String code = CTypeConversion.toJavaString(cCode);
        final String language = CTypeConversion.toJavaString(cLanguage);

        var function = codeMap.computeIfAbsent(language + ":" + code, k -> context.eval(language, code));

        return function.execute(aLat, aLong, bLat, bLong).asDouble();
    }

}
