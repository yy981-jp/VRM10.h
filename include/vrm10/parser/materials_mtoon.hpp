#pragma once

#include <vrm10/detail/json_reader.hpp>
#include <vrm10/model.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace vrm10 {
namespace detail {

inline constexpr std::array<std::pair<std::string_view, OutlineWidthMode>, 3> OutlineWidthModeValues{{
	{"none", OutlineWidthMode::None},
	{"worldCoordinates", OutlineWidthMode::WorldCoordinates},
	{"screenCoordinates", OutlineWidthMode::ScreenCoordinates}
}};

inline Result<TextureInfo> parseTextureInfo(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	auto index = readRequiredUint32(json, "index", path);
	if (!index) return index.error();
	auto texCoord = readOptionalUint32(json, "texCoord", path, 0);
	if (!texCoord) return texCoord.error();
	return TextureInfo{TextureIndex{index.value()}, TexCoordIndex{texCoord.value()}};
}

inline Result<ShadingShiftTextureInfo> parseShadingShiftTextureInfo(const Json& json, const std::string& path) {
	auto texture = parseTextureInfo(json, path);
	if (!texture) return texture.error();
	auto scale = readOptionalFloat(json, "scale", path, 1.0f);
	if (!scale) return scale.error();
	return ShadingShiftTextureInfo{std::move(texture).value(), scale.value()};
}

inline Result<std::array<float, 3>> readColorFactor(const Json& object, std::string_view key, const std::string& parentPath, const std::array<float, 3>& fallback) {
	if (!object.is_object()) return fail<std::array<float, 3>>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	const auto it = object.find(std::string(key));
	if (it == object.end()) return fallback;
	auto value = readFloatArray<3>(*it, childPath(parentPath, key), fallback, true);
	if (!value) return value.error();
	for (std::size_t index = 0; index < value.value().size(); ++index) {
		if (value.value()[index] < 0.0f || value.value()[index] > 1.0f) {
			return fail<std::array<float, 3>>(
				ErrorCode::InvalidValue,
				childPath(childPath(parentPath, key), index),
				"Color components must be between zero and one."
			);
		}
	}
	return value.value();
}

inline Result<std::int32_t> readRenderQueueOffset(const Json& object, const std::string& path) {
	const auto it = object.find("renderQueueOffsetNumber");
	if (it == object.end()) return 0;
	if (!it->is_number_integer()) {
		return fail<std::int32_t>(ErrorCode::InvalidType, childPath(path, "renderQueueOffsetNumber"), "Expected an integer.");
	}
	std::int64_t value = 0;
	if (it->is_number_unsigned()) {
		const auto unsignedValue = it->get<std::uint64_t>();
		if (unsignedValue > 9) {
			return fail<std::int32_t>(ErrorCode::InvalidValue, childPath(path, "renderQueueOffsetNumber"), "Value must be between minus nine and nine.");
		}
		value = static_cast<std::int64_t>(unsignedValue);
	} else {
		value = it->get<std::int64_t>();
	}
	if (value < -9 || value > 9) {
		return fail<std::int32_t>(ErrorCode::InvalidValue, childPath(path, "renderQueueOffsetNumber"), "Value must be between minus nine and nine.");
	}
	return static_cast<std::int32_t>(value);
}

inline Result<float> readBoundedFloat(
	const Json& object,
	std::string_view key,
	const std::string& path,
	float fallback,
	float minimum,
	float maximum
) {
	auto value = readOptionalFloat(object, key, path, fallback);
	if (!value) return value.error();
	if (value.value() < minimum || value.value() > maximum) {
		return fail<float>(
			ErrorCode::InvalidValue,
			childPath(path, key),
			"Value is outside the allowed range."
		);
	}
	return value.value();
}

} // namespace detail

