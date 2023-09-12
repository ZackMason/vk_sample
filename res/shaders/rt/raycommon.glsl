

struct MeshDesc
{
    int      texture_id;             // Texture index offset in the array of textures
    uint64_t vertex_ptr;         // Address of the Vertex buffer
    uint64_t index_ptr;          // Address of the index buffer
    uint64_t material_ptr;       // Address of the material buffer
    uint64_t material_id;  // Address of the triangle material index buffer
};

struct Vertex
{
  vec3 p;
  vec3 n;
  vec3 c;
  vec2 t;
};

struct RayData {
  Surface surface;
  TotalLight light_solution;
  float distance;
};
