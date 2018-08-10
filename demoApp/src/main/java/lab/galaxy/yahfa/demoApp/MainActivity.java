package lab.galaxy.yahfa.demoApp;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import dalvik.system.DexClassLoader;
import lab.galaxy.yahfa.HookMain;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button button = (Button)findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                for(int i=0; i<200; i++) {
                    doWork();
                }
            }
        });
    }

    void doWork() {
        // Log.e() should be hooked
        Log.e("origin", "call Log.e()");
        // String.startsWith() should be hooked
        Log.w("origin", "foo startsWith f is "+"foo".startsWith("f"));
        // ClassWithVirtualMethod.tac() should be hooked
        Log.w("origin", "virtual tac a,b,c,d, got "+
                new ClassWithVirtualMethod().tac("a","b","c","d"));
        // ClassWithStaticMethod.tac() should be hooked
        Log.w("origin", "static tac a,b,c,d, got "+
                ClassWithStaticMethod.tac("a","b","c","d"));
        Log.w("origin", "JNI method return string: "+ClassWithJNIMethod.fromJNI());
    }
}