// Parses a material's extensions.VRMC_materials_mtoon property.
inline Result<MaterialsMToon> parseMaterialsMToon(const nlohmann::json& json) {
	if (auto object = detail::requireObject(json, ""); !object) return object.error();
	MaterialsMToon result;
	auto version = detail::readRequiredString(json, "specVersion", "");
	if (!version) return version.error();
	if (version.value() != "1.0") {
		return Error{ErrorCode::UnsupportedVersion, "/specVersion", "Only VRMC_materials_mtoon specification version 1.0 is supported."};
	}
	result.specVersion = std::move(version).value();

	auto transparent = detail::readOptionalBool(json, "transparentWithZWrite", "", false);
	if (!transparent) return transparent.error();
	result.transparentWithZWrite = transparent.value();
	auto queue = detail::readRenderQueueOffset(json, "");
	if (!queue) return queue.error();
	result.renderQueueOffsetNumber = queue.value();

	auto shade = detail::readColorFactor(json, "shadeColorFactor", "", result.shadeColorFactor);
	if (!shade) return shade.error();
	result.shadeColorFactor = shade.value();
	auto shift = detail::readOptionalFloat(json, "shadingShiftFactor", "", 0.0f);
	if (!shift) return shift.error();
	result.shadingShiftFactor = shift.value();
	auto toony = detail::readBoundedFloat(json, "shadingToonyFactor", "", 0.9f, 0.0f, 1.0f);
	if (!toony) return toony.error();
	result.shadingToonyFactor = toony.value();
	auto gi = detail::readBoundedFloat(json, "giEqualizationFactor", "", 0.9f, 0.0f, 1.0f);
	if (!gi) return gi.error();
	result.giEqualizationFactor = gi.value();

	auto matcap = detail::readColorFactor(json, "matcapFactor", "", result.matcapFactor);
	if (!matcap) return matcap.error();
	result.matcapFactor = matcap.value();
	auto rimColor = detail::readColorFactor(json, "parametricRimColorFactor", "", result.parametricRimColorFactor);
	if (!rimColor) return rimColor.error();
	result.parametricRimColorFactor = rimColor.value();

	auto rimMix = detail::readBoundedFloat(json, "rimLightingMixFactor", "", 1.0f, 0.0f, 1.0f);
	if (!rimMix) return rimMix.error();
	result.rimLightingMixFactor = rimMix.value();
	auto rimPower = detail::readOptionalFloat(json, "parametricRimFresnelPowerFactor", "", 5.0f);
	if (!rimPower) return rimPower.error();
	if (rimPower.value() < 0.0f) {
		return Error{ErrorCode::InvalidValue, "/parametricRimFresnelPowerFactor", "Value must be greater than or equal to zero."};
	}
	result.parametricRimFresnelPowerFactor = rimPower.value();
	auto rimLift = detail::readOptionalFloat(json, "parametricRimLiftFactor", "", 0.0f);
	if (!rimLift) return rimLift.error();
	result.parametricRimLiftFactor = rimLift.value();

	auto outlineMode = detail::readEnum(json, "outlineWidthMode", "", detail::OutlineWidthModeValues, false, OutlineWidthMode::None);
	if (!outlineMode) return outlineMode.error();
	result.outlineWidthMode = outlineMode.value();
	auto outlineWidth = detail::readOptionalFloat(json, "outlineWidthFactor", "", 0.0f);
	if (!outlineWidth) return outlineWidth.error();
	if (outlineWidth.value() < 0.0f) {
		return Error{ErrorCode::InvalidValue, "/outlineWidthFactor", "Value must be greater than or equal to zero."};
	}
	result.outlineWidthFactor = outlineWidth.value();
	auto outlineColor = detail::readColorFactor(json, "outlineColorFactor", "", result.outlineColorFactor);
	if (!outlineColor) return outlineColor.error();
	result.outlineColorFactor = outlineColor.value();
	auto outlineMix = detail::readBoundedFloat(json, "outlineLightingMixFactor", "", 1.0f, 0.0f, 1.0f);
	if (!outlineMix) return outlineMix.error();
	result.outlineLightingMixFactor = outlineMix.value();

	auto scrollX = detail::readOptionalFloat(json, "uvAnimationScrollXSpeedFactor", "", 0.0f);
	if (!scrollX) return scrollX.error();
	result.uvAnimationScrollXSpeedFactor = scrollX.value();
	auto scrollY = detail::readOptionalFloat(json, "uvAnimationScrollYSpeedFactor", "", 0.0f);
	if (!scrollY) return scrollY.error();
	result.uvAnimationScrollYSpeedFactor = scrollY.value();
	auto rotationSpeed = detail::readOptionalFloat(json, "uvAnimationRotationSpeedFactor", "", 0.0f);
	if (!rotationSpeed) return rotationSpeed.error();
	result.uvAnimationRotationSpeedFactor = rotationSpeed.value();

	auto parseTexture = [&](const char* key, std::optional<TextureInfo>& destination) -> Result<void*> {
		const auto it = json.find(key);
		if (it == json.end()) return static_cast<void*>(nullptr);
		auto texture = detail::parseTextureInfo(*it, detail::childPath("", key));
		if (!texture) return texture.error();
		destination = std::move(texture).value();
		return static_cast<void*>(nullptr);
	};
	if (auto value = parseTexture("shadeMultiplyTexture", result.shadeMultiplyTexture); !value) return value.error();
	if (auto value = parseTexture("matcapTexture", result.matcapTexture); !value) return value.error();
	if (auto value = parseTexture("rimMultiplyTexture", result.rimMultiplyTexture); !value) return value.error();
	if (auto value = parseTexture("outlineWidthMultiplyTexture", result.outlineWidthMultiplyTexture); !value) return value.error();
	if (auto value = parseTexture("uvAnimationMaskTexture", result.uvAnimationMaskTexture); !value) return value.error();

	if (const auto it = json.find("shadingShiftTexture"); it != json.end()) {
		auto texture = detail::parseShadingShiftTextureInfo(*it, "/shadingShiftTexture");
		if (!texture) return texture.error();
		result.shadingShiftTexture = std::move(texture).value();
	}
	return result;
}

} // namespace vrm10
