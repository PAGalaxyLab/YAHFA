package lab.galaxy.yahfa.demoPlugin;

import android.util.Log;

/**
 * Created by liuruikai756 on 30/03/2017.
 */

public class Hook_String_startsWith {
    public static String className = "java.lang.String";
    public static String methodName = "startsWith";
    public static String methodSig = "(Ljava/lang/String;)Z";
    public static boolean hook(String thiz, String prefix) {
        Log.w("YAHFA", "in String.startsWith(): "+thiz+", "+prefix);
        return origin(thiz, prefix);
    }

    public static boolean origin(String thiz, String prefix) {
        Log.w("YAHFA", "String.startsWith() should not be here");
        return false;
    }
}
