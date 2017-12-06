package lab.galaxy.yahfa.demoPlugin;

import android.util.Log;

/**
 * Created by liuruikai756 on 30/03/2017.
 */

public class Hook_ClassWithVirtualMethod_tac {
    public static String className = "lab.galaxy.yahfa.demoApp.ClassWithVirtualMethod";
    public static String methodName = "tac";
    public static String methodSig =
            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;";

    public static String hook(Object thiz, String a, String b, String c, String d) {
        Log.w("YAHFA", "in ClassWithVirtualMethod.tac(): "+a+", "+b+", "+c+", "+d);
        return backup(thiz, a, b, c, d);
    }

    public static String backup(Object thiz, String a, String b, String c, String d) {
        Log.w("YAHFA", "ClassWithVirtualMethod.tac() should not be here");
        return "";
    }
}
