#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
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

struct SpringColliderIndex {
	std::uint32_t value{};

	friend constexpr bool operator==(SpringColliderIndex lhs, SpringColliderIndex rhs) noexcept {
		return lhs.value == rhs.value;
	}
};

struct SpringColliderGroupIndex {
	std::uint32_t value{};

	friend constexpr bool operator==(SpringColliderGroupIndex lhs, SpringColliderGroupIndex rhs) noexcept {
		return lhs.value == rhs.value;
	}
};

struct TextureIndex {
	std::uint32_t value{};

	friend constexpr bool operator==(TextureIndex lhs, TextureIndex rhs) noexcept {
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

struct SphereExtendedCollider {
	std::array<float, 3> offset{{0.0f, 0.0f, 0.0f}};
	float radius{0.0f};
	bool inside{false};
};

struct CapsuleExtendedCollider {
	std::array<float, 3> offset{{0.0f, 0.0f, 0.0f}};
	float radius{0.0f};
	std::array<float, 3> tail{{0.0f, 0.0f, 0.0f}};
	bool inside{false};
};

struct PlaneExtendedCollider {
	std::array<float, 3> offset{{0.0f, 0.0f, 0.0f}};
	std::array<float, 3> normal{{0.0f, 0.0f, 1.0f}};
};

using SpringBoneExtendedColliderShape = std::variant<
	SphereExtendedCollider,
	CapsuleExtendedCollider,
	PlaneExtendedCollider
>;

struct SpringBoneExtendedCollider {
	std::string specVersion;
	std::optional<SpringBoneExtendedColliderShape> shape;
};

enum class SpringBoneLimitType : std::uint8_t {
	Cone,
	Hinge,
	Spherical
};

struct ConeLimit {
	float angle{0.0f};
	std::array<float, 4> rotation{{0.0f, 0.0f, 0.0f, 1.0f}};
};

struct HingeLimit {
	float angle{0.0f};
	std::array<float, 4> rotation{{0.0f, 0.0f, 0.0f, 1.0f}};
};

struct SphericalLimit {
	float pitch{0.0f};
	float yaw{0.0f};
	std::array<float, 4> rotation{{0.0f, 0.0f, 0.0f, 1.0f}};
};

using SpringBoneLimitShape = std::variant<ConeLimit, HingeLimit, SphericalLimit>;

struct SpringBoneLimit {
	std::string specVersion;
	SpringBoneLimitShape limit;
};

struct SphereCollider {
	std::array<float, 3> offset{{0.0f, 0.0f, 0.0f}};
	float radius{0.0f};
};

struct CapsuleCollider {
	std::array<float, 3> offset{{0.0f, 0.0f, 0.0f}};
	float radius{0.0f};
	std::array<float, 3> tail{{0.0f, 0.0f, 0.0f}};
};

using SpringColliderShape = std::variant<SphereCollider, CapsuleCollider>;

struct SpringCollider {
	NodeIndex node;
	SpringColliderShape shape;
	std::optional<SpringBoneExtendedCollider> extended;
};

struct SpringColliderGroup {
	std::string name;
	std::vector<SpringColliderIndex> colliders;
};

struct SpringJoint {
	NodeIndex node;
	float hitRadius{0.0f};
	float stiffness{1.0f};
	float gravityPower{0.0f};
	std::array<float, 3> gravityDir{{0.0f, -1.0f, 0.0f}};
	float dragForce{0.5f};
	std::optional<SpringBoneLimit> limit;
};

struct SpringChain {
	std::string name;
	std::vector<SpringJoint> joints;
	std::vector<SpringColliderGroupIndex> colliderGroups;
	std::optional<NodeIndex> center;
};

struct SpringBone {
	std::string specVersion;
	std::vector<SpringCollider> colliders;
	std::vector<SpringColliderGroup> colliderGroups;
	std::vector<SpringChain> springs;
};


enum class RollAxis : std::uint8_t {
	X,
	Y,
	Z
};

enum class AimAxis : std::uint8_t {
	PositiveX,
	NegativeX,
	PositiveY,
	NegativeY,
	PositiveZ,
	NegativeZ
};

struct RollConstraint {
	NodeIndex source;
	RollAxis rollAxis{RollAxis::X};
	float weight{1.0f};
};

struct AimConstraint {
	NodeIndex source;
	AimAxis aimAxis{AimAxis::PositiveZ};
	float weight{1.0f};
};

struct RotationConstraint {
	NodeIndex source;
	float weight{1.0f};
};

using NodeConstraintData = std::variant<RollConstraint, AimConstraint, RotationConstraint>;

struct NodeConstraint {
	std::string specVersion;
	NodeConstraintData constraint;
};


struct TextureInfo {
	TextureIndex index;
	std::uint32_t texCoord{0};
};

struct ShadingShiftTextureInfo {
	TextureInfo texture;
	float scale{1.0f};
};

enum class GltfAlphaMode : std::uint8_t {
	Opaque,
	Mask,
	Blend
};

enum class OutlineWidthMode : std::uint8_t {
	None,
	WorldCoordinates,
	ScreenCoordinates
};

struct MaterialsMToon {
	std::string specVersion;
	bool transparentWithZWrite{false};
	std::int32_t renderQueueOffsetNumber{0};
	std::array<float, 3> shadeColorFactor{{1.0f, 1.0f, 1.0f}};
	std::optional<TextureInfo> shadeMultiplyTexture;
	float shadingShiftFactor{0.0f};
	std::optional<ShadingShiftTextureInfo> shadingShiftTexture;
	float shadingToonyFactor{0.9f};
	float giEqualizationFactor{0.9f};
	std::array<float, 3> matcapFactor{{1.0f, 1.0f, 1.0f}};
	std::optional<TextureInfo> matcapTexture;
	std::array<float, 3> parametricRimColorFactor{{0.0f, 0.0f, 0.0f}};
	std::optional<TextureInfo> rimMultiplyTexture;
	float rimLightingMixFactor{1.0f};
	float parametricRimFresnelPowerFactor{5.0f};
	float parametricRimLiftFactor{0.0f};
	OutlineWidthMode outlineWidthMode{OutlineWidthMode::None};
	float outlineWidthFactor{0.0f};
	std::optional<TextureInfo> outlineWidthMultiplyTexture;
	std::array<float, 3> outlineColorFactor{{0.0f, 0.0f, 0.0f}};
	float outlineLightingMixFactor{1.0f};
	std::optional<TextureInfo> uvAnimationMaskTexture;
	float uvAnimationScrollXSpeedFactor{0.0f};
	float uvAnimationScrollYSpeedFactor{0.0f};
	float uvAnimationRotationSpeedFactor{0.0f};
};

struct HdrEmissiveMultiplier {
	float emissiveMultiplier{1.0f};
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
