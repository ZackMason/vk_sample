--!strict

print("Test1 begin");

fire_prefab = load_prefab("res/entity/fire.entt");



local t : Transform = Transform.new();

t:set_origin(1,2,3);

print(t);


local entities = {};

-- local e = Entity:new();

-- e.set_name("Test1");
-- e.set_transform(t);

print(type(e));
-- print(e);

-- for i=0,64,1 do
--     entities[#entities+1] = Entity.new({name = "TEST" .. i});
-- end

alert("Test alert");

print("Test1 end");