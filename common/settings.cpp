#include "wwstd.h"
#include "settings.h"
#include "ini.h"
#include "miscasm.h"

SettingsClass Settings;

#if ANDROID
static bool g_useDosMode = true;
static int g_frameRateLimit = 120;

extern "C" {
__attribute__((used)) __attribute__((visibility("default")))
void setUseDoseModeState(const bool useDosMode) {
    g_useDosMode = useDosMode;
}
__attribute__((used)) __attribute__((visibility("default")))
void setFrameRateLimit(const int frameRateLimit) {
    g_frameRateLimit = frameRateLimit;
}
}
#endif

SettingsClass::SettingsClass()
{
    /*
    ** Mouse settings
    */
    Mouse.RawInput = true;
    Mouse.Sensitivity = 100;
    Mouse.ControllerEnabled = false;
    Mouse.ControllerPointerSpeed = 10;
    Options.MouseWheelScrolling = true;

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
    Video.BoxingAspectRatio = "16:10";
#ifndef ANDROID
    Video.FrameLimit = 120;
#else
    Video.FrameLimit = g_frameRateLimit;
#endif
    Video.InterpolationMode = 2;
    Video.HardwareCursor = false;
#ifndef ANDROID
    Video.DOSMode = false;
#else
    Video.DOSMode = g_useDosMode;
#endif
    Video.Scaler = "nearest";
    Video.Driver = "default";
    Video.PixelFormat = "default";
}

void SettingsClass::Load(INIClass& ini)
{
    char buf[128];

    /*
    ** Mouse settings
    */
    Mouse.RawInput = ini.Get_Bool("Mouse", "RawInput", Mouse.RawInput);
    Mouse.Sensitivity = ini.Get_Int("Mouse", "Sensitivity", Mouse.Sensitivity);
#ifndef ANDROID
    Mouse.ControllerEnabled = ini.Get_Bool("Mouse", "ControllerEnabled", Mouse.ControllerEnabled);
#else
    Mouse.ControllerEnabled = true;
#endif
    Mouse.ControllerPointerSpeed = ini.Get_Int("Mouse", "ControllerPointerSpeed", Mouse.ControllerPointerSpeed);
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
#else
    Video.FrameLimit = g_frameRateLimit;
    Video.HardwareCursor = false;
    Video.DOSMode = g_useDosMode;
#endif
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
    ini.Put_Int("Video", "FrameLimit", Video.FrameLimit);
    ini.Put_Bool("Video", "HardwareCursor", Video.HardwareCursor);
    ini.Put_Bool("Video", "DOSMode", Video.DOSMode);
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
