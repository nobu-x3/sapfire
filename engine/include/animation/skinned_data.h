#pragma once

#include "math/math.h"

namespace sf::anim {
    struct SFAPI Keyframe {
        f32 time_pos;
        sf::math::vec3 position{0};
        sf::math::vec3 scale{0};
        sf::math::vec4 rotation{0};
    };

    // At least 2 keyframes
    struct SFAPI BoneAnimation {
        f32 start_time() const;
        f32 end_time() const;

        void interp(f32 t, sf::math::mat4& M) const;

        stl::vector<Keyframe> keyframes{mem::MemTag::Animation};
    };

    struct SFAPI AnimationClip {
        f32 start_time() const;
        f32 end_time() const;

        void interp(f32 t, stl::vector<sf::math::mat4>& bone_transform) const;

        stl::vector<BoneAnimation> bone_animations{mem::MemTag::Animation};
        UUID uuid;
    };

    class SFAPI SkinnedData {
    public:
        SkinnedData(stl::vector<int>& bone_hierarchy, stl::vector<sf::math::mat4>& bone_offsets,
                    stl::unordered_map<UUID, AnimationClip>& animations);
        SkinnedData() = default;
        inline u32 bone_count() const { return m_BoneHierarchy.size(); }
        inline f32 clip_start_time(UUID clip_uuid) const { return m_Animations.at(clip_uuid).start_time(); }
        inline f32 clip_end_time(UUID clip_uuid) const { return m_Animations.at(clip_uuid).end_time(); }
        void final_transform(UUID clip_uuid, f32 time_pos, stl::vector<sf::math::mat4>& final_transforms);

    private:
        // Gives parentIndex of ith bone.
        stl::vector<int> m_BoneHierarchy;
        stl::vector<sf::math::mat4> m_BoneOffsets;
        stl::unordered_map<UUID, AnimationClip> m_Animations;
    };
} // namespace sf::anim
