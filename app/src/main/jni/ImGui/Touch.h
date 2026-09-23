#pragma once

int (*o_inject_event)(JNIEnv *env, jobject thiz, jobject inputEvent);

int32_t (*orig_ANativeWindow_getWidth)(ANativeWindow* window);
int32_t _ANativeWindow_getWidth(ANativeWindow* window) {
    screenWidth = orig_ANativeWindow_getWidth(window);
    return orig_ANativeWindow_getWidth(window);
}

int32_t (*orig_ANativeWindow_getHeight)(ANativeWindow* window);
int32_t _ANativeWindow_getHeight(ANativeWindow* window) {
    screenHeight = orig_ANativeWindow_getHeight(window);
    return orig_ANativeWindow_getHeight(window);
}
int Width, Height;

/*
namespace CW {
namespace Common {

struct Vector2 {
    float x;
    float y;
};

enum class TouchPhase { Began, Moved, Stationary, Ended, Canceled };

void (*orig_GetTouch)(int index, int* fingerId, Vector2* position, TouchPhase* phase) = nullptr;

static std::atomic<bool> g_ImGuiCapturedLastFrame{false};
static std::atomic<int> g_CapturedFingerIds[10] = {
    ATOMIC_VAR_INIT(-1), ATOMIC_VAR_INIT(-1), ATOMIC_VAR_INIT(-1), ATOMIC_VAR_INIT(-1), ATOMIC_VAR_INIT(-1),
    ATOMIC_VAR_INIT(-1), ATOMIC_VAR_INIT(-1), ATOMIC_VAR_INIT(-1), ATOMIC_VAR_INIT(-1), ATOMIC_VAR_INIT(-1)
};

class SpinLock {
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    
public:
    void lock() {
        int spin_count = 0;
        while (flag.test_and_set(std::memory_order_acquire)) {
            
            if (++spin_count > 100) {
                spin_count = 0;
                #if defined(_WIN32)
                    Sleep(0);
                #else
                    sched_yield();
                #endif
            }
        }
    }
    
    void unlock() {
        flag.clear(std::memory_order_release);
    }
    
    bool try_lock() {
        return !flag.test_and_set(std::memory_order_acquire);
    }
};

static SpinLock g_TouchLock;

class SpinLockGuard {
private:
    SpinLock& lock_;
    
public:
    explicit SpinLockGuard(SpinLock& lock) : lock_(lock) {
        lock_.lock();
    }
    
    ~SpinLockGuard() {
        lock_.unlock();
    }
    
    
    SpinLockGuard(const SpinLockGuard&) = delete;
    SpinLockGuard& operator=(const SpinLockGuard&) = delete;
};

void GetTouch(int index, int* fingerId, Vector2* position, TouchPhase* phase) {
    
    if (!orig_GetTouch || !fingerId || !position || !phase || index < 0 || index >= 10) {
        if (orig_GetTouch)
            orig_GetTouch(index, fingerId, position, phase);
        return;
    }

    
    SpinLockGuard lock(g_TouchLock);
    
    orig_GetTouch(index, fingerId, position, phase);

    
    const int originalFingerId = *fingerId;
    const Vector2 originalPos = *position;
    const TouchPhase originalPhase = *phase;

    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx) return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.y <= 0 || io.DisplaySize.x <= 0) return;

    const float x = position->x;
    const float y = io.DisplaySize.y - position->y;

    bool isTouchInImGuiWindow = false;
    if (index == 0) {
        if (io.WantCaptureMouse || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive()) {
            isTouchInImGuiWindow = true;
        }
    }

    
    if (index == 0) {
        switch (originalPhase) {
            case TouchPhase::Began:
                io.MousePos = ImVec2(x, y);
                io.MouseDown[0] = true;
                if (io.WantCaptureMouse || isTouchInImGuiWindow) {
                    g_ImGuiCapturedLastFrame.store(true, std::memory_order_release);
                }
                break;
            case TouchPhase::Moved:
            case TouchPhase::Stationary:
                io.MousePos = ImVec2(x, y);
                break;
            case TouchPhase::Ended:
            case TouchPhase::Canceled:
                io.MouseDown[0] = false;
                g_ImGuiCapturedLastFrame.store(false, std::memory_order_release);
                break;
        }
    }

    bool shouldBlockTouch = false;

    
    if (originalPhase == TouchPhase::Began) {
        
        for (int i = 0; i < 10; i++) {
            int expected = originalFingerId;
            g_CapturedFingerIds[i].compare_exchange_strong(expected, -1, std::memory_order_acq_rel);
        }
        
        if (io.WantCaptureMouse || isTouchInImGuiWindow) {
            g_CapturedFingerIds[index].store(originalFingerId, std::memory_order_release);
            shouldBlockTouch = true;
        }
    } else if (originalFingerId != -1) {
        
        for (int i = 0; i < 10; i++) {
            int capturedId = g_CapturedFingerIds[i].load(std::memory_order_acquire);
            if (capturedId == originalFingerId) {
                shouldBlockTouch = true;
                if (originalPhase == TouchPhase::Ended || originalPhase == TouchPhase::Canceled) {
                    g_CapturedFingerIds[i].store(-1, std::memory_order_release);
                }
                break;
            }
        }
    } else if (io.WantCaptureMouse && index == 0) {
        shouldBlockTouch = true;
    }

    
    if (shouldBlockTouch) {
        position->x = -9999.0f;
        position->y = -9999.0f;
        *fingerId = -1;
        *phase = TouchPhase::Canceled;
    }
}

}
}
*/
