#ifndef EVENT_POPUP_H
#define EVENT_POPUP_H

#include "render_server.hpp"


namespace event_popup {
    struct PopupEvents {
        bool request_close;
        bool init_drag;
    };
}

struct Popup {
    char title[100];
    char description[1024];
    RID embedded_scene;

    int width, height, face_height;
    float x, y;  // Usefull for dragging
    bool dragging;

    void Draw(event_popup::PopupEvents* events, int relative_offset) const;
};

namespace event_popup {

    // Popup Managing system
    Popup* AddPopup(int width, int height, int face_height);
    void UpdateAllPopups();
    int GetPopupCount();

    // Popup drawing methods
    void MakeEventPopup(int x, int y, int width, int height, const char* title, 
                        PopupEvents* events, int relative_offset);
    void MakeEventPopupCentered(int width, int height, const char* title, PopupEvents* events);
    void EndEventPopup();
    void EmbeddedSceneFace(RID scene_rid, int height);
    void BeginBody();
    void EndBody();
    bool Choice(const char* desc, int number, int total);
}

#endif  // EVENT_POPUP_H