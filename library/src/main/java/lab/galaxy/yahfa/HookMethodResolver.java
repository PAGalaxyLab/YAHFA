package lab.galaxy.yahfa;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * create by Swift Gan on 14/01/2019
 * To ensure method in resolved cache
 */

public class HookMethodResolver {

    public static Class artMethodClass;

    public static Field resolvedMethodsField;
    public static Field dexCacheField;
    public static Field dexMethodIndexField;
    public static Field artMethodField;

    public static boolean canResolvedInJava = false;

    public static long resolvedMethodsAddress = 0;
    public static int dexMethodIndex = 0;

    public static void init() {
        checkSupport();
    }

    private static void checkSupport() {
        try {
            Method testMethod = HookMethodResolver.class.getDeclaredMethod("init");
            dexMethodIndexField = getField(Method.class, "dexMethodIndex");
            dexMethodIndex = (int) dexMethodIndexField.get(testMethod);
            dexCacheField = getField(Class.class, "dexCache");
            Object dexCache = dexCacheField.get(testMethod.getDeclaringClass());
            resolvedMethodsField = getField(dexCache.getClass(), "resolvedMethods");
            Object resolvedMethods = resolvedMethodsField.get(dexCache);
            if (resolvedMethods instanceof Long) {
                canResolvedInJava = false;
                resolvedMethodsAddress = (long) resolvedMethods;
            } else if (resolvedMethods instanceof long[]) {
                canResolvedInJava = true;
            } else if (hasJavaArtMethod() && resolvedMethods instanceof Object[]) {
                canResolvedInJava = true;
            } else {
                canResolvedInJava = false;
            }
            if (canResolvedInJava) {
                artMethodField = getField(Method.class, "artMethod");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void resolveMethod(Method hook, Method backup) {
        if (canResolvedInJava && artMethodField != null) {
            // in java
            try {
                resolveInJava(hook, backup);
            } catch (Exception e) {
                // in native
                resolveInNative(hook, backup);
            }
        } else {
            // in native
            resolveInNative(hook, backup);
        }
    }

    private static void resolveInJava(Method hook, Method backup) throws Exception {
        Object dexCache = dexCacheField.get(hook.getDeclaringClass());
        int dexMethodIndex = (int) dexMethodIndexField.get(backup);
        Object resolvedMethods = resolvedMethodsField.get(dexCache);

        if (resolvedMethods instanceof long[]) {
            long artMethod = (long) artMethodField.get(backup);
            ((long[])resolvedMethods)[dexMethodIndex] = artMethod;
        } else if (resolvedMethods instanceof Object[]) {
            Object artMethod = artMethodField.get(backup);
            ((Object[])resolvedMethods)[dexMethodIndex] = artMethod;
        } else {
            throw new UnsupportedOperationException("unsupport");
        }
    }

    private static void resolveInNative(Method hook, Method backup) {
        HookMain.ensureMethodCached(hook, backup);
    }

    public static Field getField(Class topClass, String fieldName) throws NoSuchFieldException {
        while (topClass != null && topClass != Object.class) {
            try {
                Field field = topClass.getDeclaredField(fieldName);
                field.setAccessible(true);
                return field;
            } catch (Exception e) {
            }
            topClass = topClass.getSuperclass();
        }
        throw new NoSuchFieldException(fieldName);
    }

    public static boolean hasJavaArtMethod() {
        if (artMethodClass != null)
            return true;
        try {
            artMethodClass = Class.forName("java.lang.reflect.ArtMethod");
            return true;
        } catch (ClassNotFoundException e) {
            return false;
        }
    }

}
