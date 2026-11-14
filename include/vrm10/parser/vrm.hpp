#pragma once

#include <vrm10/detail/json_reader.hpp>
#include <vrm10/model.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace vrm10 {
namespace detail {

struct HumanBoneEntry {
	std::string_view name;
	HumanBone bone;
	bool required;
};

inline constexpr std::array<HumanBoneEntry, HumanBoneCount> HumanBoneEntries{{
	{"hips", HumanBone::Hips, true},
	{"spine", HumanBone::Spine, true},
	{"chest", HumanBone::Chest, false},
	{"upperChest", HumanBone::UpperChest, false},
	{"neck", HumanBone::Neck, false},
	{"head", HumanBone::Head, true},
	{"leftEye", HumanBone::LeftEye, false},
	{"rightEye", HumanBone::RightEye, false},
	{"jaw", HumanBone::Jaw, false},
	{"leftUpperLeg", HumanBone::LeftUpperLeg, true},
	{"leftLowerLeg", HumanBone::LeftLowerLeg, true},
	{"leftFoot", HumanBone::LeftFoot, true},
	{"leftToes", HumanBone::LeftToes, false},
	{"rightUpperLeg", HumanBone::RightUpperLeg, true},
	{"rightLowerLeg", HumanBone::RightLowerLeg, true},
	{"rightFoot", HumanBone::RightFoot, true},
	{"rightToes", HumanBone::RightToes, false},
	{"leftShoulder", HumanBone::LeftShoulder, false},
	{"leftUpperArm", HumanBone::LeftUpperArm, true},
	{"leftLowerArm", HumanBone::LeftLowerArm, true},
	{"leftHand", HumanBone::LeftHand, true},
	{"rightShoulder", HumanBone::RightShoulder, false},
	{"rightUpperArm", HumanBone::RightUpperArm, true},
	{"rightLowerArm", HumanBone::RightLowerArm, true},
	{"rightHand", HumanBone::RightHand, true},
	{"leftThumbMetacarpal", HumanBone::LeftThumbMetacarpal, false},
	{"leftThumbProximal", HumanBone::LeftThumbProximal, false},
	{"leftThumbDistal", HumanBone::LeftThumbDistal, false},
	{"leftIndexProximal", HumanBone::LeftIndexProximal, false},
	{"leftIndexIntermediate", HumanBone::LeftIndexIntermediate, false},
	{"leftIndexDistal", HumanBone::LeftIndexDistal, false},
	{"leftMiddleProximal", HumanBone::LeftMiddleProximal, false},
	{"leftMiddleIntermediate", HumanBone::LeftMiddleIntermediate, false},
	{"leftMiddleDistal", HumanBone::LeftMiddleDistal, false},
	{"leftRingProximal", HumanBone::LeftRingProximal, false},
	{"leftRingIntermediate", HumanBone::LeftRingIntermediate, false},
	{"leftRingDistal", HumanBone::LeftRingDistal, false},
	{"leftLittleProximal", HumanBone::LeftLittleProximal, false},
	{"leftLittleIntermediate", HumanBone::LeftLittleIntermediate, false},
	{"leftLittleDistal", HumanBone::LeftLittleDistal, false},
	{"rightThumbMetacarpal", HumanBone::RightThumbMetacarpal, false},
	{"rightThumbProximal", HumanBone::RightThumbProximal, false},
	{"rightThumbDistal", HumanBone::RightThumbDistal, false},
	{"rightIndexProximal", HumanBone::RightIndexProximal, false},
	{"rightIndexIntermediate", HumanBone::RightIndexIntermediate, false},
	{"rightIndexDistal", HumanBone::RightIndexDistal, false},
	{"rightMiddleProximal", HumanBone::RightMiddleProximal, false},
	{"rightMiddleIntermediate", HumanBone::RightMiddleIntermediate, false},
	{"rightMiddleDistal", HumanBone::RightMiddleDistal, false},
	{"rightRingProximal", HumanBone::RightRingProximal, false},
	{"rightRingIntermediate", HumanBone::RightRingIntermediate, false},
	{"rightRingDistal", HumanBone::RightRingDistal, false},
	{"rightLittleProximal", HumanBone::RightLittleProximal, false},
	{"rightLittleIntermediate", HumanBone::RightLittleIntermediate, false},
	{"rightLittleDistal", HumanBone::RightLittleDistal, false}
}};

inline constexpr std::array<std::pair<std::string_view, AvatarPermission>, 3> AvatarPermissionValues{{
	{"onlyAuthor", AvatarPermission::OnlyAuthor},
	{"onlySeparatelyLicensedPerson", AvatarPermission::OnlySeparatelyLicensedPerson},
	{"everyone", AvatarPermission::Everyone}
}};

inline constexpr std::array<std::pair<std::string_view, CommercialUsage>, 3> CommercialUsageValues{{
	{"personalNonProfit", CommercialUsage::PersonalNonProfit},
	{"personalProfit", CommercialUsage::PersonalProfit},
	{"corporation", CommercialUsage::Corporation}
}};

inline constexpr std::array<std::pair<std::string_view, CreditNotation>, 2> CreditNotationValues{{
	{"required", CreditNotation::Required},
	{"unnecessary", CreditNotation::Unnecessary}
}};

inline constexpr std::array<std::pair<std::string_view, Modification>, 3> ModificationValues{{
	{"prohibited", Modification::Prohibited},
	{"allowModification", Modification::AllowModification},
	{"allowModificationRedistribution", Modification::AllowModificationRedistribution}
}};

inline constexpr std::array<std::pair<std::string_view, FirstPersonType>, 4> FirstPersonTypeValues{{
	{"auto", FirstPersonType::Auto},
	{"both", FirstPersonType::Both},
	{"thirdPersonOnly", FirstPersonType::ThirdPersonOnly},
	{"firstPersonOnly", FirstPersonType::FirstPersonOnly}
}};

inline constexpr std::array<std::pair<std::string_view, MaterialColorType>, 6> MaterialColorTypeValues{{
	{"color", MaterialColorType::Color},
	{"emissionColor", MaterialColorType::EmissionColor},
	{"shadeColor", MaterialColorType::ShadeColor},
	{"matcapColor", MaterialColorType::MatcapColor},
	{"rimColor", MaterialColorType::RimColor},
	{"outlineColor", MaterialColorType::OutlineColor}
}};

inline constexpr std::array<std::pair<std::string_view, ExpressionOverrideType>, 3> ExpressionOverrideValues{{
	{"none", ExpressionOverrideType::None},
	{"block", ExpressionOverrideType::Block},
	{"blend", ExpressionOverrideType::Blend}
}};

inline constexpr std::array<std::pair<std::string_view, LookAtType>, 2> LookAtTypeValues{{
	{"bone", LookAtType::Bone},
	{"expression", LookAtType::Expression}
}};

inline constexpr std::array<std::string_view, 18> PresetExpressionNames{{
	"happy", "angry", "sad", "relaxed", "surprised",
	"aa", "ih", "ou", "ee", "oh",
	"blink", "blinkLeft", "blinkRight",
	"lookUp", "lookDown", "lookLeft", "lookRight",
	"neutral"
}};

inline Result<Meta> parseMeta(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) {
		return object.error();
	}

