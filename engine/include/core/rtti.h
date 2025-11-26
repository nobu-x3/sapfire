#pragma once

#include "core/core.h"

namespace sf::rtti {
    enum class rtti_type { U8, U16, U32, U64, I8, I16, I32, I64, F32, F64, STRING, BOOL, VEC3, REFERENCE };

    enum class rtti_reference_type {
        None,
        Mesh,
        Texture,
        Material,
        SkinnedData,
    };

    struct rtti_field {
        size_t offset = 0;
        stl::string name;
        rtti_type type = rtti_type::U8;
        stl::function<void()> setter = nullptr;
        rtti_reference_type ref_type = rtti_reference_type::None;
    };

    struct rtti_object {
        void* head{nullptr};
        stl::vector<rtti_field> fields{mem::MemTag::RTTI, 1};
    };

    void set_rtti_field_value(rtti_object* obj, rtti_field* field, void* value);
    void get_rtti_field_value(const rtti_object* obj, const rtti_field* field, void** value);

} // namespace sf::rtti

#define RTTI                                                                                                                               \
public:                                                                                                                                    \
    inline ::sf::rtti::rtti_object& get_rtti() { return m_Rtti; }                                                                          \
                                                                                                                                           \
private:                                                                                                                                   \
    ::sf::rtti::rtti_object m_Rtti;

#define RTTI_COMPONENT                                                                                                                     \
public:                                                                                                                                    \
    inline ::sf::rtti::rtti_object& get_rtti() override { return m_Rtti; }                                                                 \
                                                                                                                                           \
private:                                                                                                                                   \
    ::sf::rtti::rtti_object m_Rtti;

#define ADD_RTTI_FIELD(rtti_type, field_name, data_ptr, extra_setter)                                                                      \
    m_Rtti.fields.push_back(rtti::rtti_field{                                                                                              \
        .offset = static_cast<size_t>(reinterpret_cast<u8*>(data_ptr) - reinterpret_cast<u8*>(this)),                                      \
        .name = field_name,                                                                                                                \
        .type = rtti_type,                                                                                                                 \
        .setter = extra_setter,                                                                                                            \
    });

#define ADD_RTTI_REFERENCE(rtti_reference_type, field_name, data_ptr, extra_setter)                                                        \
    m_Rtti.fields.push_back(rtti::rtti_field{.offset = static_cast<size_t>(reinterpret_cast<u8*>(data_ptr) - reinterpret_cast<u8*>(this)), \
                                             .name = field_name,                                                                           \
                                             .type = ::sf::rtti::rtti_type::REFERENCE,                                                     \
                                             .setter = extra_setter,                                                                       \
                                             .ref_type = rtti_reference_type});

#define BEGIN_RTTI()                                                                                                                       \
    m_Rtti.fields.clear();                                                                                                                 \
    m_Rtti.head = reinterpret_cast<void*>(this);
#define END_RTTI()
