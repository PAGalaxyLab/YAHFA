package lab.galaxy.yahfa;

import android.os.Build;
import androidx.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class HookingTest {
    private static final String TAG = HookingTest.class.getSimpleName();

    @Before
    public void setup() {
        Log.i(TAG, "ABI=" + Build.CPU_ABI);
        StaticHook.targetCount = 0;
        StaticHook.hookCount = 0;
        StaticHook.backupCount = 0;

        InstanceHook.targetCount = 0;
        InstanceHook.hookCount = 0;
        InstanceHook.backupCount = 0;

        CtorHook.targetCount = 0;
        CtorHook.hookCount = 0;
        CtorHook.backupCount = 0;
    }

    @Test
    public void hookInstanceMethod() throws Exception {
        InstanceHook hookedInstance = new InstanceHook();

        Assert.assertEquals(3, hookedInstance.target(3));
        Assert.assertEquals(1, InstanceHook.targetCount);
        Assert.assertEquals(0, InstanceHook.hookCount);
        Assert.assertEquals(0, InstanceHook.backupCount);

        try {
            InstanceHook.hook(hookedInstance, 4);
            Assert.fail("Backup should have failed");
        } catch (UnsupportedOperationException e) {
            //Ok
        }
        Assert.assertEquals(1, InstanceHook.targetCount);
        Assert.assertEquals(1, InstanceHook.hookCount);
        Assert.assertEquals(1, InstanceHook.backupCount);


        //------------------------ AFTER HOOKING --------------------------------
        HookAnnotation.hookClass(InstanceHook.class);

        Assert.assertEquals(5, hookedInstance.target(5));
        Assert.assertEquals(2, InstanceHook.targetCount);
        Assert.assertEquals(2, InstanceHook.hookCount);
        Assert.assertEquals(1, InstanceHook.backupCount);

        Assert.assertEquals(6, InstanceHook.hook(hookedInstance, 6));
        Assert.assertEquals(3, InstanceHook.targetCount);
        Assert.assertEquals(3, InstanceHook.hookCount);
        Assert.assertEquals(1, InstanceHook.backupCount);

        Assert.assertEquals(7, InstanceHook.backup(hookedInstance, 7));
        Assert.assertEquals(4, InstanceHook.targetCount);
        Assert.assertEquals(3, InstanceHook.hookCount);
        Assert.assertEquals(1, InstanceHook.backupCount);
    }

    @Test
    public void hookStaticMethod() throws Exception {
        Assert.assertEquals(3, StaticHook.target(3));
        Assert.assertEquals(1, StaticHook.targetCount);
        Assert.assertEquals(0, StaticHook.hookCount);
        Assert.assertEquals(0, StaticHook.backupCount);

        try {
            StaticHook.hook(4);
            Assert.fail("Backup should have failed");
        } catch (UnsupportedOperationException e) {
            //Ok
        }
        Assert.assertEquals(1, StaticHook.targetCount);
        Assert.assertEquals(1, StaticHook.hookCount);
        Assert.assertEquals(1, StaticHook.backupCount);


        //------------------------ AFTER HOOKING --------------------------------
        HookAnnotation.hookClass(StaticHook.class);

        Assert.assertEquals(5, StaticHook.target(5));
        Assert.assertEquals(2, StaticHook.targetCount);
        Assert.assertEquals(2, StaticHook.hookCount);
        Assert.assertEquals(1, StaticHook.backupCount);

        Assert.assertEquals(6, StaticHook.hook(6));
        Assert.assertEquals(3, StaticHook.targetCount);
        Assert.assertEquals(3, StaticHook.hookCount);
        Assert.assertEquals(1, StaticHook.backupCount);

        Assert.assertEquals(7, StaticHook.backup(7));
        Assert.assertEquals(4, StaticHook.targetCount);
        Assert.assertEquals(3, StaticHook.hookCount);
        Assert.assertEquals(1, StaticHook.backupCount);
    }

    @Test
    public void hookCtorMethod() throws Exception {
        CtorHook ctorHook = new CtorHook(0);
        Assert.assertEquals(1, CtorHook.targetCount);
        Assert.assertEquals(0, CtorHook.hookCount);
        Assert.assertEquals(0, CtorHook.backupCount);

        try {
            CtorHook.hook(ctorHook, 4);
            Assert.fail("Backup should have failed");
        } catch (UnsupportedOperationException e) {
            //Ok
        }
        Assert.assertEquals(1, CtorHook.targetCount);
        Assert.assertEquals(1, CtorHook.hookCount);
        Assert.assertEquals(1, CtorHook.backupCount);


        //------------------------ AFTER HOOKING --------------------------------
        HookAnnotation.hookClass(CtorHook.class);

        ctorHook = new CtorHook(0);

        Assert.assertEquals(2, CtorHook.targetCount);
        Assert.assertEquals(2, CtorHook.hookCount);
        Assert.assertEquals(1, CtorHook.backupCount);

        ctorHook = new CtorHook(0);
        Assert.assertEquals(3, CtorHook.targetCount);
        Assert.assertEquals(3, CtorHook.hookCount);
        Assert.assertEquals(1, CtorHook.backupCount);

        CtorHook.backup(ctorHook, 0);
        Assert.assertEquals(4, CtorHook.targetCount);
        Assert.assertEquals(3, CtorHook.hookCount);
        Assert.assertEquals(1, CtorHook.backupCount);
    }

    static class CtorHook {
        static int targetCount = 0;
        static int hookCount = 0;
        static int backupCount = 0;

        public CtorHook(int arg) {
            targetCount++;
        }

        @HookAnnotation.ConstructorHook
        public static void hook(CtorHook thiz, int arg) {
            hookCount++;
            backup(thiz, arg);
        }

        @HookAnnotation.ConstructorBackup
        public static void backup(CtorHook thiz, int arg) {
            backupCount++;
            throw new UnsupportedOperationException("Stub!");
        }
    }

    static class StaticHook {
        static int targetCount = 0;
        static int hookCount = 0;
        static int backupCount = 0;

        public static int target(int arg) {
            targetCount++;
            return arg;
        }

        @HookAnnotation.StaticMethodHook(targetClass = StaticHook.class, methodName = "target")
        public static int hook(int arg) {
            hookCount++;
            return backup(arg);
        }

        @HookAnnotation.StaticMethodBackup(targetClass = StaticHook.class, methodName = "target")
        public static int backup(int arg) {
            backupCount++;
            throw new UnsupportedOperationException("Stub!");
        }
    }

    static class InstanceHook {
        static int targetCount;
        static int hookCount;
        static int backupCount;

        @HookAnnotation.MethodHook(methodName = "target")
        public static int hook(InstanceHook thiz, int arg) {
            hookCount++;
            return backup(thiz, arg);
        }

        @HookAnnotation.MethodBackup(methodName = "target")
        public static int backup(InstanceHook thiz, int arg) {
            backupCount++;
            throw new UnsupportedOperationException("Stub!");
        }

        public int target(int arg) {
            targetCount++;
            return arg;
        }
    }
}
