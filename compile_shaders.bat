
set IncFlags=-Ires/shaders/utl -Ires/shaders/rt

glslc %IncFlags% res/shaders/rt_compute.comp -o res/shaders/bin/rt_compute.comp.spv

glslc %IncFlags% res/shaders/screen.vert -o res/shaders/bin/screen.vert.spv

glslc %IncFlags% res/shaders/invert.frag -o res/shaders/bin/invert.frag.spv
glslc %IncFlags% res/shaders/tonemap.frag -o res/shaders/bin/tonemap.frag.spv

glslc %IncFlags% res/shaders/skeletal.vert -o res/shaders/bin/skeletal.vert.spv
glslc %IncFlags% res/shaders/simple.vert -o res/shaders/bin/simple.vert.spv
glslc %IncFlags% res/shaders/simple.frag -o res/shaders/bin/simple.frag.spv
glslc %IncFlags% res/shaders/error_mesh.frag -o res/shaders/bin/error_mesh.frag.spv

glslc %IncFlags% res/shaders/skybox.vert -o res/shaders/bin/skybox.vert.spv

glslc %IncFlags% res/shaders/skybox.frag -o res/shaders/bin/skybox.frag.spv
glslc %IncFlags% res/shaders/voidsky.frag -o res/shaders/bin/voidsky.frag.spv

glslc %IncFlags% res/shaders/gui.vert -o res/shaders/bin/gui.vert.spv
glslc %IncFlags% res/shaders/gui.frag -o res/shaders/bin/gui.frag.spv

glslc %IncFlags% res/shaders/trail.vert -o res/shaders/bin/trail.vert.spv
glslc %IncFlags% res/shaders/trail.frag -o res/shaders/bin/trail.frag.spv

glslc %IncFlags% res/shaders/debug_line.vert -o res/shaders/bin/debug_line.vert.spv
glslc %IncFlags% res/shaders/debug_line.frag -o res/shaders/bin/debug_line.frag.spv
