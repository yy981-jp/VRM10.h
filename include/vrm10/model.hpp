#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vrm10 {

struct NodeIndex {
	std::uint32_t value{};

	friend constexpr bool operator==(NodeIndex lhs, NodeIndex rhs) noexcept {
		return lhs.value == rhs.value;
	}

	friend constexpr bool operator!=(NodeIndex lhs, NodeIndex rhs) noexcept {
		return !(lhs == rhs);
	}
};

struct MaterialIndex {
	std::uint32_t value{};

	friend constexpr bool operator==(MaterialIndex lhs, MaterialIndex rhs) noexcept {
		return lhs.value == rhs.value;
	}
};

enum class HumanBone : std::uint8_t {
	Hips,
	Spine,
	Chest,
	UpperChest,
	Neck,
	Head,
	LeftEye,
	RightEye,
	Jaw,
	LeftUpperLeg,
	LeftLowerLeg,
	LeftFoot,
	LeftToes,
	RightUpperLeg,
	RightLowerLeg,
	RightFoot,
	RightToes,
	LeftShoulder,
	LeftUpperArm,
	LeftLowerArm,
	LeftHand,
	RightShoulder,
	RightUpperArm,
	RightLowerArm,
	RightHand,
	LeftThumbMetacarpal,
	LeftThumbProximal,
	LeftThumbDistal,
	LeftIndexProximal,
	LeftIndexIntermediate,
	LeftIndexDistal,
	LeftMiddleProximal,
	LeftMiddleIntermediate,
	LeftMiddleDistal,
	LeftRingProximal,
	LeftRingIntermediate,
	LeftRingDistal,
	LeftLittleProximal,
	LeftLittleIntermediate,
	LeftLittleDistal,
	RightThumbMetacarpal,
	RightThumbProximal,
	RightThumbDistal,
	RightIndexProximal,
	RightIndexIntermediate,
	RightIndexDistal,
	RightMiddleProximal,
	RightMiddleIntermediate,
	RightMiddleDistal,
	RightRingProximal,
	RightRingIntermediate,
	RightRingDistal,
	RightLittleProximal,
	RightLittleIntermediate,
	RightLittleDistal,
	Count
};

inline constexpr std::size_t HumanBoneCount = static_cast<std::size_t>(HumanBone::Count);

struct Humanoid {
	std::array<std::optional<NodeIndex>, HumanBoneCount> bones{};

	std::optional<NodeIndex>& operator[](HumanBone bone) noexcept {
		return bones[static_cast<std::size_t>(bone)];
	}

	const std::optional<NodeIndex>& operator[](HumanBone bone) const noexcept {
		return bones[static_cast<std::size_t>(bone)];
	}
};

enum class AvatarPermission : std::uint8_t {
	OnlyAuthor,
	OnlySeparatelyLicensedPerson,
	Everyone
};

enum class CommercialUsage : std::uint8_t {
	PersonalNonProfit,
	PersonalProfit,
	Corporation
};

enum class CreditNotation : std::uint8_t {
	Required,
	Unnecessary
};

enum class Modification : std::uint8_t {
	Prohibited,
	AllowModification,
	AllowModificationRedistribution
};

struct Meta {
	std::string name;
	std::string version;
	std::vector<std::string> authors;
	std::string copyrightInformation;
	std::string contactInformation;
	std::vector<std::string> references;
	std::string thirdPartyLicenses;
	std::optional<std::uint32_t> thumbnailImage;
	std::string licenseUrl;
	AvatarPermission avatarPermission{AvatarPermission::OnlyAuthor};
	bool allowExcessivelyViolentUsage{false};
	bool allowExcessivelySexualUsage{false};
	CommercialUsage commercialUsage{CommercialUsage::PersonalNonProfit};
	bool allowPoliticalOrReligiousUsage{false};
	bool allowAntisocialOrHateUsage{false};
	CreditNotation creditNotation{CreditNotation::Required};
	bool allowRedistribution{false};
	Modification modification{Modification::Prohibited};
	std::string otherLicenseUrl;
};

enum class FirstPersonType : std::uint8_t {
	Auto,
	Both,
	ThirdPersonOnly,
	FirstPersonOnly
};

struct MeshAnnotation {
	NodeIndex node;
	FirstPersonType type{FirstPersonType::Auto};
};

struct FirstPerson {
	std::vector<MeshAnnotation> meshAnnotations;
};

enum class MaterialColorType : std::uint8_t {
	Color,
	EmissionColor,
	ShadeColor,
	MatcapColor,
	RimColor,
	OutlineColor
};

enum class ExpressionOverrideType : std::uint8_t {
	None,
	Block,
	Blend
};

struct MorphTargetBind {
	NodeIndex node;
	std::uint32_t index{};
	float weight{};
};

struct MaterialColorBind {
	MaterialIndex material;
	MaterialColorType type{MaterialColorType::Color};
	std::array<float, 4> targetValue{};
};

struct TextureTransformBind {
	MaterialIndex material;
	std::array<float, 2> scale{1.0f, 1.0f};
	std::array<float, 2> offset{0.0f, 0.0f};
};

struct Expression {
	std::vector<MorphTargetBind> morphTargetBinds;
	std::vector<MaterialColorBind> materialColorBinds;
	std::vector<TextureTransformBind> textureTransformBinds;
	bool isBinary{false};
	ExpressionOverrideType overrideBlink{ExpressionOverrideType::None};
	ExpressionOverrideType overrideLookAt{ExpressionOverrideType::None};
	ExpressionOverrideType overrideMouth{ExpressionOverrideType::None};
};

struct Expressions {
	std::unordered_map<std::string, Expression> preset;
	std::unordered_map<std::string, Expression> custom;
};

enum class LookAtType : std::uint8_t {
	Bone,
	Expression
};

struct LookAtRangeMap {
	float inputMaxValue{};
	float outputScale{};
};

struct LookAt {
	std::optional<std::array<float, 3>> offsetFromHeadBone;
	std::optional<LookAtType> type;
	std::optional<LookAtRangeMap> rangeMapHorizontalInner;
	std::optional<LookAtRangeMap> rangeMapHorizontalOuter;
	std::optional<LookAtRangeMap> rangeMapVerticalDown;
	std::optional<LookAtRangeMap> rangeMapVerticalUp;
};

struct Vrm {
	std::string specVersion;
	Meta meta;
	Humanoid humanoid;
	std::optional<FirstPerson> firstPerson;
	std::optional<Expressions> expressions;
	std::optional<LookAt> lookAt;
};

} // namespace vrm10
