package lab.galaxy.yahfa.demoApp;

import android.util.Log;

public class Hook_Log_e {
    public static String className = "android.util.Log";
    public static String methodName = "e";
    public static String methodSig = "(Ljava/lang/String;Ljava/lang/String;)I";

    public static int hook(String tag, String msg) {
        Log.w("HookTest", "in Log.e(): " + tag + ", " + msg);
        return backup(tag, msg);
    }

    public static int backup(String tag, String msg) {
        Log.w("HookTest", "Log.e() should not be here");
        return 1;
    }
}
