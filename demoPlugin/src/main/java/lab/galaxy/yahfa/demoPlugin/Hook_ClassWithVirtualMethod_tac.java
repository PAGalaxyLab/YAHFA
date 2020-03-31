package lab.galaxy.yahfa.demoPlugin;

import android.util.Log;

import static lab.galaxy.yahfa.HookInfo.TAG;

/**
 * Created by liuruikai756 on 30/03/2017.
 */

public class Hook_ClassWithVirtualMethod_tac {
    public static String className = "lab.galaxy.yahfa.demoApp.ClassWithVirtualMethod";
    public static String methodName = "tac";
    public static String methodSig =
            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;";

    public static String hook(Object thiz, String a, String b, String c, String d) {
        int uid = android.os.Process.myUid();
        Log.w(TAG, "in ClassWithVirtualMethod.tac(): " + a + ", " + b + ", " + c + ", " + d + ": " + uid);
        return backup(thiz, a, b, c, d);
    }

    public static String backup(Object thiz, String a, String b, String c, String d) {
        try {
            Log.w(TAG, "ClassWithVirtualMethod.tac() should not be here");
        }
        catch (Exception e) {

        }
        return "";
    }
}

