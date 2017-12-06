package lab.galaxy.yahfa.demoApp;

/**
 * Created by liuruikai756 on 30/03/2017.
 */

public class ClassWithVirtualMethod {
    public String tac(String a, String b, String c, String d) {
        try {
            return d + c + b + a;
        }
        catch (Exception e) {
            e.printStackTrace();
            return "";
        }
    }
}
