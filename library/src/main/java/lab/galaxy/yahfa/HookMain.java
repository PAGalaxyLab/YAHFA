package lab.galaxy.yahfa;

import android.app.Application;
import android.util.Log;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
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


    public static void findAndBackupAndHook(Class targetClass, String methodName, String methodSig,
                                     Method hook, Method backup) {
        try {
            int hookParamCount = hook.getParameterTypes().length;
            int targetParamCount = getParamCountFromSignature(methodSig);
            Log.d(TAG, "target method param count is "+targetParamCount);
            boolean isStatic = (hookParamCount == targetParamCount);
            // virtual method has 'thiz' object as the first parameter
            findAndBackupAndHook(targetClass, methodName, methodSig, isStatic, hook, backup);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static int getParamCountFromSignature(String signature) throws Exception{
        int index;
        int count = 0;
        int seg;
        try { // Read all declarations between for `(' and `)'
            if (signature.charAt(0) != '(') {
                throw new Exception("Invalid method signature: " + signature);
            }
            index = 1; // current string position
            while(signature.charAt(index) != ')') {
                seg = parseSignature(signature.substring(index));
                index += seg;
                count++;
            }

        } catch (final StringIndexOutOfBoundsException e) { // Should never occur
            throw new Exception("Invalid method signature: " + signature, e);
        }
        return count;
    }

    private static int parseSignature(String signature) throws Exception {
        int count = 0;
        switch (signature.charAt(0)) {
            case 'B': // byte
            case 'C': // char
            case 'D': // double
            case 'F': // float
            case 'I': // int
            case 'J': // long
            case 'S': // short
            case 'Z': // boolean
            case 'V': // void
                count++;
                break;
            case 'L': // class
                count++; // char L
                while(signature.charAt(count) != ';') {
                    count++;
                }
                count++; // char ;
                break;
            case '[': // array
                count++; // char [
                count += parseSignature(signature.substring(count));
                break;
            default:
                throw new Exception("Invalid type: " + signature);
        }
        return count;
    }


    private static native void findAndBackupAndHook(Class targetClass, String methodName, String methodSig,
                                            boolean isStatic,
                                            Method hook, Method backup);

    private static native void init(int SDK_version);
}
