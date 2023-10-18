#pragma once

#include "zyy_core.hpp"
#include "uid.hpp"

namespace zyy {
    struct entity_t;
}

struct blackboard_t {
    arena_t* arena=0;
    utl::hash_trie_t<std::string_view, v3f>* points=0;
    utl::hash_trie_t<std::string_view, f32>* floats=0;
    utl::hash_trie_t<std::string_view, b32>* bools=0;

    v3f move = {};
    // stack_buffer<interest_point_t, 16> interest_points = {};
    f32 last_seen{};
};

namespace bt {

enum struct behavior_status {
    SUCCESS, //green
    FAILURE, //red
    RUNNING, //white
    ABORTED, //purple
    INVALID  //gray
};

// todo: remove OOP
struct behavior_t {
    behavior_t* next{0};
    behavior_t* prev{0};

    math::rect2d_t rect{v2f{0.0f}, v2f{0.0f}};

    behavior_status status{behavior_status::INVALID};

    virtual void on_init() {}
    virtual void on_end(behavior_status s) {}
    virtual behavior_status on_update(blackboard_t* blkbrd) { return behavior_status::INVALID; }

    void abort() {
        on_end(behavior_status::ABORTED);
        status = behavior_status::ABORTED;
    }

    void reset() {
        status = behavior_status::INVALID;
    }

    b32 is_running() const {
        return status == behavior_status::RUNNING;
    }
    b32 is_terminated() {
        return status == behavior_status::SUCCESS || status == behavior_status::FAILURE;
    }

    behavior_status tick(blackboard_t* blkbrd) {
        if (status!=behavior_status::RUNNING) {
            on_init();
        }
        status = on_update(blkbrd);
        if (status!=behavior_status::RUNNING) {
            on_end(status);
        }
        return status;
    }
};

struct decorator_t : public behavior_t {
    behavior_t* child;
    decorator_t(behavior_t* b) : child{b} {}
};

struct repeat_t : public decorator_t {
    using decorator_t::decorator_t;
    u32 counter = 0;
    u32 limit = 0;
    behavior_status on_update(blackboard_t* blkbrd) override {
        for (;;) {
            child->tick(blkbrd);
            if (child->status == behavior_status::RUNNING) break;
            if (child->status == behavior_status::FAILURE) return behavior_status::FAILURE;
            if (counter++==limit) return behavior_status::SUCCESS;
        }
    }
};

struct always_succeed_t : public decorator_t {
    using decorator_t::decorator_t;
    behavior_status on_update(blackboard_t* blkbrd) override {
        child->tick(blkbrd);
        return behavior_status::SUCCESS;
    }
};

struct invert_t : public decorator_t {
    using decorator_t::decorator_t;
    behavior_status on_update(blackboard_t* blkbrd) override {
        auto s = child->tick(blkbrd);
        if (s==behavior_status::SUCCESS) {
            return behavior_status::FAILURE;
        }
        if (s==behavior_status::FAILURE) {
            return behavior_status::SUCCESS;
        }
        return s;
    }
};


template <typename Value>
struct set_property_on_fail_t : public decorator_t {
    using decorator_t::decorator_t;
    std::string_view name;
    Value value;
    behavior_status on_update(blackboard_t* blkbrd) override {
        auto s = child->tick(blkbrd);
        if (s==behavior_status::FAILURE) {
            Value* prop;
            if constexpr (std::is_same_v<Value, f32>) {
                prop = utl::hash_get(&blkbrd->floats, name, blkbrd->arena);
            }
            if constexpr (std::is_same_v<Value, b32>) {
                prop = utl::hash_get(&blkbrd->bools, name, blkbrd->arena);
            }
            if constexpr (std::is_same_v<Value, v3f>) {
                prop = utl::hash_get(&blkbrd->points, name, blkbrd->arena);
            }
            assert(prop);

            *prop = value;
        }
        return s;
    }
};

struct composite_t : public behavior_t {
    behavior_t head;

    composite_t() {
        dlist_init(&head);
    }

    void add(behavior_t* b) {
        dlist_insert_as_last(&head, b);
    }
    void remove(behavior_t* b) {
        dlist_remove(b);
    }
    void clear() {
        dlist_init(&head);
    }
};

struct parallel_t : public composite_t {
    enum struct policy {
        REQUIRE_ONE, REQUIRE_ALL, INVALID
    };

    policy success_policy{};
    policy failure_policy{};

    parallel_t(policy s, policy f) 
        : success_policy{s}, failure_policy{f}
    {
    }

