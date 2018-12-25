package lab.galaxy.yahfa.demoApp;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity {
    private static final String TAG = "origin";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button button = (Button) findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                for (int i = 0; i < 200; i++) {
                    doWork();
                }
            }
        });
    }

    void doWork() {
        // Log.e() should be hooked
        Log.e(TAG, "call Log.e()");
        // String.startsWith() should be hooked
        Log.w(TAG, "foo startsWith f is " + "foo".startsWith("f"));
        // ClassWithVirtualMethod.tac() should be hooked
        Log.w(TAG, "virtual tac a,b,c,d, got " +
                new ClassWithVirtualMethod().tac("a", "b", "c", "d"));
        // ClassWithStaticMethod.tac() should be hooked
        Log.w(TAG, "static tac a,b,c,d, got " +
                ClassWithStaticMethod.tac("a", "b", "c", "d"));
        Log.w(TAG, "JNI method return string: " + ClassWithJNIMethod.fromJNI());

        ClassWithCtor classWithCtor = new ClassWithCtor("param");
        Log.w(TAG, "class ctor and get field: " + classWithCtor.getField());
    }
}
