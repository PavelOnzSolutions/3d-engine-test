#include <Engine/Assets/GlbLoader.h>
#include <Engine/Core/Logger.h>
#include <filesystem>
#include <vector>
#include <string>

// Use official cgltf (single-header). We define implementation in this TU only.
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace fs = std::filesystem;

namespace Engine::Assets {

static void AppendCgltfError(cgltf_result res, std::vector<std::string>& out_warnings, const std::string& ctx)
{
    const char* msg = "unknown";
    switch (res)
    {
        case cgltf_result_success: msg = "success"; break;
        case cgltf_result_invalid_json: msg = "invalid_json"; break;
        case cgltf_result_invalid_gltf: msg = "invalid_gltf"; break;
        case cgltf_result_file_not_found: msg = "file_not_found"; break;
        case cgltf_result_io_error: msg = "io_error"; break;
        case cgltf_result_out_of_memory: msg = "out_of_memory"; break;
        default: break;
    }
    out_warnings.emplace_back(ctx + ": cgltf error = " + msg);
}

bool LoadFirstTriangleMesh(const std::string& scene_path,
                           MeshData& out_mesh,
                           std::vector<std::string>& out_warnings)
{
    out_mesh = MeshData{};
    out_warnings.clear();

    // Basic path checks
    std::error_code ec;
    if (!fs::exists(scene_path, ec))
    {
        out_warnings.emplace_back("Scene file not found: " + scene_path);
        return false;
    }

    cgltf_options options{};
    cgltf_data* data = nullptr;

    // Parse file (supports .glb and .gltf)
    cgltf_result res = cgltf_parse_file(&options, scene_path.c_str(), &data);
    if (res != cgltf_result_success)
    {
        AppendCgltfError(res, out_warnings, std::string("cgltf_parse_file failed for ") + scene_path);
        return false;
    }

    // Load external and buffer data
    res = cgltf_load_buffers(&options, data, scene_path.c_str());
    if (res != cgltf_result_success)
    {
        AppendCgltfError(res, out_warnings, "cgltf_load_buffers failed");
        cgltf_free(data);
        return false;
    }

    // Find first mesh + first primitive of triangles
    const cgltf_mesh* mesh = nullptr;
    const cgltf_primitive* prim = nullptr;
    for (cgltf_size mi = 0; mi < data->meshes_count && !prim; ++mi)
    {
        const cgltf_mesh& m = data->meshes[mi];
        for (cgltf_size pi = 0; pi < m.primitives_count && !prim; ++pi)
        {
            if (m.primitives[pi].type == cgltf_primitive_type_triangles)
            {
                mesh = &m;
                prim = &m.primitives[pi];
            }
        }
    }

    if (!prim)
    {
        out_warnings.emplace_back("No triangle-list primitive found in: " + scene_path);
        cgltf_free(data);
        return false;
    }

    // Locate accessors for POSITION (required), NORMAL and TEXCOORD_0 (optional)
    const cgltf_accessor* acc_pos = nullptr;
    const cgltf_accessor* acc_nrm = nullptr;
    const cgltf_accessor* acc_uv0 = nullptr;
    for (cgltf_size ai = 0; ai < prim->attributes_count; ++ai)
    {
        const cgltf_attribute& a = prim->attributes[ai];
        if (a.type == cgltf_attribute_type_position) acc_pos = a.data;
        else if (a.type == cgltf_attribute_type_normal) acc_nrm = a.data;
        else if (a.type == cgltf_attribute_type_texcoord && a.index == 0) acc_uv0 = a.data;
    }

    if (!acc_pos)
    {
        out_warnings.emplace_back("Mesh primitive has no POSITION attribute in: " + scene_path);
        cgltf_free(data);
        return false;
    }

    const cgltf_size vertex_count = acc_pos->count;
    out_mesh.vertices.resize(static_cast<size_t>(vertex_count));

    // Read vertex attributes
    for (cgltf_size i = 0; i < vertex_count; ++i)
    {
        MeshData::VertexPNC v{};
        cgltf_float p[3] = {0,0,0};
        cgltf_accessor_read_float(acc_pos, i, p, 3);
        v.px = static_cast<float>(p[0]); v.py = static_cast<float>(p[1]); v.pz = static_cast<float>(p[2]);

        if (acc_nrm)
        {
            cgltf_float n[3] = {0,0,1};
            cgltf_accessor_read_float(acc_nrm, i, n, 3);
            v.nx = static_cast<float>(n[0]); v.ny = static_cast<float>(n[1]); v.nz = static_cast<float>(n[2]);
        }
        else { v.nx = 0.f; v.ny = 0.f; v.nz = 1.f; }

        if (acc_uv0)
        {
            cgltf_float uv[2] = {0,0};
            cgltf_accessor_read_float(acc_uv0, i, uv, 2);
            v.u = static_cast<float>(uv[0]); v.v = static_cast<float>(uv[1]);
        }
        else { v.u = 0.f; v.v = 0.f; }

        out_mesh.vertices[static_cast<size_t>(i)] = v;
    }

    // Indices: if none provided, generate 0..N-1
    if (prim->indices)
    {
        const cgltf_accessor* acc_idx = prim->indices;
        const cgltf_size index_count = acc_idx->count;
        out_mesh.indices.resize(static_cast<size_t>(index_count));
        for (cgltf_size i = 0; i < index_count; ++i)
        {
            cgltf_uint idx = cgltf_accessor_read_index(acc_idx, i);
            out_mesh.indices[static_cast<size_t>(i)] = static_cast<uint32_t>(idx);
        }
    }
    else
    {
        const cgltf_size index_count = vertex_count;
        out_mesh.indices.resize(static_cast<size_t>(index_count));
        for (cgltf_size i = 0; i < index_count; ++i)
            out_mesh.indices[static_cast<size_t>(i)] = static_cast<uint32_t>(i);
    }

    cgltf_free(data);

    // Basic sanity: ensure indices are multiple of 3 for triangles
    if (out_mesh.indices.size() % 3 != 0)
    {
        out_warnings.emplace_back("Index count is not a multiple of 3; primitive may not be triangles-only.");
    }

    return !out_mesh.vertices.empty() && !out_mesh.indices.empty();
}

} // namespace Engine::Assets