    virtual void on_end(behavior_status s) override {
        for (auto* child = head.next;
            child != &head;
            node_next(child)
        ) {
            if (child->is_running()) {
                child->abort();
            }
        }
    }

    virtual behavior_status on_update(blackboard_t* blkbrd) override {
        u64 size = 0;
        u64 succeed_count = 0;
        u64 fail_count = 0;
        for (auto* child = head.next;
            child != &head;
            node_next(child)
        ) {
            size++;
            behavior_status s = child->status;
            if (child->is_terminated() == false) {
                s = child->tick(blkbrd);
            }
            if (s == behavior_status::SUCCESS) {
                succeed_count++;
                if (success_policy == policy::REQUIRE_ONE) {
                    return behavior_status::SUCCESS;
                }
            }
            if (s == behavior_status::FAILURE) {
                fail_count++;
                if (failure_policy == policy::REQUIRE_ONE) {
                    return behavior_status::FAILURE;
                }
            }
        }
        if (size == fail_count) {
            if (failure_policy == policy::REQUIRE_ALL) {
                return behavior_status::FAILURE;
            }
        }
        if (size == succeed_count) {
            if (success_policy == policy::REQUIRE_ALL) {
                return behavior_status::SUCCESS;
            }
        }
        return behavior_status::RUNNING;
    }
};

struct monitor_t : public parallel_t {
    void add_condition(behavior_t* b) {
        dlist_insert(&head, b);
    }
    void add_action(behavior_t* b) {
        dlist_insert_as_last(&head, b);
    }
};

// and node
struct sequence_t : public composite_t {
    behavior_t* itr=0;

    virtual void on_init() override {
        assert(head.next != &head);
        itr = head.next;
    }

    virtual behavior_status on_update(blackboard_t* blkbrd) override {
        for(;;) {
            behavior_status s = itr->tick(blkbrd);
            if (s != behavior_status::SUCCESS) {
                return s;
            }
            itr = itr->next;
            if (itr == &head) {
                return behavior_status::SUCCESS;
            }
        }
        return behavior_status::INVALID;
    }
};

struct filter_t : public sequence_t {
    void add_condition(behavior_t* b) {
        dlist_insert(&head, b);
    }
    void add_action(behavior_t* b) {
        dlist_insert_as_last(&head, b);
    }
};

// or
struct selector_t : public composite_t {
    behavior_t* itr=0;

    virtual void on_init() override {
        assert(head.next != &head);
        itr = head.next;
    }

    virtual behavior_status on_update(blackboard_t* blkbrd) override {
        for(;;) {
            behavior_status s = itr->tick(blkbrd);
            if (s == behavior_status::SUCCESS) {
                return s;
            }
            itr = itr->next;
            if (itr == &head) {
                return behavior_status::FAILURE;
            }
        }
        return behavior_status::INVALID;
    }
};

struct condition_t : public behavior_t {
    std::string_view name;
    virtual behavior_status on_update(blackboard_t* blkbrd) override {
        auto* var = utl::hash_get(&blkbrd->bools, name);
        assert(var);
        if (var && *var) {
            // sequence_t::on_update(blkbrd);
            return behavior_status::SUCCESS;
        }
        return behavior_status::FAILURE;
    }
};

// struct condition_t : public sequence_t {
//     std::string_view name;
//     virtual behavior_status on_update(blackboard_t* blkbrd) override {
//         auto* var = utl::hash_get(&blkbrd->bools, name);
//         assert(var);
//         if (var && *var) {
//             sequence_t::on_update(blkbrd);
//             return behavior_status::SUCCESS;
//         }
//         return behavior_status::FAILURE;
//     }
// };

template <typename Value>
struct value_condition_t : public condition_t {
    Value value;

    virtual behavior_status on_update(blackboard_t* blkbrd) override {
        Value* var;
        if constexpr (std::is_same_v<Value, b32>) {
            var = utl::hash_get(&blkbrd->bools, name);
        }
        if constexpr (std::is_same_v<Value, f32>) {
            var = utl::hash_get(&blkbrd->floats, name);
        }
        if constexpr (std::is_same_v<Value, v3f>) {
            var = utl::hash_get(&blkbrd->points, name);
        }
        assert(var);
        if (var && *var == value) {
            // sequence_t::on_update(blkbrd);
            return behavior_status::SUCCESS;
        }
        return behavior_status::FAILURE;
    }
};

template <typename Value>
struct greater_condition_t : public condition_t {
    Value value;

