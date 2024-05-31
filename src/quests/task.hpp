#ifndef TASKS_H
#define TASKS_H

#include "basic.hpp"
#include "id_system.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "ui.hpp"

#define TASK_PANEL_HEIGHT 64

struct Task {
    RID departure_planet;
    RID arrival_planet;
    RID current_planet;
    RID ship;

    RID quest;

    double payload_mass;
    timemath::Time pickup_expiration_time;
    timemath::Time delivery_expiration_time;
    cost_t payout;

    Task();
    //void CopyFrom(const Task* other);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    bool IsValid() const;
    button_state_flags::T DrawUI(bool show_as_button, bool highlight) const;
};


#endif  // TASKS_H