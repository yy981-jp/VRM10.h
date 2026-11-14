#pragma once

#include <vrm10/detail/json_reader.hpp>
#include <vrm10/model.hpp>

#include <string>
#include <utility>

namespace vrm10 {

// Parses a material's extensions.VRMC_materials_hdr_emissiveMultiplier property.
// The extension is archived and superseded by KHR_materials_emissive_strength.
inline Result<HdrEmissiveMultiplier> parseHdrEmissiveMultiplier(const nlohmann::json& json) {
	if (auto object = detail::requireObject(json, ""); !object) return object.error();
	auto multiplier = detail::readRequiredFloat(json, "emissiveMultiplier", "");
	if (!multiplier) return multiplier.error();
	if (multiplier.value() < 0.0f) {
		return Error{ErrorCode::InvalidValue, "/emissiveMultiplier", "Value must be greater than or equal to zero."};
	}
	return HdrEmissiveMultiplier{multiplier.value()};
}

} // namespace vrm10