	Meta result;
	auto name = readRequiredString(json, "name", path);
	if (!name) return name.error();
	if (name.value().empty()) {
		return fail<Meta>(ErrorCode::InvalidValue, childPath(path, "name"), "String must not be empty.");
	}
	result.name = std::move(name).value();

	auto authors = readStringArray(json, "authors", path, true, true, true);
	if (!authors) return authors.error();
	result.authors = std::move(authors).value();

	auto licenseUrl = readRequiredString(json, "licenseUrl", path);
	if (!licenseUrl) return licenseUrl.error();
	result.licenseUrl = std::move(licenseUrl).value();

#define VRM10_READ_META_STRING(field) \
	auto field = readOptionalString(json, #field, path); \
	if (!field) return field.error(); \
	result.field = std::move(field).value()

	VRM10_READ_META_STRING(version);
	VRM10_READ_META_STRING(copyrightInformation);
	VRM10_READ_META_STRING(contactInformation);
	VRM10_READ_META_STRING(thirdPartyLicenses);
	VRM10_READ_META_STRING(otherLicenseUrl);
#undef VRM10_READ_META_STRING

	auto references = readStringArray(json, "references", path, false, false, true);
	if (!references) return references.error();
	result.references = std::move(references).value();

	if (json.contains("thumbnailImage")) {
		auto thumbnail = readOptionalUint32(json, "thumbnailImage", path);
		if (!thumbnail) return thumbnail.error();
		result.thumbnailImage = thumbnail.value();
	}

	auto avatarPermission = readEnum(json, "avatarPermission", path, AvatarPermissionValues, false, AvatarPermission::OnlyAuthor);
	if (!avatarPermission) return avatarPermission.error();
	result.avatarPermission = avatarPermission.value();

	auto commercialUsage = readEnum(json, "commercialUsage", path, CommercialUsageValues, false, CommercialUsage::PersonalNonProfit);
	if (!commercialUsage) return commercialUsage.error();
	result.commercialUsage = commercialUsage.value();

	auto creditNotation = readEnum(json, "creditNotation", path, CreditNotationValues, false, CreditNotation::Required);
	if (!creditNotation) return creditNotation.error();
	result.creditNotation = creditNotation.value();

	auto modification = readEnum(json, "modification", path, ModificationValues, false, Modification::Prohibited);
	if (!modification) return modification.error();
	result.modification = modification.value();

#define VRM10_READ_META_BOOL(field) \
	auto field = readOptionalBool(json, #field, path); \
	if (!field) return field.error(); \
	result.field = field.value()

	VRM10_READ_META_BOOL(allowExcessivelyViolentUsage);
	VRM10_READ_META_BOOL(allowExcessivelySexualUsage);
	VRM10_READ_META_BOOL(allowPoliticalOrReligiousUsage);
	VRM10_READ_META_BOOL(allowAntisocialOrHateUsage);
	VRM10_READ_META_BOOL(allowRedistribution);
#undef VRM10_READ_META_BOOL

	return result;
}

inline Result<Humanoid> parseHumanoid(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) {
		return object.error();
	}
	const auto it = json.find("humanBones");
	const auto bonesPath = childPath(path, "humanBones");
	if (it == json.end()) {
		return fail<Humanoid>(ErrorCode::MissingField, bonesPath, "Required field is missing.");
	}
	if (!it->is_object()) {
		return fail<Humanoid>(ErrorCode::InvalidType, bonesPath, "Expected an object.");
	}

	Humanoid result;
	for (const auto& entry : HumanBoneEntries) {
		const auto bonePath = childPath(bonesPath, entry.name);
		const auto boneIt = it->find(std::string(entry.name));
		if (boneIt == it->end()) {
			if (entry.required) {
				return fail<Humanoid>(ErrorCode::MissingField, bonePath, "Required humanoid bone is missing.");
			}
			continue;
		}
		if (!boneIt->is_object()) {
			return fail<Humanoid>(ErrorCode::InvalidType, bonePath, "Expected a human bone object.");
		}
		auto node = readRequiredUint32(*boneIt, "node", bonePath);
		if (!node) return node.error();
		result[entry.bone] = NodeIndex{node.value()};
	}
	return result;
}

inline Result<FirstPerson> parseFirstPerson(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) {
		return object.error();
	}
	FirstPerson result;
	const auto it = json.find("meshAnnotations");
	if (it == json.end()) return result;
	const auto annotationsPath = childPath(path, "meshAnnotations");
	if (!it->is_array()) {
		return fail<FirstPerson>(ErrorCode::InvalidType, annotationsPath, "Expected an array.");
	}
	if (it->empty()) {
		return fail<FirstPerson>(ErrorCode::InvalidValue, annotationsPath, "Array must contain at least one item when present.");
	}

