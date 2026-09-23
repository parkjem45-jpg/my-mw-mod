#pragma once

// Installs a Dobby hook on eglSwapBuffers so Menu::Render() runs every frame.
// Safe to call from the hack thread; performs detection logging for
// libEGL / libGLES / libvulkan so Logcat reveals the game's real renderer.
void InitEGLHook();
