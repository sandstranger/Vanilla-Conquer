#include "wwstd.h"
#include "settings.h"
#include "ini.h"
#include "miscasm.h"
#if ANDROID
#include "SDL.h"
#endif

SettingsClass Settings;

#if ANDROID
#include <atomic>
#include <mutex>

extern SDL_Window* window;
static std::atomic<bool> g_onScreenControlsActive(true);

/*
** The launcher's settings, set through JNA on the java side's thread. The game's thread reads Settings without locks, and
** SDL's relative mouse mode is the SDL thread's: the setters record the values, Apply_Launcher_Settings puts them into
** Settings on the game's thread - at the start of SettingsClass::Load and whenever it polls its events. The last call
** for a setting wins, as when the setters wrote Settings themselves.
*/
namespace
{
    struct LauncherSettings
    {
        bool hasDosMode = false, dosMode = false;
        bool hasFrameLimit = false;
        int frameLimit = 0;
        bool hasSensitivity = false;
        int sensitivity = 0;
        bool hasPointerSpeed = false;
        int pointerSpeed = 0;
        bool hasRawInput = false, rawInput = false, rawInputToSdl = false;
    };
    std::mutex g_launcherMutex;
    LauncherSettings g_launcherSettings;
    std::atomic<bool> g_launcherSettingsPending(false);

    template <typename F> void Set_Launcher_Setting(F set)
    {
        std::lock_guard<std::mutex> lock(g_launcherMutex);
        set(g_launcherSettings);
        g_launcherSettingsPending = true;
    }
}

void Apply_Launcher_Settings()
{
    if (!g_launcherSettingsPending.exchange(false)) {
        return;
    }
    LauncherSettings s;
    {
        std::lock_guard<std::mutex> lock(g_launcherMutex);
        s = g_launcherSettings;
        g_launcherSettings = LauncherSettings();
    }
    if (s.hasDosMode) {
        Settings.Video.DOSMode = s.dosMode;
    }
    if (s.hasFrameLimit) {
        Settings.Video.FrameLimit = s.frameLimit;
    }
    if (s.hasSensitivity) {
        Settings.Mouse.Sensitivity = s.sensitivity;
    }
    if (s.hasPointerSpeed) {
        Settings.Mouse.ControllerPointerSpeed = s.pointerSpeed;
    }
    if (s.hasRawInput) {
        Settings.Mouse.RawInput = s.rawInput;
        if (s.rawInputToSdl && window) {
            SDL_SetRelativeMouseMode(s.rawInput ? SDL_TRUE : SDL_FALSE);
        }
    }
}

extern "C" {
// The switches take an int: JNA passes a java 'boolean' as a 32 bit int.
__attribute__((used)) __attribute__((visibility("default")))
void setUseDoseModeState(const int useDosMode) {
    Set_Launcher_Setting([=](LauncherSettings& s) { s.hasDosMode = true; s.dosMode = useDosMode != 0; });
}
__attribute__((used)) __attribute__((visibility("default")))
void setFrameRateLimit(const int frameRateLimit) {
    Set_Launcher_Setting([=](LauncherSettings& s) { s.hasFrameLimit = true; s.frameLimit = frameRateLimit; });
}
__attribute__((used)) __attribute__((visibility("default")))
void setOnScreenControlsState(const int onScreenControlsActive) {
    g_onScreenControlsActive = onScreenControlsActive != 0;
    if (onScreenControlsActive) {
        Set_Launcher_Setting([](LauncherSettings& s) { s.hasRawInput = true; s.rawInput = false; s.rawInputToSdl = false; });
    }
}
__attribute__((used)) __attribute__((visibility("default")))
void setMouseSensitivity(const int mouseSensitivity) {
    Set_Launcher_Setting([=](LauncherSettings& s) { s.hasSensitivity = true; s.sensitivity = mouseSensitivity; });
}
__attribute__((used)) __attribute__((visibility("default")))
void setControllerPointerSpeed(const int controllerPointerSpeed ) {
    Set_Launcher_Setting([=](LauncherSettings& s) { s.hasPointerSpeed = true; s.pointerSpeed = controllerPointerSpeed; });
}
__attribute__((used)) __attribute__((visibility("default")))
void updateRawInputState (const int enableRawInput){
    Set_Launcher_Setting([=](LauncherSettings& s) { s.hasRawInput = true; s.rawInput = enableRawInput != 0; s.rawInputToSdl = true; });
}
}
#endif

