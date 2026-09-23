APP_ABI             := arm64-v8a
APP_PLATFORM        := android-28
APP_STL             := c++_static
APP_OPTIM           := release
APP_THIN_ARCHIVE    := true
APP_PIE             := true

APP_CFLAGS   += -O2 -DNDEBUG -fomit-frame-pointer -fstrict-aliasing                 -ffunction-sections -fdata-sections                 -fno-unwind-tables -fno-asynchronous-unwind-tables
APP_CPPFLAGS += -O2 -DNDEBUG -fomit-frame-pointer -fstrict-aliasing                 -ffunction-sections -fdata-sections                 -fno-unwind-tables -fno-asynchronous-unwind-tables
APP_LDFLAGS  += -Wl,--gc-sections -Wl,--strip-all -Wl,--exclude-libs,ALL

APP_SHORT_COMMANDS := true

APP_BUILD_SCRIPT := Android.mk
