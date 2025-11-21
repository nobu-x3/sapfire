#include "engpch.h"

#include "animation/skinned_data.h"

namespace Sapfire::anim {
	f32 BoneAnimation::start_time() const {
		// keyframes are sorted by time, so first keyframe gives start time.
		return keyframes.front().time_pos;
	}

	f32 BoneAnimation::end_time() const { return keyframes.back().time_pos; }

	void BoneAnimation::interp(f32 t, sf::math::mat4& M) const {
		if (t <= keyframes.front().time_pos) {
			const sf::math::vec3& S = keyframes.front().scale;
			const sf::math::vec3& P = keyframes.front().position;
			const sf::math::quat Q(keyframes.front().rotation);
			const sf::math::vec3 zero(0.0f, 0.0f, 0.0f);
			M = sf::math::mat4::affine_transformation(S, zero, Q, P);
		} else if (t >= keyframes.back().time_pos) {
			const sf::math::vec3& S = keyframes.back().scale;
			const sf::math::vec3& P = keyframes.back().position;
			const sf::math::quat Q(keyframes.back().rotation);
			const sf::math::vec3 zero(0.0f, 0.0f, 0.0f);
			M = sf::math::mat4::affine_transformation(S, zero, Q, P);
		} else {
			for (u32 i = 0; i < keyframes.size() - 1; ++i) {
				if (t >= keyframes[i].time_pos && t <= keyframes[i + 1].time_pos) {
					float lerpPercent = (t - keyframes[i].time_pos) / (keyframes[i + 1].time_pos - keyframes[i].time_pos);
					const sf::math::vec3& s0 = keyframes[i].scale;
					const sf::math::vec3& s1 = keyframes[i + 1].scale;
					const sf::math::vec3& p0 = keyframes[i].position;
					const sf::math::vec3& p1 = keyframes[i + 1].position;
					const sf::math::quat q0(keyframes[i].rotation);
					const sf::math::quat q1(keyframes[i + 1].rotation);
					sf::math::vec3 S = sf::math::vec3::lerp(s0, s1, lerpPercent);
					sf::math::vec3 P = sf::math::vec3::lerp(p0, p1, lerpPercent);
					sf::math::quat Q = sf::math::quat::slerp(q0, q1, lerpPercent);
					const sf::math::vec3 zero(0.0f, 0.0f, 0.0f);
					M = sf::math::mat4::affine_transformation(S, zero, Q, P);
					break;
				}
			}
		}
	}

	f32 AnimationClip::start_time() const {
		// Find smallest start time over all bones in this clip.
		f32 t = FLT_MAX;
		for (u32 i = 0; i < bone_animations.size(); ++i) {
			t = t < bone_animations[i].start_time() ? t : bone_animations[i].start_time();
		}
		return t;
	}

	f32 AnimationClip::end_time() const {
		f32 t = 0.f;
		for (u32 i = 0; i < bone_animations.size(); ++i) {
			t = t > bone_animations[i].end_time() ? t : bone_animations[i].end_time();
		}
		return t;
	}

	void AnimationClip::interp(f32 t, stl::vector<sf::math::mat4>& bone_transform) const {
		for (int i = 0; i < bone_animations.size(); ++i) {
			bone_animations[i].interp(t, bone_transform[i]);
		}
	}

	SkinnedData::SkinnedData(stl::vector<int>& bone_hierarchy, stl::vector<sf::math::mat4>& bone_offsets,
							 stl::unordered_map<UUID, AnimationClip>& animations) :
		m_BoneHierarchy(bone_hierarchy),
		m_BoneOffsets(bone_offsets), m_Animations(animations) {}

	void SkinnedData::final_transform(UUID clip_uuid, f32 time_pos, stl::vector<sf::math::mat4>& final_transforms) {

		u32 numBones = m_BoneOffsets.size();
		std::vector<sf::math::mat4> toParentTransforms(numBones);
		// Interpolate all the bones of this clip at the given time instance.
		auto clip = m_Animations.find(clip_uuid);
		clip->second.interp(time_pos, toParentTransforms);
		//
		// Traverse the hierarchy and transform all the bones to the root space.
		//
		std::vector<sf::math::mat4> toRootTransforms(numBones);
		// The root bone has index 0.  The root bone has no parent, so its toRootTransform
		// is just its local bone transform.
		toRootTransforms[0] = toParentTransforms[0];
		// Now find the toRootTransform of the children.
		for (u32 i = 1; i < numBones; ++i) {
			sf::math::mat4 toParent = toParentTransforms[i];
			int parentIndex = m_BoneHierarchy[i];
			sf::math::mat4 parentToRoot = toRootTransforms[parentIndex];
			sf::math::mat4 toRoot = toParent * parentToRoot;
			toRootTransforms[i] = toRoot;
		}
		// Premultiply by the bone offset transform to get the final transform.
		for (u32 i = 0; i < numBones; ++i) {
			sf::math::mat4 offset = m_BoneOffsets[i];
			sf::math::mat4 toRoot = toRootTransforms[i];
			sf::math::mat4 finalTransform = offset * toRoot;
			final_transforms[i] = finalTransform.transposed();
		}
	}

} // namespace Sapfire::anim