	result.meshAnnotations.reserve(it->size());
	for (std::size_t index = 0; index < it->size(); ++index) {
		const auto& item = (*it)[index];
		const auto itemPath = childPath(annotationsPath, index);
		if (!item.is_object()) {
			return fail<FirstPerson>(ErrorCode::InvalidType, itemPath, "Expected a mesh annotation object.");
		}
		auto node = readRequiredUint32(item, "node", itemPath);
		if (!node) return node.error();
		auto type = readEnum(item, "type", itemPath, FirstPersonTypeValues, true, FirstPersonType::Auto);
		if (!type) return type.error();
		result.meshAnnotations.push_back(MeshAnnotation{NodeIndex{node.value()}, type.value()});
	}
	return result;
}

inline Result<MorphTargetBind> parseMorphTargetBind(const Json& json, const std::string& path) {
	if (!json.is_object()) {
		return fail<MorphTargetBind>(ErrorCode::InvalidType, path, "Expected an object.");
	}
	auto node = readRequiredUint32(json, "node", path);
	if (!node) return node.error();
	auto index = readRequiredUint32(json, "index", path);
	if (!index) return index.error();
	auto weight = readRequiredFloat(json, "weight", path);
	if (!weight) return weight.error();
	if (weight.value() < 0.0f || weight.value() > 1.0f) {
		return fail<MorphTargetBind>(
			ErrorCode::InvalidValue,
			childPath(path, "weight"),
			"Morph target weight must be between zero and one."
		);
	}
	return MorphTargetBind{NodeIndex{node.value()}, index.value(), weight.value()};
}

