#ifndef COMMON_MAPZOOM_H
#define COMMON_MAPZOOM_H

/*
** Map zoom: the tactical view is drawn over fewer cells, in the top left part of its area on the screen, and the video
** output magnifies that part over the whole area - the sidebar and the rest of the interface stay as they are. Pointer
** positions inside the area are mapped back to the game pixels they show.
**
** The factor (1: none, up to 4) comes from the android launcher through JNA (setMapZoom) or from the VANILLA_MAP_ZOOM
** environment variable. The game's main loop applies a change (DisplayClass::Apply_Map_Zoom), and only the frames it draws
** the map for (GScreenClass::Render) are magnified: dialogs and menus over the map are shown and clicked as they are.
*/
float Map_Zoom_Factor();

// The tactical area on the screen and the part of it the view is drawn in (DisplayClass::Set_View_Dimensions)
void Map_Zoom_Set_Area(int x, int y, int area_w, int area_h, int view_w, int view_h);

// The map was drawn for the frame about to be shown (GScreenClass::Render)
void Map_Zoom_Mark_Tactical_Frame();

// For the video output, once per frame shown: whether to magnify the view over the area, and their rectangles.
// world_frame: the frame is the smooth motion compositor's world frame (shown between the game's ticks; a dialog drawn over
// the map invalidates it), which is the map too
bool Map_Zoom_Begin_Frame(bool world_frame, int& x, int& y, int& area_w, int& area_h, int& view_w, int& view_h);

// A pointer position (game pixels on the screen) to the game pixel it shows
void Map_Zoom_To_Game(int& x, int& y);

#endif
