
set IncFlags=-Ires/shaders/utl -Ires/shaders/rt
set OptFlags=--target-env=vulkan1.3 --target-spv=spv1.6 -O

glslc %OptFlags% %IncFlags% res/shaders/closesthit.rchit -o res/shaders/bin/closesthit.rchit.spv
glslc %OptFlags% %IncFlags% res/shaders/miss.rmiss -o res/shaders/bin/miss.rmiss.spv
glslc %OptFlags% %IncFlags% res/shaders/anyhit.rahit -o res/shaders/bin/anyhit.rahit.spv
glslc %OptFlags% %IncFlags% res/shaders/shadow.rmiss -o res/shaders/bin/shadow.rmiss.spv
glslc %OptFlags% %IncFlags% res/shaders/raygen.rgen -o res/shaders/bin/raygen.rgen.spv

glslc %OptFlags% %IncFlags% res/shaders/rt_compute.comp -o res/shaders/bin/rt_compute.comp.spv

glslc %OptFlags% %IncFlags% res/shaders/screen.vert -o res/shaders/bin/screen.vert.spv

glslc %OptFlags% %IncFlags% res/shaders/invert.frag -o res/shaders/bin/invert.frag.spv
glslc %OptFlags% %IncFlags% res/shaders/tonemap.frag -o res/shaders/bin/tonemap.frag.spv

glslc %OptFlags% %IncFlags% res/shaders/skeletal.vert -o res/shaders/bin/skeletal.vert.spv
glslc %OptFlags% %IncFlags% res/shaders/simple.vert -o res/shaders/bin/simple.vert.spv
glslc %OptFlags% %IncFlags% res/shaders/simple.frag -o res/shaders/bin/simple.frag.spv
glslc %OptFlags% %IncFlags% res/shaders/error_mesh.frag -o res/shaders/bin/error_mesh.frag.spv

glslc %OptFlags% %IncFlags% res/shaders/skybox.vert -o res/shaders/bin/skybox.vert.spv

glslc %OptFlags% %IncFlags% res/shaders/skybox.frag -o res/shaders/bin/skybox.frag.spv
glslc %OptFlags% %IncFlags% res/shaders/voidsky.frag -o res/shaders/bin/voidsky.frag.spv

glslc %OptFlags% %IncFlags% res/shaders/gui.vert -o res/shaders/bin/gui.vert.spv
glslc %OptFlags% %IncFlags% res/shaders/gui.frag -o res/shaders/bin/gui.frag.spv

glslc %OptFlags% %IncFlags% res/shaders/trail.vert -o res/shaders/bin/trail.vert.spv
glslc %OptFlags% %IncFlags% res/shaders/trail.frag -o res/shaders/bin/trail.frag.spv

glslc %OptFlags% %IncFlags% res/shaders/debug_line.vert -o res/shaders/bin/debug_line.vert.spv
glslc %OptFlags% %IncFlags% res/shaders/debug_line.frag -o res/shaders/bin/debug_line.frag.spv
