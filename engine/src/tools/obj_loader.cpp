#include "engpch.h"

#include <cfloat>
#include "core/logger.h"
#include "math/math.h"
#include "tiny_obj_loader.h"
#include "tools/obj_loader.h"

namespace sf::tools {
    stl::optional<sf::render::primitives::MeshData> OBJLoader::load_mesh(const stl::string& full_path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes_std;
        std::string warning_std, error_std;
        tinyobj::LoadObj(&attrib, &shapes_std, nullptr, &warning_std, &error_std, full_path.c_str());
        stl::string warning(mem::MemTag::Strings, warning_std);
        stl::string error(mem::MemTag::Strings, error_std);
        stl::vector<tinyobj::shape_t> shapes(mem::MemTag::Mesh);
        for (auto& shape : shapes_std) {
            shapes.push_back(std::move(shape));
        }
        if (!warning.empty()) {
            CORE_WARN("Warning while loading obj file: {}", warning);
        }
        if (!error.empty()) {
            CORE_ERROR("Error while reading obj file: {}", error);
            return {};
        }
        sf::render::primitives::MeshData mesh_data{};
        sf::math::vec3 v_min{+FLT_MAX, +FLT_MAX, +FLT_MAX};
        sf::math::vec3 v_max{-FLT_MAX, -FLT_MAX, -FLT_MAX};
        for (size_t s = 0; s < shapes.size(); s++) {
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                const int fv = 3;
                for (size_t v = 0; v < fv; v++) {
                    const tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    const tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                    const tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                    const tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                    const tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                    const tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                    const tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                    const tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
                    const tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];
                    mesh_data.positions.emplace_back(vx, vy, vz);
                    mesh_data.normals.emplace_back(nx, ny, nz);
                    mesh_data.texcs.emplace_back(ux, uy);
                    mesh_data.indices32.emplace_back(index_offset + v);
                    v_min.x = v_min.x < vx ? v_min.x : vx;
                    v_min.y = v_min.y < vy ? v_min.y : vy;
                    v_min.z = v_min.z < vz ? v_min.z : vz;
                    v_max.x = v_max.x > vx ? v_max.x : vx;
                    v_max.y = v_max.y > vy ? v_max.y : vy;
                    v_max.z = v_max.z > vz ? v_max.z : vz;
                }
                index_offset += fv;
            }
        }
        const sf::math::vec3 center = (v_min + v_max) * 0.5f;
        const sf::math::vec3 extents = (v_max - v_min) * 0.5f;
        mesh_data.aabb = sf::math::aabb(center, extents);
        return mesh_data;
    }
} // namespace sf::tools