inline Result<MaterialColorBind> parseMaterialColorBind(const Json& json, const std::string& path) {
	if (!json.is_object()) {
		return fail<MaterialColorBind>(ErrorCode::InvalidType, path, "Expected an object.");
	}
	auto material = readRequiredUint32(json, "material", path);
	if (!material) return material.error();
	auto type = readEnum(json, "type", path, MaterialColorTypeValues, true, MaterialColorType::Color);
	if (!type) return type.error();
	const auto valueIt = json.find("targetValue");
	const auto valuePath = childPath(path, "targetValue");
	if (valueIt == json.end()) {
		return fail<MaterialColorBind>(ErrorCode::MissingField, valuePath, "Required field is missing.");
	}
	auto targetValue = readFloatArray<4>(*valueIt, valuePath, {}, true);
	if (!targetValue) return targetValue.error();
	return MaterialColorBind{MaterialIndex{material.value()}, type.value(), targetValue.value()};
}

inline Result<TextureTransformBind> parseTextureTransformBind(const Json& json, const std::string& path) {
	if (!json.is_object()) {
		return fail<TextureTransformBind>(ErrorCode::InvalidType, path, "Expected an object.");
	}
	auto material = readRequiredUint32(json, "material", path);
	if (!material) return material.error();
	TextureTransformBind result;
	result.material = MaterialIndex{material.value()};
	if (const auto it = json.find("scale"); it != json.end()) {
		auto scale = readFloatArray<2>(*it, childPath(path, "scale"), {1.0f, 1.0f}, true);
		if (!scale) return scale.error();
		result.scale = scale.value();
	}
	if (const auto it = json.find("offset"); it != json.end()) {
		auto offset = readFloatArray<2>(*it, childPath(path, "offset"), {0.0f, 0.0f}, true);
		if (!offset) return offset.error();
		result.offset = offset.value();
	}
	return result;
}

inline Result<Expression> parseExpression(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) {
		return object.error();
	}
	Expression result;

	const auto morphIt = json.find("morphTargetBinds");
	if (morphIt != json.end()) {
		const auto arrayPath = childPath(path, "morphTargetBinds");
		if (!morphIt->is_array()) return fail<Expression>(ErrorCode::InvalidType, arrayPath, "Expected an array.");
		if (morphIt->empty()) return fail<Expression>(ErrorCode::InvalidValue, arrayPath, "Array must contain at least one item when present.");
		for (std::size_t index = 0; index < morphIt->size(); ++index) {
			auto bind = parseMorphTargetBind((*morphIt)[index], childPath(arrayPath, index));
			if (!bind) return bind.error();
			result.morphTargetBinds.push_back(std::move(bind).value());
		}
	}

	const auto colorIt = json.find("materialColorBinds");
	if (colorIt != json.end()) {
		const auto arrayPath = childPath(path, "materialColorBinds");
		if (!colorIt->is_array()) return fail<Expression>(ErrorCode::InvalidType, arrayPath, "Expected an array.");
		if (colorIt->empty()) return fail<Expression>(ErrorCode::InvalidValue, arrayPath, "Array must contain at least one item when present.");
		for (std::size_t index = 0; index < colorIt->size(); ++index) {
			auto bind = parseMaterialColorBind((*colorIt)[index], childPath(arrayPath, index));
			if (!bind) return bind.error();
			result.materialColorBinds.push_back(std::move(bind).value());
		}
	}

	const auto transformIt = json.find("textureTransformBinds");
	if (transformIt != json.end()) {
		const auto arrayPath = childPath(path, "textureTransformBinds");
		if (!transformIt->is_array()) return fail<Expression>(ErrorCode::InvalidType, arrayPath, "Expected an array.");
		if (transformIt->empty()) return fail<Expression>(ErrorCode::InvalidValue, arrayPath, "Array must contain at least one item when present.");
		for (std::size_t index = 0; index < transformIt->size(); ++index) {
			auto bind = parseTextureTransformBind((*transformIt)[index], childPath(arrayPath, index));
			if (!bind) return bind.error();
			result.textureTransformBinds.push_back(std::move(bind).value());
		}
	}

	auto isBinary = readOptionalBool(json, "isBinary", path);
	if (!isBinary) return isBinary.error();
	result.isBinary = isBinary.value();

	auto overrideBlink = readEnum(json, "overrideBlink", path, ExpressionOverrideValues, false, ExpressionOverrideType::None);
	if (!overrideBlink) return overrideBlink.error();
	result.overrideBlink = overrideBlink.value();
	auto overrideLookAt = readEnum(json, "overrideLookAt", path, ExpressionOverrideValues, false, ExpressionOverrideType::None);
	if (!overrideLookAt) return overrideLookAt.error();
	result.overrideLookAt = overrideLookAt.value();
	auto overrideMouth = readEnum(json, "overrideMouth", path, ExpressionOverrideValues, false, ExpressionOverrideType::None);
	if (!overrideMouth) return overrideMouth.error();
	result.overrideMouth = overrideMouth.value();
	return result;
}

