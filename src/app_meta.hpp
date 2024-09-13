#ifndef APP_META_H
#define APP_META_H
#include "basic.hpp"

void InternalToggleFullScreen();
void DatedScreenShot();

void ReconfigureWindow();
void AppMetaInit();
void AppMetaStep();
void AppMetaClose();

#endif  // APP_META_H