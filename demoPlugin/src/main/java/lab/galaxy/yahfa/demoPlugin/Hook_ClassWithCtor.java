package lab.galaxy.yahfa.demoPlugin;

import android.util.Log;

import static lab.galaxy.yahfa.HookInfo.TAG;

public class Hook_ClassWithCtor {
    public static String className = "lab.galaxy.yahfa.demoApp.ClassWithCtor";
    public static String methodName = "<init>";
    public static String methodSig =
            "(Ljava/lang/String;)V";

    public static void hook(Object thiz, String param) {
        Log.w(TAG, "in ClassWithCtor.<init>: " + param);
        backup(thiz, "hooked " + param);
        return;
    }

    public static void backup(Object thiz, String param) {
        Log.w(TAG, "ClassWithVirtualMethod.tac() should not be here");
        return;
    }
}
