#pragma once

#include <vrm10/detail/json_reader.hpp>
#include <vrm10/model.hpp>

namespace vrm10 {

// Parses a material's extensions.KHR_materials_emissive_strength property.
inline Result<MaterialsEmissiveStrength> parseMaterialsEmissiveStrength(const nlohmann::json& json) {
	if (auto object = detail::requireObject(json, ""); !object) return object.error();

	auto strength = detail::readOptionalFloat(json, "emissiveStrength", "", 1.0f);
	if (!strength) return strength.error();
	if (strength.value() < 0.0f) {
		return Error{
			ErrorCode::InvalidValue,
			"/emissiveStrength",
			"Emissive strength must be greater than or equal to zero."
		};
	}
	return MaterialsEmissiveStrength{strength.value()};
}

} // namespace vrm10
