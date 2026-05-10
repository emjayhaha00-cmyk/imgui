// menu.cpp
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include <jni.h>
#include <android/log.h>

#define LOG_TAG "ImGuiMenu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" {
    JNIEXPORT void JNICALL Java_com_yourpackage_imgui_1overlay_MainActivity_nativeRender(JNIEnv*, jobject);
    JNIEXPORT void JNICALL Java_com_yourpackage_imgui_1overlay_MainActivity_nativeInit(JNIEnv*, jobject, jint w, jint h);
    JNIEXPORT void JNICALL Java_com_yourpackage_imgui_1overlay_MainActivity_nativeTouch(JNIEnv*, jobject, jfloat x, jfloat y, jint action);
    JNIEXPORT void JNICALL Java_com_yourpackage_imgui_1overlay_MainActivity_nativeResize(JNIEnv*, jobject, jint w, jint h);
    JNIEXPORT void JNICALL Java_com_yourpackage_imgui_1overlay_MainActivity_memoryPatch(JNIEnv*, jobject, jstring lib, jlong offset, jstring hex);
    JNIEXPORT void JNICALL Java_com_yourpackage_imgui_1overlay_MainActivity_killGG(JNIEnv*, jobject);
}

static JavaVM* g_JVM = nullptr;
static jobject g_activity = nullptr;
static int screenW = 0, screenH = 0;
static bool isDragging = false;
static int dragStartX, dragStartY, winStartX, winStartY;

// UI state (same as original)
static bool bypassEnabled = false;
static bool aimbotEnabled = false;
static float aimStrength = 0.0f;
static bool wallEnabled = false, hitboxEnabled = false, norecoilEnabled = false;
static bool espOn = false, espLine = false, espBox = false, espHealth = false, espName = false, espDist = false;
static bool speedEnabled = false;
static bool skinAK = false, skinTang = false;

// JNI helpers
void callMemoryPatch(const char* lib, long offset, const char* hex) {
    JNIEnv* env;
    g_JVM->AttachCurrentThread(&env, nullptr);
    jstring jlib = env->NewStringUTF(lib);
    jstring jhex = env->NewStringUTF(hex);
    jmethodID method = env->GetMethodID(env->GetObjectClass(g_activity), "memoryPatch", "(Ljava/lang/String;JLjava/lang/String;)V");
    env->CallVoidMethod(g_activity, method, jlib, (jlong)offset, jhex);
    env->DeleteLocalRef(jlib);
    env->DeleteLocalRef(jhex);
    g_JVM->DetachCurrentThread();
}

// ------------------------------------------------------------------
// IMGUI WINDOW (AESTHETIC MINT THEME)
// ------------------------------------------------------------------
void setupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.GrabRounding = 10.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.12f, 0.10f, 0.95f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.88f, 0.65f, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.88f, 0.65f, 0.7f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.88f, 0.65f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.88f, 0.65f, 1.0f);
}

