#include "engpch.h"

#include "core/rtti.h"
#include "math/math.h"

#pragma warning(disable : 4244)
namespace sf::rtti {
    void set_rtti_field_value(rtti_object* obj, rtti_field* field, void* value) {
        switch (field->type) {
        case rtti_type::U8:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<u8*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::U16:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<u16*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::U32:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<u32*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::U64:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<u64*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::I8:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<i8*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::I16:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<i16*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::I32:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<i32*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::I64:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<i64*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::F32:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<f32*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::F64:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<f64*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::BOOL:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<bool*>(value);
                *ptr = *data;
                break;
            }
        case rtti_type::STRING:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto* data = static_cast<stl::string*>(value)->data();
                auto* string = reinterpret_cast<stl::string*>(ptr);
                *string = stl::string{data};
                break;
            }
        case rtti_type::VEC3:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto& data = *static_cast<stl::array<f32, 3>*>(value);
                *reinterpret_cast<sf::math::vec3*>(ptr) = sf::math::vec3{data[0], data[1], data[2]};
                break;
            }
        case rtti_type::REFERENCE:
            {
                auto* ptr = static_cast<u8*>(obj->head) + field->offset;
                auto data = *static_cast<sf::UUID*>(value);
                *ptr = data;
                break;
            }
        }
        if (field->setter) {
            field->setter();
        }
    }

    void get_rtti_field_value(const rtti_object* obj, const rtti_field* field, void** value) {
        auto* ptr = static_cast<u8*>(obj->head);
        *value = ptr + field->offset;
    }
} // namespace sf::rtti
#pragma warning(default : 4244)
