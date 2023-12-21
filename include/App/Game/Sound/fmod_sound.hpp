#pragma once

#include "zyy_core.hpp"

#include "fmod.hpp"
#include "fmod_studio.hpp"

namespace sound_event {
    enum : u64 {
        footstep_dirt,
        jump_dirt,
        land_dirt,

        arcane_bolt,

        SIZE
    };
}

#define fmod_studio_path(path) fmt_sv("./res/audio/sound/Build/Desktop/{}", path).data()

namespace fmod_sound {

    // #define assert_success(exp) (exp)
    #define assert_success(exp) assert((exp) == 0)

    struct sound_engine_t {
        struct {
            FMOD::System*         core_system = 0;
            FMOD::Studio::System* system = 0;
            utl::hash_trie_t<std::string_view, FMOD::ChannelGroup*>* channels = 0;

            std::array<FMOD::Studio::EventDescription*, sound_event::SIZE> events = {};

        } fmod = {};

        auto* emit_event(u64 e) {
            FMOD::Studio::EventInstance* instance = 0;
            assert(fmod.events[e]);
            assert_success(fmod.events[e]->createInstance(&instance));

            assert_success(instance->start());
            assert_success(instance->release());

            return instance;
        }

        utl::hash_trie_t<std::string_view, FMOD::Sound*>* sounds = 0;

        utl::allocator_t allocator = {};

        void play_sound(std::string_view name, std::string_view channel_name = "master") {
            b32 has;
            auto* sound = utl::hash_get(&sounds, name, &allocator.arena, &has);
            if (has == false) {
                auto r = fmod.core_system->createSound(name.data(), FMOD_DEFAULT, 0, sound);
                assert_success(r);
            }

            FMOD::Channel* c;
            fmod.core_system->playSound(*sound, 0, false, &c);
        }
    };

    sound_engine_t* init(arena_t* arena) {
        // FMOD_RESULT result;

        // constexpr auto pool_size = megabytes(128);
        // static_assert((pool_size % 512) == 0);
        // tag_array(auto* bytes, u8, arena, pool_size);


        // result = FMOD::Memory_Initialize(bytes, (int)pool_size, 0, 0, 0);

        // if (result != 0) {
        //     assert(!"Failed to Init FMOD");
        //     return 0;
        // }

        auto memory = begin_temporary_memory(arena);
        tag_struct(auto* engine, sound_engine_t, memory.arena);

        FMOD::Studio::System* system = NULL;
        assert_success( FMOD::Studio::System::create(&system) );
        engine->fmod.system = system;

        // // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
        // FMOD::System* coreSystem = NULL;
        assert_success( system->getCoreSystem(&engine->fmod.core_system) );
        // assert_success( engine->fmod.core_system->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0) );

        assert_success( system->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0) );
        
        FMOD::Studio::Bank* masterBank = NULL;
        assert_success( system->loadBankFile(fmod_studio_path("Master.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank) );
        
        FMOD::Studio::Bank* stringsBank = NULL;
        assert_success( system->loadBankFile(fmod_studio_path("Master.strings.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank) );
        
        FMOD::Studio::Bank* sfxBank = NULL;
        assert_success( system->loadBankFile(fmod_studio_path("SFX.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &sfxBank) );
        sfxBank->loadSampleData();

        assert_success( system->getEvent("event:/Footstep", &engine->fmod.events[sound_event::footstep_dirt]) );
        engine->fmod.events[sound_event::footstep_dirt]->loadSampleData();

        assert_success( system->getEvent("event:/Jump", &engine->fmod.events[sound_event::jump_dirt]) );
        assert_success( system->getEvent("event:/LandDirt", &engine->fmod.events[sound_event::land_dirt]) );
        assert_success( system->getEvent("event:/ArcaneBolt", &engine->fmod.events[sound_event::arcane_bolt]) );


        // auto* master_channel_group = utl::hash_get(&engine->fmod.channels, "Master"sv, arena, 0);
        // result = engine->fmod.core_system->getMasterChannelGroup(master_channel_group);
        // assert_success(result);

        // auto* sfx_channel_group = utl::hash_get(&engine->fmod.channels, "SFX"sv, arena, 0);
        // result = engine->fmod.core_system->createChannelGroup("SFX", sfx_channel_group);
        // assert_success(result);

        // (*master_channel_group)->addGroup(*sfx_channel_group);

        return engine;
    }

    void release(sound_engine_t* engine) {
        engine->fmod.system->release();
        arena_clear(&engine->allocator.arena);
    }

    #undef assert_success
}

#undef fmod_studio_path