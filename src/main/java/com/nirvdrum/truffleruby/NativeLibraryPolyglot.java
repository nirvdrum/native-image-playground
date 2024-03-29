package com.nirvdrum.truffleruby;

import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.c.function.CEntryPoint;
import org.graalvm.nativeimage.c.type.CCharPointer;
import org.graalvm.nativeimage.c.type.CTypeConversion;
import org.graalvm.polyglot.Context;
import org.graalvm.polyglot.Value;

import java.util.concurrent.ConcurrentHashMap;

public class NativeLibraryPolyglot {
    private static final ConcurrentHashMap<String, Value> parseCache = new ConcurrentHashMap<>();
    private static final Context context = Context.newBuilder()
            .allowExperimentalOptions(true)
            .option("ruby.no-home-provided", "true")
            .build();
    private static Value function;

    public static void main(String[] args) {
        System.out.println("You called native-library-polyglot-runner with: " + args.toString());
    }

    @CEntryPoint(name = "distance_polyglot_no_cache")
    public static double distance(IsolateThread thread,
            CCharPointer cLanguage,
            CCharPointer cCode,
            double aLat, double aLong,
            double bLat, double bLong) {
        try (Context context = Context.newBuilder()
                .allowExperimentalOptions(true)
                .option("ruby.no-home-provided", "true")
                .build()) {
            final String code = CTypeConversion.toJavaString(cCode);
            final String language = CTypeConversion.toJavaString(cLanguage);

            var function = context.eval(language, code);

            return function.execute(aLat, aLong, bLat, bLong).asDouble();
        }
    }

    @CEntryPoint(name = "distance_polyglot_no_parse_cache")
    public static double distance_no_parse_cache(IsolateThread thread,
            CCharPointer cLanguage,
            CCharPointer cCode,
            double aLat, double aLong,
            double bLat, double bLong) {
        final String code = CTypeConversion.toJavaString(cCode);
        final String language = CTypeConversion.toJavaString(cLanguage);

        var function = context.eval(language, code);

        return function.execute(aLat, aLong, bLat, bLong).asDouble();
    }

    @CEntryPoint(name = "distance_polyglot_thread_safe_parse_cache")
    public static double distanceThreadSafeParseCache(IsolateThread thread,
            CCharPointer cLanguage,
            CCharPointer cCode,
            double aLat, double aLong,
            double bLat, double bLong) {
        final String code = CTypeConversion.toJavaString(cCode);
        final String language = CTypeConversion.toJavaString(cLanguage);

        var function = parseCache.computeIfAbsent(language + ":" + code, k -> context.eval(language, code));

        if (function == null) {
            function = context.eval(language, code);
        }

        return function.execute(aLat, aLong, bLat, bLong).asDouble();
    }

    @CEntryPoint(name = "distance_polyglot_thread_unsafe_parse_cache")
    public static double distanceThreadUnsafeParseCache(IsolateThread thread,
            CCharPointer cLanguage,
            CCharPointer cCode,
            double aLat, double aLong,
            double bLat, double bLong) {
        final String code = CTypeConversion.toJavaString(cCode);
        final String language = CTypeConversion.toJavaString(cLanguage);

        if (function == null) {
            function = context.eval(language, code);
        }

        return function.execute(aLat, aLong, bLat, bLong).asDouble();
    }

}
