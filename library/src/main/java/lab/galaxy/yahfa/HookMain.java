package lab.galaxy.yahfa;

import android.app.Application;
import android.util.Log;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

import dalvik.system.DexClassLoader;

/**
 * Created by liuruikai756 on 28/03/2017.
 */

public class HookMain {
    private static final String TAG = "YAHFA";

    static {
        System.loadLibrary("yahfa");
    }

    public HookMain() {
        init(android.os.Build.VERSION.SDK_INT);
    }

    public void doHookDefault(ClassLoader patchClassLoader, ClassLoader originClassLoader) {
        try {
            Class<?> hookInfoClass = Class.forName("lab.galaxy.yahfa.HookInfo", true, patchClassLoader);
            String[] hookItemNames = (String[])hookInfoClass.getField("hookItemNames").get(null);
            initHookCap(hookItemNames.length);
            for(String hookItemName : hookItemNames) {
                doHookItemDefault(patchClassLoader, hookItemName, originClassLoader);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void doHookItemDefault(ClassLoader patchClassLoader, String hookItemName, ClassLoader originClassLoader) {
        try {
            Log.i(TAG, "Start hooking with item "+hookItemName);
            Class<?> hookItem = Class.forName(hookItemName, true, patchClassLoader);
            String className = (String)hookItem.getField("className").get(null);
            String methodName = (String)hookItem.getField("methodName").get(null);
            String methodSig = (String)hookItem.getField("methodSig").get(null);

            if(className == null || className.equals("")) {
                Log.w(TAG, "No target class. Skipping...");
                return;
            }
            Class<?> clazz = Class.forName(className, true, originClassLoader);
            if(Modifier.isAbstract(clazz.getModifiers())) {
                Log.w(TAG, "Hook may fail for abstract class: "+className);
            }

            Method hook = null;
            Method backup = null;
            for (Method method : hookItem.getDeclaredMethods()) {
                if (method.getName().equals("hook")) {
                    hook = method;
                } else if (method.getName().equals("origin")) {
                    backup = method;
                }
            }
            if (hook == null) {
                Log.e(TAG, "Cannot find hook for "+methodName);
                return;
            }
            findAndBackupAndHook(clazz, methodName, methodSig, hook, backup);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    public native void findAndBackupAndHook(Class targetClass, String methodName, String methodSig,
                                     Method hook, Method backup);

    private native void init(int SDK_version);

    private native void initHookCap(int cap);
}
