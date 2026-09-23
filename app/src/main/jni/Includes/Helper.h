#pragma once

#include <jni.h>
#include <string>
#include <cstring>
#include <cstdio>

/////////////////////////////////////////////////////////////////////////////
// Helper.h - JNI / Android utility functions                             //
// Provides: package info, app context, version, URL opener,              //
//           build timestamp, and media-volume reader.                    //
/////////////////////////////////////////////////////////////////////////////

/* jvm is defined in AnAn/Call_Me.h (or equivalent); declared here so      */
/* Helper.h can be included before that header in the translation unit.    */
extern JavaVM* jvm;


/////////////////////////////////////////////////////////////////////////////
// getPackageName                                                          //
// Returns the app package name (e.g. "com.example.game") via JNI.       //
/////////////////////////////////////////////////////////////////////////////
inline std::string getPackageName(JNIEnv* env, jobject context) {
    jclass contextClass = env->GetObjectClass(context);
    jmethodID getPackageName = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
    jstring packageName = (jstring)env->CallObjectMethod(context, getPackageName);
    const char* pkgChars = env->GetStringUTFChars(packageName, nullptr);
    std::string pkg(pkgChars);
    env->ReleaseStringUTFChars(packageName, pkgChars);
    return pkg;
}


/////////////////////////////////////////////////////////////////////////////
// getApplicationContext                                                   //
// Retrieves the global Application context using ActivityThread.         //
// Caller is responsible for DetachCurrentThread if needed.               //
/////////////////////////////////////////////////////////////////////////////
inline jobject getApplicationContext() {
    JNIEnv* env;
    jvm->AttachCurrentThread(&env, nullptr);
    jclass activityThread = env->FindClass("android/app/ActivityThread");
    jmethodID currentApp = env->GetStaticMethodID(activityThread, "currentApplication", "()Landroid/app/Application;");
    jobject app = env->CallStaticObjectMethod(activityThread, currentApp);
    return app;
}


