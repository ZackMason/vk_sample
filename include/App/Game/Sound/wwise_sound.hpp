#pragma once

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/IBytes.h>

#include <AK/SoundEngine/Common/AkMemoryMgr.h>

#include <AK/SoundEngine/Common/AkModule.h>


#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkModule.h>
#include <AK/MusicEngine/Common/AkMusicEngine.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
 
namespace wwise_sound {

    b32 init_sound_engine() {
        AkMemSettings memSettings;
        AK::MemoryMgr::GetDefaultSettings(memSettings);
    
        if ( AK::MemoryMgr::Init( &memSettings ) != AK_Success )
        {
            assert( ! "Could not create the memory manager." );
            return false;
        }

        return true; 
    }
}