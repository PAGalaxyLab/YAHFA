package lab.galaxy.yahfa.demoPlugin;

import android.util.Log;

import static lab.galaxy.yahfa.HookInfo.TAG;

/**
 * Created by lrk on 5/4/17.
 */

public class Hook_ClassWithJNIMethod_fromJNI {
    public static String className = "lab.galaxy.yahfa.demoApp.ClassWithJNIMethod";
    public static String methodName = "fromJNI";
    public static String methodSig = "()Ljava/lang/String;";

    // calling origin method is no longer available for JNI methods
    public static String hook() {
        Log.w(TAG, "calling fromJNI");
        return backup();
    }

    public static String backup() {
        Log.w(TAG, "calling fromJNI");
        return "1234";
    }
}
