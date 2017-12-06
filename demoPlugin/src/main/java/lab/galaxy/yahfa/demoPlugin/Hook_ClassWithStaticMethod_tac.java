package lab.galaxy.yahfa.demoPlugin;

import android.util.Log;

/**
 * Created by liuruikai756 on 30/03/2017.
 */

public class Hook_ClassWithStaticMethod_tac {
    public static String className = "lab.galaxy.yahfa.demoApp.ClassWithStaticMethod";
    public static String methodName = "tac";
    public static String methodSig =
            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;";

    public static String hook(String a, String b, String c, String d) {
        Log.w("YAHFA", "in ClassWithStaticMethod.tac(): "+a+", "+b+", "+c+", "+d);
        return "test"+a;
    }
    /*
    public static String backup(String a, String b, String c, String d) {
        Log.w("YAHFA", "ClassWithStaticMethod.tac() should not be here");
        return "";
    }
    */
}
