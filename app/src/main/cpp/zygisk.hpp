#pragma once

#include <jni.h>

namespace zygisk {

struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

class ModuleBase {
public:
    virtual void onLoad([[maybe_unused]] Api *api, [[maybe_unused]] JNIEnv *env) {}
    virtual void preAppSpecialize([[maybe_unused]] AppSpecializeArgs *args) {}
    virtual void postAppSpecialize([[maybe_unused]] const AppSpecializeArgs *args) {}
    virtual void preServerSpecialize([[maybe_unused]] ServerSpecializeArgs *args) {}
    virtual void postServerSpecialize([[maybe_unused]] const ServerSpecializeArgs *args) {}
};

struct AppSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jobjectArray &rlimits;
    jint &mount_external;
    jstring &se_info;
    jstring &nice_name;
    jstring &instruction_set;
    jstring &app_data_dir;

    jintArray *const fds_to_ignore = nullptr;
    jboolean *const is_child_zygote = nullptr;
    jboolean *const is_top_app = nullptr;
    jobjectArray *const pkg_data_info_list = nullptr;
    jobjectArray *const whitelisted_data_info_list = nullptr;
    jboolean *const mount_data_dirs = nullptr;
    jboolean *const mount_storage_dirs = nullptr;
    jboolean *const mount_sysprop_overrides = nullptr;

    AppSpecializeArgs() = delete;
};

struct ServerSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jlong &permitted_capabilities;
    jlong &effective_capabilities;

    ServerSpecializeArgs() = delete;
};

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(moduleClass) \
    extern "C" void zygisk_module_entry() { \
        (void)sizeof(moduleClass); \
    }
