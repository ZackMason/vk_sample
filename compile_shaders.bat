
set IncFlags=-Iassets/shaders/utl

glslc %IncFlags% assets/shaders/simple.vert -o assets/shaders/bin/simple.vert.spv
glslc %IncFlags% assets/shaders/simple.frag -o assets/shaders/bin/simple.frag.spv
glslc %IncFlags% assets/shaders/error_mesh.frag -o assets/shaders/bin/error_mesh.frag.spv

glslc %IncFlags% assets/shaders/skybox.vert -o assets/shaders/bin/skybox.vert.spv
glslc %IncFlags% assets/shaders/skybox.frag -o assets/shaders/bin/skybox.frag.spv

glslc %IncFlags% assets/shaders/gui.vert -o assets/shaders/bin/gui.vert.spv
glslc %IncFlags% assets/shaders/gui.frag -o assets/shaders/bin/gui.frag.spv

glslc %IncFlags% assets/shaders/debug_line.vert -o assets/shaders/bin/debug_line.vert.spv
glslc %IncFlags% assets/shaders/debug_line.frag -o assets/shaders/bin/debug_line.frag.spv
