#pragma once

namespace ztd {

    struct entity_t;

    struct event_t {
        event_t* next;
        entity_t* entity;


    };

}