package lab.galaxy.yahfa;

import android.app.Application;
import android.util.Log;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

import dalvik.system.DexClassLoader;

/**
 * Created by liuruikai756 on 28/03/2017.
 */

public class HookMain {
    private static final String TAG = "YAHFA";
    private static List<Class<?>> hookInfoClasses = new LinkedList<>();

    static {
        System.loadLibrary("yahfa");
        init(android.os.Build.VERSION.SDK_INT);
    }

    public static void doHookDefault(ClassLoader patchClassLoader, ClassLoader originClassLoader) {
        try {
            Class<?> hookInfoClass = Class.forName("lab.galaxy.yahfa.HookInfo", true, patchClassLoader);
            String[] hookItemNames = (String[])hookInfoClass.getField("hookItemNames").get(null);
            for(String hookItemName : hookItemNames) {
                doHookItemDefault(patchClassLoader, hookItemName, originClassLoader);
            }
            hookInfoClasses.add(hookInfoClass);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static void doHookItemDefault(ClassLoader patchClassLoader, String hookItemName, ClassLoader originClassLoader) {
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
                if (method.getName().equals("hook") && Modifier.isStatic(method.getModifiers())) {
                    hook = method;
                } else if (method.getName().equals("backup") && Modifier.isStatic(method.getModifiers())) {
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

    public static void findAndHook(Class targetClass, String methodName, String methodSig, Method hook) {
        hook(findMethod(targetClass, methodName, methodSig), hook);
    }

    public static void findAndBackupAndHook(Class targetClass, String methodName, String methodSig,
                                     Method hook, Method backup) {
        backupAndHook(findMethod(targetClass, methodName, methodSig), hook, backup);
    }

    public static void hook(Method target, Method hook) {
        backupAndHook(target, hook, null);
    }

    public static void backupAndHook(Method target, Method hook, Method backup) {
        if (target == null) {
            throw new IllegalArgumentException("null target method");
        }
        if (hook == null) {
            throw new IllegalArgumentException("null hook method");
        }

        checkCompatibleMethods(target, hook);
        if (backup != null) {
            checkCompatibleMethods(target, backup);
        }
        if (!backupAndHookNative(target, hook, backup)) {
            throw new RuntimeException("Failed to hook " + target + " with " + hook);
        }
    }

    private static Method findMethod(Class cls, String methodName, String methodSig) {
        if (cls == null) {
            throw new IllegalArgumentException("null class");
        }
        if (methodName == null) {
            throw new IllegalArgumentException("null method name");
        }
        if (methodSig == null) {
            throw new IllegalArgumentException("null method signature");
        }
        return findMethodNative(cls, methodName, methodSig);
    }

    private static void checkCompatibleMethods(Method original, Method replacement) {
        ArrayList<Class<?>> originalParams = new ArrayList<>(Arrays.asList(original.getParameterTypes()));
        ArrayList<Class<?>> replacementParams = new ArrayList<>(Arrays.asList(replacement.getParameterTypes()));

        if (!Modifier.isStatic(original.getModifiers())) {
            originalParams.add(0, original.getDeclaringClass());
        }
        if (!Modifier.isStatic(replacement.getModifiers())) {
            throw new IllegalArgumentException("replacement must be a static method: " + replacement);
        }

        if (!original.getReturnType().isAssignableFrom(replacement.getReturnType())) {
            throw new IllegalArgumentException("Replacement has incompatible return type. Expected " + original.getReturnType() + ", got " + replacement.getReturnType());
        }

        if (originalParams.size() != replacementParams.size()) {
            throw new IllegalArgumentException("Replacement arguments don't match. Expected " + originalParams.size() + ", got " + replacementParams.size());
        }

        for (int i=0; i<originalParams.size(); i++) {
            if (!replacementParams.get(i).isAssignableFrom(originalParams.get(i))) {
                throw new IllegalArgumentException("Replacement has incompatible argument #" + i + ": Expected " + originalParams.get(i) + ", got " + replacementParams.get(i));
            }
        }
    }

    private static native boolean backupAndHookNative(Method target, Method hook, Method backup);

    public static native Method findMethodNative(Class targetClass, String methodName, String methodSig);

    private static native void init(int SDK_version);
}
