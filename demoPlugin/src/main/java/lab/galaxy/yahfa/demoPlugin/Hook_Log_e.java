package lab.galaxy.yahfa.demoPlugin;

import android.util.Log;

import static lab.galaxy.yahfa.HookInfo.TAG;

/**
 * Created by liuruikai756 on 30/03/2017.
 */

public class Hook_Log_e {
    public static String className = "android.util.Log";
    public static String methodName = "e";
    public static String methodSig = "(Ljava/lang/String;Ljava/lang/String;)I";

    public static int hook(String tag, String msg) {
        Log.w(TAG, "in Log.e(): " + tag + ", " + msg);
        return backup(tag, msg);
    }

    public static int backup(String tag, String msg) {
        Log.w(TAG, "Log.e() should not be here");
        return 1;
    }
}
