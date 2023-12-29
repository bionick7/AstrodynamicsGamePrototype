#ifndef TASKS_H
#define TASKS_H

#include "basic.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "ui.hpp"

#define TASK_PANEL_HEIGHT 64

struct Task {
    entity_id_t departure_planet;
    entity_id_t arrival_planet;
    entity_id_t current_planet;
    entity_id_t ship;

    entity_id_t quest;

    double payload_mass;
    timemath::Time pickup_expiration_time;
    timemath::Time delivery_expiration_time;
    cost_t payout;

    Task();
    //void CopyFrom(const Task* other);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    bool IsValid() const;
    ButtonStateFlags DrawUI(bool show_as_button, bool highlight) const;
};


#endif  // TASKS_H