SettingsClass::SettingsClass()
{
    /*
    ** Mouse settings
    */
#ifndef ANDROID
    Mouse.RawInput = true;
#else
    Mouse.RawInput = false;
#endif
    Mouse.Sensitivity = 100;
#ifndef ANDROID
    Mouse.ControllerEnabled = false;
#else
    Mouse.ControllerEnabled = true;
#endif
    Options.MouseWheelScrolling = true;
    Options.CampaignAllTech = false;

    Mouse.ControllerPointerSpeed = 10;

    /*
    ** Video settings
    */
    Video.WindowWidth = 640;
    Video.WindowHeight = 400;
    Video.Windowed = false;
    Video.Width = 0;
    Video.Height = 0;
#ifndef ANDROID
    Video.Boxing = true;
#else
    Video.Boxing = false;
#endif
    Video.GameWidth = 640;
    Video.GameHeight = 400;
    Video.BoxingAspectRatio = "16:10";
    Video.FrameLimit = 120;
    Video.InterpolationMode = 2;
    Video.HardwareCursor = false;
#ifndef ANDROID
    Video.DOSMode = false;
#else
    Video.DOSMode = true;
#endif
    Video.SmoothMotion = true;
    Video.Scaler = "nearest";
    Video.Driver = "default";
    Video.PixelFormat = "default";
}

void SettingsClass::Load(INIClass& ini)
{
    char buf[128];
#if ANDROID
    Apply_Launcher_Settings(); // (what the launcher set before: then the ini, as when the setters wrote Settings)
#endif
    Options.CampaignAllTech = ini.Get_Bool("Options", "CampaignAllTech", Options.CampaignAllTech);

    /*
    ** Mouse settings
    */
#ifndef ANDROID
    Mouse.RawInput = ini.Get_Bool("Mouse", "RawInput", Mouse.RawInput);
#else
    Mouse.RawInput = !g_onScreenControlsActive && ini.Get_Bool("Mouse", "RawInput", Mouse.RawInput);
#endif
#ifndef ANDROID
    Mouse.Sensitivity = ini.Get_Int("Mouse", "Sensitivity", Mouse.Sensitivity);
    Mouse.ControllerEnabled = ini.Get_Bool("Mouse", "ControllerEnabled", Mouse.ControllerEnabled);
    Mouse.ControllerPointerSpeed = ini.Get_Int("Mouse", "ControllerPointerSpeed", Mouse.ControllerPointerSpeed);
#endif
    /*
    ** Compatibility with CNCNet configuration for this feature
    */
    Options.MouseWheelScrolling = ini.Get_Bool("Options", "MouseWheelScrolling", Options.MouseWheelScrolling);
    Options.MouseWheelScrolling = ini.Get_Bool("Mouse", "MouseWheelScrolling", Options.MouseWheelScrolling);

    /*
    ** Video settings
    */
    Video.WindowWidth = ini.Get_Int("Video", "WindowWidth", Video.WindowWidth);
    Video.WindowHeight = ini.Get_Int("Video", "WindowHeight", Video.WindowHeight);
    Video.Windowed = ini.Get_Bool("Video", "Windowed", Video.Windowed);
#ifndef ANDROID
    Video.Boxing = ini.Get_Bool("Video", "Boxing", Video.Boxing);
#else
    Video.Boxing = false;
#endif
    Video.BoxingAspectRatio = ini.Get_String("Video", "BoxingAspectRatio", Video.BoxingAspectRatio);
    Video.Width = ini.Get_Int("Video", "Width", Video.Width);
    Video.Height = ini.Get_Int("Video", "Height", Video.Height);
#ifndef ANDROID
    Video.FrameLimit = ini.Get_Int("Video", "FrameLimit", Video.FrameLimit);
    Video.HardwareCursor = ini.Get_Bool("Video", "HardwareCursor", Video.HardwareCursor);
    Video.DOSMode = ini.Get_Bool("Video", "DOSMode", Video.DOSMode);
#endif
    Video.GameWidth = Bound(ini.Get_Int("Video", "GameWidth", Video.GameWidth), 640, 1280);
    Video.GameHeight = Bound(ini.Get_Int("Video", "GameHeight", Video.GameHeight), 400, 800);
    Video.SmoothMotion = ini.Get_Bool("Video", "SmoothMotion", Video.SmoothMotion);
    Video.Scaler = ini.Get_String("Video", "Scaler", Video.Scaler);
    Video.Driver = ini.Get_String("Video", "Driver", Video.Driver);
    Video.PixelFormat = ini.Get_String("Video", "PixelFormat", Video.PixelFormat);

    /*
    ** VQA and WSA interpolation mode 0 = scanlines, 1 = vertical doubling, 2 = linear
    */
    Video.InterpolationMode = Bound(ini.Get_Int("Video", "InterpolationMode", Video.InterpolationMode), 0, 2);

    /*
    ** Boxing and raw input require software cursor.
    */
    if (Video.Boxing || Mouse.RawInput || Mouse.ControllerEnabled) {
        Video.HardwareCursor = false;
    }

    ini.Get_String("Video", "ButtonStyle", "Default", buf, sizeof(buf));
    if (!stricmp(buf, "Gold")) {
        Video.ButtonStyle = 1;
    } else if (!stricmp(buf, "Classic") || !stricmp(buf, "DOS")) {
        Video.ButtonStyle = 0;
    } else {
        Video.ButtonStyle = -1;
    }
}

