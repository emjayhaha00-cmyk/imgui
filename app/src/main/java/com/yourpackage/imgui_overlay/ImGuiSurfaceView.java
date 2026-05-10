// ImGuiSurfaceView.java
package com.yourpackage.imgui_overlay;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;

public class ImGuiSurfaceView extends GLSurfaceView {
    private MainActivity activity;

    public ImGuiSurfaceView(Context context) {
        super(context);
        activity = (MainActivity) context;
        setEGLContextClientVersion(3);
        setRenderer(new ImGuiRenderer(activity));
        setRenderMode(RENDERMODE_CONTINUOUSLY);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        activity.nativeTouch(e.getX(), e.getY(), e.getActionMasked());
        return true;
    }
}