/////////////////////////////////////////////////////////////////////////////
// getVersion (overload 1)                                                 //
// Returns the app versionName given an existing JNIEnv + context.        //
// Falls back to "Unknown" if any JNI call fails.                         //
/////////////////////////////////////////////////////////////////////////////
inline std::string getVersion(JNIEnv* env, jobject context) {
    try {
        jclass contextClass = env->GetObjectClass(context);

        jmethodID getPackageManager = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject packageManager = env->CallObjectMethod(context, getPackageManager);

        jmethodID getPackageNameMethod = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
        jstring packageName = (jstring)env->CallObjectMethod(context, getPackageNameMethod);

        jclass packageManagerClass = env->GetObjectClass(packageManager);
        jmethodID getPackageInfo = env->GetMethodID(packageManagerClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
        jobject packageInfo = env->CallObjectMethod(packageManager, getPackageInfo, packageName, 0);

        jclass packageInfoClass = env->GetObjectClass(packageInfo);
        jfieldID versionNameField = env->GetFieldID(packageInfoClass, "versionName", "Ljava/lang/String;");
        jstring versionName = (jstring)env->GetObjectField(packageInfo, versionNameField);

        const char* versionChars = env->GetStringUTFChars(versionName, nullptr);
        std::string version(versionChars);
        env->ReleaseStringUTFChars(versionName, versionChars);
        return version;
    } catch (...) {
        return "Unknown";
    }
}


/////////////////////////////////////////////////////////////////////////////
// getVersion (overload 2 - no args)                                       //
// Convenience wrapper: attaches JNI, fetches context, returns version.   //
/////////////////////////////////////////////////////////////////////////////
inline std::string getVersion() {
    JNIEnv* env;
    jvm->AttachCurrentThread(&env, nullptr);
    jobject context = getApplicationContext();
    std::string version = getVersion(env, context);
    jvm->DetachCurrentThread();
    return version;
}


/////////////////////////////////////////////////////////////////////////////
// OpenUrl                                                                 //
// Launches an external URL in the device browser via an ACTION_VIEW      //
// Intent. Requires FLAG_ACTIVITY_NEW_TASK.                               //
/////////////////////////////////////////////////////////////////////////////
void OpenUrl(const char* url) {
    if (!jvm) return;

    JNIEnv* env = nullptr;
    jvm->AttachCurrentThread(&env, nullptr);

    jclass activityThread = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(
        activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);

    jmethodID getApplication = env->GetMethodID(
        activityThread, "getApplication", "()Landroid/app/Application;");
    jobject context = env->CallObjectMethod(at, getApplication);

    jclass intentCls = env->FindClass("android/content/Intent");
    jmethodID intentCtor = env->GetMethodID(
        intentCls, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");

    jclass uriCls = env->FindClass("android/net/Uri");
    jmethodID parse = env->GetStaticMethodID(
        uriCls, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");

    jstring jUrl   = env->NewStringUTF(url);
    jobject uri    = env->CallStaticObjectMethod(uriCls, parse, jUrl);
    jstring action = env->NewStringUTF("android.intent.action.VIEW");
    jobject intent = env->NewObject(intentCls, intentCtor, action, uri);

    /* Add FLAG_ACTIVITY_NEW_TASK so the Intent can start outside an Activity */
    jfieldID flagField = env->GetStaticFieldID(intentCls, "FLAG_ACTIVITY_NEW_TASK", "I");
    jint flag = env->GetStaticIntField(intentCls, flagField);

    jmethodID addFlags = env->GetMethodID(
        intentCls, "addFlags", "(I)Landroid/content/Intent;");
    env->CallObjectMethod(intent, addFlags, flag);

    jclass contextCls = env->GetObjectClass(context);
    jmethodID startActivity = env->GetMethodID(
        contextCls, "startActivity", "(Landroid/content/Intent;)V");
    env->CallVoidMethod(context, startActivity, intent);
}


/////////////////////////////////////////////////////////////////////////////
// BuildTime                                                               //
// Returns compile-time date/time as a formatted string:                  //
// "DD-MM-YYYY HH:MM:SS (UTC+0)" - useful for About tab.                 //
/////////////////////////////////////////////////////////////////////////////
static std::string BuildTime() {
    const char* date = __DATE__;  /* e.g. "Feb 18 2026" */
    const char* time = __TIME__;  /* e.g. "18:00:00"    */

    char monthStr[4];
    int  day, year;
    sscanf(date, "%3s %d %d", monthStr, &day, &year);

    int month = 0;
    #define CMP(m, n) if (strcmp(monthStr, m) == 0) month = n;
    CMP("Jan",1) CMP("Feb",2) CMP("Mar",3) CMP("Apr",4)
    CMP("May",5) CMP("Jun",6) CMP("Jul",7) CMP("Aug",8)
    CMP("Sep",9) CMP("Oct",10) CMP("Nov",11) CMP("Dec",12)
    #undef CMP

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%02d-%02d-%d %s (UTC+0)", day, month, year, time);
    return std::string(buffer);
}


/////////////////////////////////////////////////////////////////////////////
// getCurrentVolume                                                        //
// Reads the current media stream volume (STREAM_MUSIC = 3) via JNI.     //
// Returns -1 on failure or if jvm is not available.                      //
/////////////////////////////////////////////////////////////////////////////
int getCurrentVolume() {
    if (!jvm) return -1;

    JNIEnv* env     = nullptr;
    bool    attached = false;

    if (jvm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        } else {
            return -1;
        }
    }

    int volume = -1;

    jclass activityThread = env->FindClass(oxorany("android/app/ActivityThread"));
    if (activityThread) {
        jmethodID currentApplication = env->GetStaticMethodID(
            activityThread, oxorany("currentApplication"), oxorany("()Landroid/app/Application;"));
        if (currentApplication) {
            jobject context = env->CallStaticObjectMethod(activityThread, currentApplication);
            if (context) {
                /* Obtain AudioManager via getSystemService("audio") */
                jmethodID getSystemService = env->GetMethodID(
                    env->GetObjectClass(context),
                    oxorany("getSystemService"),
                    oxorany("(Ljava/lang/String;)Ljava/lang/Object;"));

                jstring audioServiceStr = env->NewStringUTF(oxorany("audio"));
                jobject audioManager = env->CallObjectMethod(context, getSystemService, audioServiceStr);
                env->DeleteLocalRef(audioServiceStr);

                if (audioManager) {
                    /* STREAM_MUSIC = 3 */
                    jmethodID getStreamVolume = env->GetMethodID(
                        env->GetObjectClass(audioManager),
                        oxorany("getStreamVolume"),
                        oxorany("(I)I"));
                    volume = env->CallIntMethod(audioManager, getStreamVolume, 3);
                    env->DeleteLocalRef(audioManager);
                }
                env->DeleteLocalRef(context);
            }
        }
        env->DeleteLocalRef(activityThread);
    }

    if (attached) jvm->DetachCurrentThread();
    return volume;
}
