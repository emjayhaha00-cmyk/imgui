// MainActivity.java
package com.yourpackage.imgui_overlay;

import android.app.Activity;
import android.os.Bundle;
import android.view.*;
import android.widget.*;
import android.graphics.PixelFormat;
import android.provider.Settings;

public class MainActivity extends Activity {
    private WindowManager wm;
    private ImGuiSurfaceView surfaceView;
    private WindowManager.LayoutParams params;

    static {
        System.loadLibrary("imgui_menu");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (!Settings.canDrawOverlays(this)) {
            startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                    Uri.parse("package:" + getPackageName())));
        }
        setupOverlay();
    }

    private void setupOverlay() {
        wm = (WindowManager) getSystemService(WINDOW_SERVICE);
        surfaceView = new ImGuiSurfaceView(this);
        surfaceView.setZOrderOnTop(true);
        surfaceView.getHolder().setFormat(PixelFormat.TRANSLUCENT);

        params = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.WRAP_CONTENT,
                Build.VERSION.SDK_INT >= 26 ?
                        WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY :
                        WindowManager.LayoutParams.TYPE_PHONE,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT);
        params.gravity = Gravity.TOP | Gravity.START;
        params.x = 100;
        params.y = 200;
        wm.addView(surfaceView, params);
    }

    // Called from native code to update layout params (drag)
    public void updateOverlayPosition(int x, int y) {
        params.x = x;
        params.y = y;
        wm.updateViewLayout(surfaceView, params);
    }

    // Native ImGui render loop starts
    public native void nativeInit(int width, int height);
    public native void nativeRender();
    public native void nativeTouch(float x, float y, int action);
    public native void nativeResize(int w, int h);

    // Memory patches (JNI bridges)
    public native void memoryPatch(String lib, long offset, String hex);
    public native void killGG();
}
