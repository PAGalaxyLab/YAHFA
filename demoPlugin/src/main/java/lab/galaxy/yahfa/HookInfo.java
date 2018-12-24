package lab.galaxy.yahfa;

import lab.galaxy.yahfa.demoPlugin.Hook_ClassWithCtor;
import lab.galaxy.yahfa.demoPlugin.Hook_ClassWithJNIMethod_fromJNI;
import lab.galaxy.yahfa.demoPlugin.Hook_ClassWithStaticMethod_tac;
import lab.galaxy.yahfa.demoPlugin.Hook_ClassWithVirtualMethod_tac;
import lab.galaxy.yahfa.demoPlugin.Hook_Log_e;
import lab.galaxy.yahfa.demoPlugin.Hook_String_startsWith;

/**
 * Created by liuruikai756 on 30/03/2017.
 */

public class HookInfo {
    public static String[] hookItemNames = {
            Hook_Log_e.class.getName(),
            Hook_String_startsWith.class.getName(),
            Hook_ClassWithVirtualMethod_tac.class.getName(),
            Hook_ClassWithStaticMethod_tac.class.getName(),
            Hook_ClassWithJNIMethod_fromJNI.class.getName(),
            Hook_ClassWithCtor.class.getName(),
    };
}
