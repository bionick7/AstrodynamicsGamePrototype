#ifndef WREN_INTERFACE_H
#define WREN_INTERFACE_H

#include "basic.hpp"
#include "wren.hpp"

struct WrenInterface {
    WrenInterface();
    ~WrenInterface();

    void MakeVM();
    void LoadQuests();

    WrenVM* vm;
};


#endif  // WREN_INTERFACE_H
