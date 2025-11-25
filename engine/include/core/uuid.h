#pragma once

#include "core/core.h"

namespace sf {

    class SFAPI UUID {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&) = default;
        operator uint64_t() const { return m_UUID; }

    private:
        uint64_t m_UUID;
    };

} // namespace sf

namespace std {
    template <typename T>
    struct hash;

    template <>
    struct hash<sf::UUID> {
        std::size_t operator()(const sf::UUID& uuid) const { return (uint64_t)uuid; }
    };

} // namespace std
