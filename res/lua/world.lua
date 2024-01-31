--!strict

-- oop design
-- https://devforum.roblox.com/t/object-oriented-programming-with-luau-in-2023/2135043/9

local World = {_user = nil, update = function(dt: number) return nil end };
World.__index = World;

World.__tostring = function(w: World)
    return "" .. w._user;
end

export type World = typeof(World);


function World.new(_uw) : World
    local self = setmetatable({}, World);

    self._user = _uw;

    return self;
end


return World
