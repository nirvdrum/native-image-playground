package com.shopify.truffleruby;

import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.c.function.CEntryPoint;

public class PolyglotUtils {
    @CEntryPoint(builtin=CEntryPoint.Builtin.CREATE_ISOLATE, name = "create_isolate")
    static native IsolateThread createIsolate();
}
