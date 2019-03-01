package lab.galaxy.yahfa;

import android.util.Log;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

/**
 * Defines hooks using annotated methods.
 *
 * Target class, method name and signature are automatically inferred from the hook when possible
 */
public class HookAnnotation {
    public static final String TAG = HookAnnotation.class.getSimpleName();

    public static final String HOOK_SUFFIX = "Hook";
    public static final String BACKUP_SUFFIX = "Backup";

    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.METHOD)
    public @interface ConstructorHook {
    }

    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.METHOD)
    public @interface MethodHook {
        String methodName() default "";
    }

    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.METHOD)
    public @interface StaticMethodHook {
        Class<?> targetClass();
        String methodName() default "";
    }

    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.METHOD)
    public @interface ConstructorBackup {
    }

    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.METHOD)
    public @interface MethodBackup {
        String methodName() default "";
    }

    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.METHOD)
    public @interface StaticMethodBackup {
        Class<?> targetClass();
        String methodName() default "";
    }

    public static void hookClass(Class cls) {
        Map<Object, Method> hooks = new HashMap<>();
        Map<Object, Method> backups = new HashMap<>();

        for (Method m : cls.getDeclaredMethods()) {

            if (m.getAnnotation(ConstructorHook.class) != null) {
                try {
                    ConstructorHook annotation = m.getAnnotation(ConstructorHook.class);
                    Class targetClass = m.getParameterTypes()[0];
                    Class[] argTypes = Arrays.copyOfRange(m.getParameterTypes(), 1, m.getParameterTypes().length);
                    Constructor targetMethod = targetClass.getDeclaredConstructor(argTypes);
                    hooks.put(targetMethod, m);
                } catch (Exception e) {
                    Log.e(TAG, "Cannot find target constructor for " + m, e);
                }
            }

            if (m.getAnnotation(MethodHook.class) != null) {
                try {
                    MethodHook annotation = m.getAnnotation(MethodHook.class);
                    Class targetClass = m.getParameterTypes()[0];
                    String methodName = annotation.methodName().isEmpty() ? m.getName().replaceFirst(HOOK_SUFFIX + "$", "") : annotation.methodName();
                    Class[] argTypes = Arrays.copyOfRange(m.getParameterTypes(), 1, m.getParameterTypes().length);
                    Method targetMethod = targetClass.getDeclaredMethod(methodName, argTypes);
                    if (Modifier.isStatic(targetMethod.getModifiers())) {
                        throw new IllegalArgumentException("Target method is static: " + targetMethod);
                    }
                    hooks.put(targetMethod, m);
                } catch (Exception e) {
                    Log.e(TAG, "Cannot find target method for " + m, e);
                }
            }

            if (m.getAnnotation(StaticMethodHook.class) != null) {
                try {
                    StaticMethodHook annotation = m.getAnnotation(StaticMethodHook.class);
                    Class targetClass = annotation.targetClass();
                    String methodName = annotation.methodName().isEmpty() ? m.getName().replaceFirst(HOOK_SUFFIX + "$", "") : annotation.methodName();
                    Class[] argTypes = m.getParameterTypes();
                    Method targetMethod = targetClass.getDeclaredMethod(methodName, argTypes);
                    if (!Modifier.isStatic(targetMethod.getModifiers())) {
                        throw new IllegalArgumentException("Target method is not static: " + targetMethod);
                    }
                    hooks.put(targetMethod, m);
                } catch (Exception e) {
                    Log.e(TAG, "Cannot find target method for " + m, e);
                }
            }

            if (m.getAnnotation(ConstructorBackup.class) != null) {
                try {
                    ConstructorBackup annotation = m.getAnnotation(ConstructorBackup.class);
                    Class targetClass = m.getParameterTypes()[0];
                    Class[] argTypes = Arrays.copyOfRange(m.getParameterTypes(), 1, m.getParameterTypes().length);
                    Constructor targetMethod = targetClass.getDeclaredConstructor(argTypes);
                    backups.put(targetMethod, m);
                } catch (Exception e) {
                    Log.e(TAG, "Cannot find target constructor for " + m, e);
                }
            }

            if (m.getAnnotation(MethodBackup.class) != null) {
                try {
                    MethodBackup annotation = m.getAnnotation(MethodBackup.class);
                    Class targetClass = m.getParameterTypes()[0];
                    String methodName = annotation.methodName().isEmpty() ? m.getName().replaceFirst(BACKUP_SUFFIX + "$", "") : annotation.methodName();
                    Class[] argTypes = Arrays.copyOfRange(m.getParameterTypes(), 1, m.getParameterTypes().length);
                    Method targetMethod = targetClass.getDeclaredMethod(methodName, argTypes);
                    if (Modifier.isStatic(targetMethod.getModifiers())) {
                        throw new IllegalArgumentException("Target method is static: " + targetMethod);
                    }
                    backups.put(targetMethod, m);
                } catch (Exception e) {
                    Log.e(TAG, "Cannot find target method for " + m, e);
                }
            }

            if (m.getAnnotation(StaticMethodBackup.class) != null) {
                try {
                    StaticMethodBackup annotation = m.getAnnotation(StaticMethodBackup.class);
                    Class targetClass = annotation.targetClass();
                    String methodName = annotation.methodName().isEmpty() ? m.getName().replaceFirst(BACKUP_SUFFIX + "$", "") : annotation.methodName();
                    Class[] argTypes = m.getParameterTypes();
                    Method targetMethod = targetClass.getDeclaredMethod(methodName, argTypes);
                    if (!Modifier.isStatic(targetMethod.getModifiers())) {
                        throw new IllegalArgumentException("Target method is not static: " + targetMethod);
                    }
                    backups.put(targetMethod, m);
                } catch (Exception e) {
                    Log.e(TAG, "Cannot find target method for " + m, e);
                }
            }
        }

        for (Object target : hooks.keySet()) {
            Method hook = hooks.get(target);
            Method backup = backups.get(target);
            Throwable err = null;
            try {
                HookMain.backupAndHook(target, hook, backup);
                Log.i(TAG, "Hooked " + target + ": hook=" + hook + (backup == null ? "" : ", backup=" + backup));
            } catch (Throwable t) {
                Log.e(TAG, "Failed to hooked " + target + ": hook=" + hook + (backup == null ? "" : ", backup=" + backup), t);
            }
        }

        for (Object target : backups.keySet()) {
            if (!hooks.containsKey(target)) {
                Log.w(TAG, "Failed to install backup " + backups.get(target) + " on " + target + " because there is no matching hook");
            }
        }
    }
}
