#ifndef EVENT_POPUP_H
#define EVENT_POPUP_H

namespace event_popup {
    void MakeEventPopup(int x, int y, int width, int height);
    void MakeEventPopupCentered(int width, int height);
    void EndEventPopup();
    void BeginTurntableFace(int height, float angular_vel, float pitch);
    void BeginBody();
    void EndBody();
    bool Choice(const char* desc, int number, int total);
}

#endif  // EVENT_POPUP_H