inline Result<Expressions> parseExpressions(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) {
		return object.error();
	}
	Expressions result;
	const auto presetIt = json.find("preset");
	if (presetIt != json.end()) {
		const auto presetPath = childPath(path, "preset");
		if (!presetIt->is_object()) return fail<Expressions>(ErrorCode::InvalidType, presetPath, "Expected an object.");
		for (auto it = presetIt->begin(); it != presetIt->end(); ++it) {
			auto expression = parseExpression(it.value(), childPath(presetPath, it.key()));
			if (!expression) return expression.error();
			result.preset.emplace(it.key(), std::move(expression).value());
		}
	}
	const auto customIt = json.find("custom");
	if (customIt != json.end()) {
		const auto customPath = childPath(path, "custom");
		if (!customIt->is_object()) return fail<Expressions>(ErrorCode::InvalidType, customPath, "Expected an object.");
		for (auto it = customIt->begin(); it != customIt->end(); ++it) {
			auto expression = parseExpression(it.value(), childPath(customPath, it.key()));
			if (!expression) return expression.error();
			result.custom.emplace(it.key(), std::move(expression).value());
		}
	}

	for (const auto& namedExpression : result.custom) {
		bool reservedPresetName = result.preset.find(namedExpression.first) != result.preset.end();
		for (const auto presetName : PresetExpressionNames) {
			if (presetName == namedExpression.first) {
				reservedPresetName = true;
				break;
			}
		}
		if (reservedPresetName) {
			return fail<Expressions>(
				ErrorCode::InvalidValue,
				childPath(childPath(path, "custom"), namedExpression.first),
				"Custom expression names must not reuse a preset expression name."
			);
		}
	}
	return result;
}

inline Result<LookAtRangeMap> parseLookAtRangeMap(const Json& json, const std::string& path) {
	if (!json.is_object()) return fail<LookAtRangeMap>(ErrorCode::InvalidType, path, "Expected an object.");
	auto inputMaxValue = readOptionalFloat(json, "inputMaxValue", path);
	if (!inputMaxValue) return inputMaxValue.error();
	if (inputMaxValue.value() < 0.0f || inputMaxValue.value() > 180.0f) {
		return fail<LookAtRangeMap>(ErrorCode::InvalidValue, childPath(path, "inputMaxValue"), "Value must be between 0 and 180 degrees.");
	}
	auto outputScale = readOptionalFloat(json, "outputScale", path);
	if (!outputScale) return outputScale.error();
	if (outputScale.value() < 0.0f) {
		return fail<LookAtRangeMap>(ErrorCode::InvalidValue, childPath(path, "outputScale"), "Value must be greater than or equal to zero.");
	}
	return LookAtRangeMap{inputMaxValue.value(), outputScale.value()};
}

