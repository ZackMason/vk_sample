
struct MeshDesc
{
  int      texture_id;  
  uint64_t vertex_ptr;   
  uint64_t index_ptr;    
  
  uint64_t material_id;  
  uint64_t entity_id;
};

struct Vertex
{
  vec3 p;
  vec3 n;
  vec3 c;
  vec2 t;
};

struct RayData {
  vec3 direction;
  float distance;
  vec3 color;  
};
