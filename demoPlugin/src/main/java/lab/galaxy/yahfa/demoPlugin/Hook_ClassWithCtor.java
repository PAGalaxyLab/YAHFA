package lab.galaxy.yahfa.demoPlugin;

import android.util.Log;

public class Hook_ClassWithCtor {
    public static String className = "lab.galaxy.yahfa.demoApp.ClassWithCtor";
    public static String methodName = "<init>";
    public static String methodSig =
            "(Ljava/lang/String;)V";

    public static void hook(Object thiz, String param) {
        Log.w("YAHFA", "in ClassWithCtor.<init>: "+param);
        backup(thiz, "hooked "+param);
        return;
    }

    public static void backup(Object thiz, String param) {
        Log.w("YAHFA", "ClassWithVirtualMethod.tac() should not be here");
        return;
    }
}
