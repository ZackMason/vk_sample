
local World = _World();

_world = World.new("waveworld");

local entity_folder = "res/entity/"

_world.prefabs = {
    cultist = load_prefab(entity_folder .. "cultist.entt"),
    fire    = load_prefab(entity_folder .. "fire.entt"),
    player  = load_prefab(entity_folder .. "player.entt"),
}

_world.time = 0

_world.init = function() 
    alert("Initializing " .. _world._user);
end

_world.update = function(dt: number) : nil
    _world.time += dt;

    if _world.init ~= nil then
        _world:init();
        _world.init = nil;
    end

    -- alert(`Update {_world._user} - {_world.time}`)

    -- print("Update " .. _world._user)
end

alert("World Created!");

-- return _world;