    virtual behavior_status on_update(blackboard_t* blkbrd) override {
        Value* var;
        if constexpr (std::is_same_v<Value, b32>) {
            var = utl::hash_get(&blkbrd->bools, name);
        }
        if constexpr (std::is_same_v<Value, f32>) {
            var = utl::hash_get(&blkbrd->floats, name);
        }
        if constexpr (std::is_same_v<Value, v3f>) {
            var = utl::hash_get(&blkbrd->points, name);
        }
        assert(var);
        if (var && *var > value) {
            // sequence_t::on_update(blkbrd);
            return behavior_status::SUCCESS;
        }
        return behavior_status::FAILURE;
    }
};

struct active_selector_t : public selector_t {
    virtual void on_init() override {
        assert(head.prev != &head);
        itr = head.prev;
    }

    virtual behavior_status on_update(blackboard_t* blkbrd) override {
        auto* last = itr;
        selector_t::on_init();
        auto result = selector_t::on_update(blkbrd);
        if (last != &head && itr != last) {
            last->abort();
        }
        return result;
    }
};

struct behavior_tree_t {
    behavior_t* root{0};
    blackboard_t* blackboard{0};

    f32 tick_rate{0.16f};
    f32 _timer{0.0f};

    void add(behavior_t* b) {
        root = b;
    }

    void tick(f32 dt, blackboard_t* blkbrd) {
        _timer += dt;
        while (_timer >= tick_rate) {
            _timer -= tick_rate;
            root->tick(blkbrd);
        }
    }
};

struct builder_t {
    arena_t* arena;
    behavior_tree_t tree{};

    behavior_t* _current = 0;
    active_selector_t* _active_selector = 0;
    sequence_t* _sequence = 0;
    selector_t* _selector = 0;
    filter_t* _filter = 0;
    condition_t* _condition = 0;
    decorator_t* _decorator=0;

    stack_buffer<composite_t*, 32> stack{};

    builder_t& end() {
        stack.pop();
        if (_condition) {
            _condition = 0;
        } else if (_filter) {
            _filter = 0;
        } else 
        if (_sequence) {
            _sequence = 0;
        } else if (_active_selector) {
            _active_selector = 0;
        }
        return *this;
    }

    builder_t& active_selector() {
        tag_struct(_active_selector, active_selector_t, arena);
        auto* top = stack[stack._top==0?0:stack._top-1];
        stack.push(_active_selector);

        if (tree.root==0) {
            tree.add(_active_selector);
        } else {
            top->add(_active_selector);
        }

        return *this;
    }

    builder_t& sequence() {
        auto* top = stack[stack._top==0?0:stack._top-1];
        tag_struct(_sequence, sequence_t, arena);
        stack.push(_sequence);

        top->add(_sequence);

        return *this;
    }

    builder_t& selector() {
        auto* top = stack[stack._top==0?0:stack._top-1];
        tag_struct(_selector, selector_t, arena);

        stack.push(_selector);

        top->add(_selector);
        
        return *this;
    }

    template <typename Child, typename ... Args>
    builder_t& always_succeed(Args&& ... args) {
        auto* top = stack[stack._top==0?0:stack._top-1];
        tag_struct(auto* child, Child, arena, std::forward<Args>(args)...);
        tag_struct(auto* node, always_succeed_t, arena, child);
        
        if (std::is_base_of_v<bt::composite_t, Child>) {
            stack.push(child);
        }
        top->add(node);
        
        return *this;
    }

    template <typename Child, typename ... Args>
    builder_t& invert(Args&& ... args) {
        auto* top = stack[stack._top==0?0:stack._top-1];
        tag_struct(auto* child, Child, arena, std::forward<Args>(args)...);
        tag_struct(auto* node, invert_t, arena, child);
        
        if (std::is_base_of_v<bt::composite_t, Child>) {
            stack.push(child);
        }
        top->add(node);
        
        return *this;
    }

    builder_t& condition(std::string_view cond) {
        auto* top = stack[stack._top==0?0:stack._top-1];
        tag_struct(_condition, condition_t, arena);
        _condition->name = cond;
        // stack.push(_condition);

        top->add(_condition);
        
        return *this;
    }

    template <typename T>
    builder_t& condition(std::string_view name, T value) {
        auto* top = stack[stack._top==0?0:stack._top-1];
        tag_struct(auto* cond, value_condition_t<T>, arena);

        cond->name = name;
        cond->value = value;
        _condition = cond;

        // stack.push(_condition);

        top->add(_condition);
        
        return *this;
    }

    template <typename T>
    builder_t& greater_than(std::string_view name, T value) {
        auto* top = stack[stack._top==0?0:stack._top-1];
        tag_struct(auto* cond, greater_condition_t<T>, arena);

        cond->name = name;
        cond->value = value;
        _condition = cond;

        // stack.push(_condition);

        top->add(_condition);
        
        return *this;
    }