void SettingsClass::Save(INIClass& ini)
{
    ini.Put_Bool("Options", "CampaignAllTech", Options.CampaignAllTech);
    /*
    ** Mouse settings
    */
    ini.Put_Bool("Mouse", "RawInput", Mouse.RawInput);
    ini.Put_Int("Mouse", "Sensitivity", Mouse.Sensitivity);
    ini.Put_Bool("Mouse", "ControllerEnabled", Mouse.ControllerEnabled);
    ini.Put_Int("Mouse", "ControllerPointerSpeed", Mouse.ControllerPointerSpeed);
    ini.Put_Bool("Mouse", "MouseWheelScrolling", Options.MouseWheelScrolling);

    /*
    ** Video settings
    */
    ini.Put_Int("Video", "WindowWidth", Video.WindowWidth);
    ini.Put_Int("Video", "WindowHeight", Video.WindowHeight);
    ini.Put_Bool("Video", "Windowed", Video.Windowed);
    ini.Put_Bool("Video", "Boxing", Video.Boxing);
    ini.Put_String("Video", "BoxingAspectRatio", Video.BoxingAspectRatio);
    ini.Put_Int("Video", "Width", Video.Width);
    ini.Put_Int("Video", "Height", Video.Height);
    ini.Put_Int("Video", "GameWidth", Video.GameWidth);
    ini.Put_Int("Video", "GameHeight", Video.GameHeight);
    ini.Put_Int("Video", "FrameLimit", Video.FrameLimit);
    ini.Put_Bool("Video", "HardwareCursor", Video.HardwareCursor);
    ini.Put_Bool("Video", "DOSMode", Video.DOSMode);
    ini.Put_Bool("Video", "SmoothMotion", Video.SmoothMotion);
    ini.Put_String("Video", "Scaler", Video.Scaler);
    ini.Put_String("Video", "Driver", Video.Driver);
    ini.Put_String("Video", "PixelFormat", Video.PixelFormat);

    /*
    ** VQA and WSA interpolation mode 0 = scanlines, 1 = vertical doubling, 2 = linear
    */
    ini.Put_Int("Video", "InterpolationMode", Video.InterpolationMode);

    ini.Put_String(
        "Video", "ButtonStyle", Video.ButtonStyle == -1 ? "Default" : (Video.ButtonStyle == 1 ? "Gold" : "Classic"));
}
