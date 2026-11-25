#include "mapzoom.h"
#include <atomic>
#include <cstdlib>

namespace
{
    // 0: not set through JNA (VANILLA_MAP_ZOOM, or none). Set on the java side's thread on android: atomic
    std::atomic<float> RequestedZoom(0.0f);

    // The rest is the game's thread's (the video output runs on it too)
    struct
    {
        int X, Y, AreaW, AreaH, ViewW, ViewH;
    } Area = {0, 0, 0, 0, 0, 0};
    bool TacticalFrame = false; // the map was drawn for the next frame shown
    bool Magnified = false;     // the last frame shown was magnified: pointer positions are mapped

    float Clamp_Zoom(float zoom)
    {
        if (!(zoom >= 1.0f)) { // (NaN too)
            return 1.0f;
        }
        return zoom > 4.0f ? 4.0f : zoom;
    }
} // namespace

float Map_Zoom_Factor()
{
    float zoom = RequestedZoom.load();
    if (zoom == 0.0f) {
        static const float from_environment = []() {
            const char* value = std::getenv("VANILLA_MAP_ZOOM");
            return value != nullptr ? float(std::atof(value)) : 1.0f;
        }();
        zoom = from_environment;
    }
    return Clamp_Zoom(zoom);
}

void Map_Zoom_Set_Area(int x, int y, int area_w, int area_h, int view_w, int view_h)
{
    Area.X = x;
    Area.Y = y;
    Area.AreaW = area_w;
    Area.AreaH = area_h;
    Area.ViewW = view_w;
    Area.ViewH = view_h;
}

void Map_Zoom_Mark_Tactical_Frame()
{
    TacticalFrame = true;
}

bool Map_Zoom_Begin_Frame(bool world_frame, int& x, int& y, int& area_w, int& area_h, int& view_w, int& view_h)
{
    Magnified = (TacticalFrame || world_frame) && Area.ViewW > 0 && Area.ViewH > 0 && (Area.ViewW < Area.AreaW || Area.ViewH < Area.AreaH);
    TacticalFrame = false;
    x = Area.X;
    y = Area.Y;
    area_w = Area.AreaW;
    area_h = Area.AreaH;
    view_w = Area.ViewW;
    view_h = Area.ViewH;
    return Magnified;
}

void Map_Zoom_To_Game(int& x, int& y)
{
    if (!Magnified || x < Area.X || y < Area.Y || x >= Area.X + Area.AreaW || y >= Area.Y + Area.AreaH) {
        return;
    }
    x = Area.X + (x - Area.X) * Area.ViewW / Area.AreaW;
    y = Area.Y + (y - Area.Y) * Area.ViewH / Area.AreaH;
}

/*
** The android launcher's call (JNA, on the java side's thread): the zoom factor, 1 for none. The game applies it from its
** next frame on.
*/
extern "C" {
#if ANDROID
__attribute__((used)) __attribute__((visibility("default")))
#endif
void setMapZoom(float zoom)
{
    RequestedZoom = zoom > 0.0f ? zoom : 1.0f;
}
}
