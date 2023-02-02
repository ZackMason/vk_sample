
set IncFlags=-I assets/shaders/utl

glslc %IncFlags% assets/shaders/tri.vert -o assets/shaders/bin/tri.vert.spv
glslc %IncFlags% assets/shaders/tri.frag -o assets/shaders/bin/tri.frag.spv

glslc %IncFlags% assets/shaders/simple.vert -o assets/shaders/bin/simple.vert.spv
glslc %IncFlags% assets/shaders/simple.frag -o assets/shaders/bin/simple.frag.spv

glslc %IncFlags% assets/shaders/gui.vert -o assets/shaders/bin/gui.vert.spv
glslc %IncFlags% assets/shaders/gui.frag -o assets/shaders/bin/gui.frag.spv