    template <typename Action, typename ... Args>
    builder_t& action(Args&& ... args) {
        auto* top = stack[stack._top==0?0:stack._top-1];
        tag_struct(auto* action, Action, arena, std::forward<Args>(args)...);

        top->add(action);
        
        return *this;
    }

    
};

}

DEFINE_TYPED_ID(brain_id);

enum struct brain_type {
    player,
    flyer,
    chaser,
    shooter,
    person,

    invalid
};

enum struct interest_type {
    entity,
    sound,
    size,
};

struct interest_point_t {
    v3f point{0.0f};
    void* data{0};
    interest_type type{interest_type::size};
};

struct ai_path_t {
    stack_buffer<v3f, 16> path = {};
};

struct skull_brain_t {
    zyy::entity_t* owner{0};
    stack_buffer<skull_brain_t*, 16> neighbors = {};
    stack_buffer<interest_point_t, 16> enemies = {};
};

struct person_brain_t {
    f32 fear{0.2550f};    
    f32 curiosity{};
    bt::behavior_tree_t tree;
};

struct brain_t {
    brain_t* next;
    brain_t* prev;

    brain_id    id{uid::invalid_id};
    brain_type  type{brain_type::invalid};

    blackboard_t blackboard{};

    union {
        skull_brain_t skull{};
        person_brain_t person;
    };

    u64 group{};
    u64 ignore_mask{};
};

struct print_t : public bt::behavior_t {
    std::string_view text;

    print_t(std::string_view t) {
        text = t;
    }

    virtual bt::behavior_status on_update(blackboard_t* blkbrd) override {
        zyy_info("bt::print", text);
        return bt::behavior_status::SUCCESS;
    }
};

struct breakpoint_t : public bt::behavior_t {
    std::string_view text;

    breakpoint_t(std::string_view t = "unnamed") {
        text = t;
    }

    virtual bt::behavior_status on_update(blackboard_t* blkbrd) override {
        __debugbreak();
        return bt::behavior_status::SUCCESS;
    }
};

struct move_toward_t : public bt::behavior_t {
    std::string_view text;

    move_toward_t(std::string_view t = "target") {
        text = t;
    }

    virtual bt::behavior_status on_update(blackboard_t* blkbrd) override {
        zyy_info("bt::move_toward", text);

        auto* target = utl::hash_get(&blkbrd->points, text, blkbrd->arena);
        auto* self = utl::hash_get(&blkbrd->points, "self"sv, blkbrd->arena);
        
        if (target && self) {
            v3f delta = *target - *self;
            blkbrd->move = delta;

            return bt::behavior_status::SUCCESS;
        }        
        return bt::behavior_status::FAILURE;
    }
};

struct run_away_t : public bt::behavior_t {
    std::string_view text;

    run_away_t(std::string_view t = "target") {
        text = t;
    }

    virtual bt::behavior_status on_update(blackboard_t* blkbrd) override {
        zyy_info("bt::run_away", text);
        auto* target = utl::hash_get(&blkbrd->points, text, blkbrd->arena);
        auto* self = utl::hash_get(&blkbrd->points, "self"sv, blkbrd->arena);

        if (target && self) {
            v3f delta = *target - *self;
            blkbrd->move = -delta;

            return bt::behavior_status::SUCCESS;
        }        
        return bt::behavior_status::FAILURE;
    }
};


static void 
person_init(arena_t* arena, brain_t* brain) { 
    brain->person = {};
    brain->blackboard = {};
    brain->blackboard.arena = arena;

    bt::builder_t builder{.arena = arena};

    builder
        .active_selector()
            .sequence()
                .condition("has_target")
                .action<print_t>("has target")
                .selector()
                    .sequence()
                        .greater_than("fear", 0.5f)
                        .action<run_away_t>("target")
                    .end()
                    .action<move_toward_t>("target")
                .end()
                .always_succeed<bt::sequence_t>()
                    .condition("has_weapon")
                    .condition("has_range")
                    .action<breakpoint_t>("Attack")
                    .action<print_t>("attack")
                .end()
            .end()
            .sequence()
                .action<print_t>("looking for target")
                .action<move_toward_t>("rng_move")
            .end()
        .end();

    brain->person.tree = builder.tree;
}

static void
brain_init(arena_t* arena, zyy::entity_t* entity, brain_t* brain) {
    switch(brain->type) {
        case brain_type::flyer: brain->skull = {.owner = entity}; break;
        case brain_type::person: person_init(arena, brain); break;
    };
}