void renderMenu() {
    ImGui::SetNextWindowSize(ImVec2(340, 360), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.95f);
    ImGui::Begin("LEAFY MENU", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    // Tab bar
    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Bypass")) {
            ImGui::Checkbox("Bypass Logo", &bypassEnabled);
            ImGui::Checkbox("Skip Tutorial", &skipTutorial);
            ImGui::Checkbox("Clear Logs", &clearLogs);
            ImGui::Checkbox("ESP On", &espOn);
            if (espOn) {
                ImGui::Indent();
                ImGui::Checkbox("Line", &espLine);
                ImGui::Checkbox("Box", &espBox);
                ImGui::Checkbox("Health", &espHealth);
                ImGui::Checkbox("Name", &espName);
                ImGui::Checkbox("Distance", &espDist);
                ImGui::Unindent();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Memory")) {
            ImGui::Checkbox("No Recoil", &norecoilEnabled);
            ImGui::Checkbox("WallHack RED", &wallEnabled);
            ImGui::Checkbox("Hitbox", &hitboxEnabled);
            ImGui::Checkbox("WALLHACK TEST", &wh2Enabled);
            ImGui::Text("Aimbot: %.0f%%", aimStrength);
            ImGui::SliderFloat("##aim", &aimStrength, 0.0f, 100.0f, "%.0f%%");
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                // call memory patch with float to hex
                float val = aimStrength / 100.0f;
                char hex[32];
                sprintf(hex, "%02X %02X %02X %02X", 
                        *(uint8_t*)&val, *((uint8_t*)&val+1), 
                        *((uint8_t*)&val+2), *((uint8_t*)&val+3));
                callMemoryPatch("libunity.so", 0x5DAECFC, hex);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Ability")) {
            ImGui::Checkbox("Speed", &speedEnabled);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Skins")) {
            ImGui::Checkbox("Ak RADIANCE", &skinAK);
            ImGui::Checkbox("Tang", &skinTang);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::Checkbox("Auto Save", &autoSave);
            if (ImGui::Button("Kill GG")) {
                JNIEnv* env; g_JVM->AttachCurrentThread(&env, nullptr);
                jmethodID mid = env->GetMethodID(env->GetObjectClass(g_activity), "killGG", "()V");
                env->CallVoidMethod(g_activity, mid);
                g_JVM->DetachCurrentThread();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // Apply patches when checkboxes change
    static bool lastBypass = false;
    if (bypassEnabled && !lastBypass) {
        // Apply all libanogs.so patches (same as Lua)
        const long bypassOffsets[] = {0x1EEDA8, 0x1FDAAC, ...}; // add your offsets
        for (long off : bypassOffsets)
            callMemoryPatch("libanogs.so", off, "00 00 80 D2 C0 03 5F D6");
    }
    lastBypass = bypassEnabled;

    if (wallEnabled) {
        callMemoryPatch("libunity.so", 0x5DDB13C, "20 00 80 D2 C0 03 5F D6");
        callMemoryPatch("libunity.so", 0x4E5D954, "00 00 9F E5 1E FF 2F E1");
    }
    if (hitboxEnabled)
        callMemoryPatch("libunity.so", 0xAA91084, "20 00 80 D2 C0 03 5F D6");

    ImGui::End();
}

// ------------------------------------------------------------------
// JNI EXPORTS
// ------------------------------------------------------------------
JNIEXPORT void JNICALL Java_com_yourpackage_imgui_1overlay_MainActivity_nativeInit(JNIEnv* env, jobject obj, jint w, jint h) {
    env->GetJavaVM(&g_JVM);
    g_activity = env->NewGlobalRef(obj);
    screenW = w; screenH = h;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    setupImGuiStyle();
    
    ImGui_ImplAndroid_Init();
    ImGui_ImplOpenGL3_Init("#version 300 es");
}

JNIEXPORT void JNICALL Java_com_yourpackage_imgui_1overlay_MainActivity_nativeRender(JNIEnv* env, jobject obj) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
    renderMenu();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

JNIEXPORT void JNICALL Java_com_yourpackage_imgui_1overlay_MainActivity_nativeTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y, jint action) {
    ImGui_ImplAndroid_HandleTouch(x, y, action);
    // Drag logic for floating window (call Java updateOverlayPosition)
    static float lastX, lastY;
    if (action == 0) { // DOWN
        lastX = x; lastY = y;
        isDragging = true;
    } else if (action == 2 && isDragging) { // MOVE
        int newX = winStartX + (x - lastX);
        int newY = winStartY + (y - lastY);
        JNIEnv* env2; g_JVM->AttachCurrentThread(&env2, nullptr);
        jmethodID mid = env2->GetMethodID(env2->GetObjectClass(g_activity), "updateOverlayPosition", "(II)V");
        env2->CallVoidMethod(g_activity, mid, newX, newY);
        g_JVM->DetachCurrentThread();
    } else if (action == 1) { // UP
        isDragging = false;
    }
}