inline Result<LookAt> parseLookAt(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	LookAt result;

	if (const auto it = json.find("offsetFromHeadBone"); it != json.end()) {
		auto offset = readFloatArray<3>(*it, childPath(path, "offsetFromHeadBone"), {}, true);
		if (!offset) return offset.error();
		result.offsetFromHeadBone = offset.value();
	}

	if (json.contains("type")) {
		auto type = readEnum(json, "type", path, LookAtTypeValues, true, LookAtType::Bone);
		if (!type) return type.error();
		result.type = type.value();
	}

#define VRM10_READ_RANGE_MAP(field) \
	if (const auto it = json.find(#field); it != json.end()) { \
		auto value = parseLookAtRangeMap(*it, childPath(path, #field)); \
		if (!value) return value.error(); \
		result.field = std::move(value).value(); \
	}

	VRM10_READ_RANGE_MAP(rangeMapHorizontalInner);
	VRM10_READ_RANGE_MAP(rangeMapHorizontalOuter);
	VRM10_READ_RANGE_MAP(rangeMapVerticalDown);
	VRM10_READ_RANGE_MAP(rangeMapVerticalUp);
#undef VRM10_READ_RANGE_MAP

	if (result.type && *result.type == LookAtType::Expression) {
#define VRM10_VALIDATE_EXPRESSION_RANGE_MAP(field) \
		if (result.field && result.field->outputScale > 1.0f) { \
			return fail<LookAt>( \
				ErrorCode::InvalidValue, \
				childPath(childPath(path, #field), "outputScale"), \
				"Expression output scale must be between zero and one." \
			); \
		}

		VRM10_VALIDATE_EXPRESSION_RANGE_MAP(rangeMapHorizontalInner);
		VRM10_VALIDATE_EXPRESSION_RANGE_MAP(rangeMapHorizontalOuter);
		VRM10_VALIDATE_EXPRESSION_RANGE_MAP(rangeMapVerticalDown);
		VRM10_VALIDATE_EXPRESSION_RANGE_MAP(rangeMapVerticalUp);
#undef VRM10_VALIDATE_EXPRESSION_RANGE_MAP
	}

	return result;
}

} // namespace detail

// Parses the value of the glTF extensions.VRMC_vrm property, not the full glTF document.
inline Result<Vrm> parseVrm(const nlohmann::json& json) {
	if (auto object = detail::requireObject(json, ""); !object) {
		return object.error();
	}

	Vrm result;
	auto specVersion = detail::readRequiredString(json, "specVersion", "");
	if (!specVersion) return specVersion.error();
	if (specVersion.value() != "1.0") {
		return Error{ErrorCode::UnsupportedVersion, "/specVersion", "Only VRMC_vrm specification version 1.0 is supported."};
	}
	result.specVersion = std::move(specVersion).value();

	const auto metaIt = json.find("meta");
	if (metaIt == json.end()) return Error{ErrorCode::MissingField, "/meta", "Required field is missing."};
	auto meta = detail::parseMeta(*metaIt, "/meta");
	if (!meta) return meta.error();
	result.meta = std::move(meta).value();

	const auto humanoidIt = json.find("humanoid");
	if (humanoidIt == json.end()) return Error{ErrorCode::MissingField, "/humanoid", "Required field is missing."};
	auto humanoid = detail::parseHumanoid(*humanoidIt, "/humanoid");
	if (!humanoid) return humanoid.error();
	result.humanoid = std::move(humanoid).value();

	if (const auto it = json.find("firstPerson"); it != json.end()) {
		auto firstPerson = detail::parseFirstPerson(*it, "/firstPerson");
		if (!firstPerson) return firstPerson.error();
		result.firstPerson = std::move(firstPerson).value();
	}
	if (const auto it = json.find("expressions"); it != json.end()) {
		auto expressions = detail::parseExpressions(*it, "/expressions");
		if (!expressions) return expressions.error();
		result.expressions = std::move(expressions).value();
	}
	if (const auto it = json.find("lookAt"); it != json.end()) {
		auto lookAt = detail::parseLookAt(*it, "/lookAt");
		if (!lookAt) return lookAt.error();
		result.lookAt = std::move(lookAt).value();
	}

	return result;
}

} // namespace vrm10
