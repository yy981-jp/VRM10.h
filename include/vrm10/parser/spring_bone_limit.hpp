#pragma once

#include <vrm10/detail/json_reader.hpp>
#include <vrm10/model.hpp>

#include <array>
#include <cmath>
#include <string>
#include <utility>

namespace vrm10 {
namespace detail {

inline Result<std::array<float, 4>> parseUnitQuaternion(
	const Json& object,
	const std::string& path
) {
	std::array<float, 4> identity{{0.0f, 0.0f, 0.0f, 1.0f}};
	const auto it = object.find("rotation");
	if (it == object.end()) return identity;

	auto value = readFloatArray<4>(*it, childPath(path, "rotation"), identity, true);
	if (!value) return value.error();

	double lengthSquared = 0.0;
	for (std::size_t index = 0; index < value.value().size(); ++index) {
		const float component = value.value()[index];
		if (component < -1.0f || component > 1.0f) {
			return fail<std::array<float, 4>>(
				ErrorCode::InvalidValue,
				childPath(childPath(path, "rotation"), index),
				"Quaternion components must be between minus one and one."
			);
		}
		lengthSquared += static_cast<double>(component) * component;
	}
	if (std::abs(lengthSquared - 1.0) > 1.0e-3) {
		return fail<std::array<float, 4>>(
			ErrorCode::InvalidValue,
			childPath(path, "rotation"),
			"Quaternion must be normalized."
		);
	}
	return value.value();
}

inline Result<ConeLimit> parseConeLimit(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	auto angle = readRequiredFloat(json, "angle", path);
	if (!angle) return angle.error();
	if (angle.value() < 0.0f) {
		return fail<ConeLimit>(ErrorCode::InvalidValue, childPath(path, "angle"), "Angle must be greater than or equal to zero.");
	}
	auto rotation = parseUnitQuaternion(json, path);
	if (!rotation) return rotation.error();
	return ConeLimit{angle.value(), rotation.value()};
}

inline Result<HingeLimit> parseHingeLimit(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	auto angle = readRequiredFloat(json, "angle", path);
	if (!angle) return angle.error();
	if (angle.value() < 0.0f) {
		return fail<HingeLimit>(ErrorCode::InvalidValue, childPath(path, "angle"), "Angle must be greater than or equal to zero.");
	}
	auto rotation = parseUnitQuaternion(json, path);
	if (!rotation) return rotation.error();
	return HingeLimit{angle.value(), rotation.value()};
}

inline Result<SphericalLimit> parseSphericalLimit(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	auto pitch = readRequiredFloat(json, "pitch", path);
	if (!pitch) return pitch.error();
	if (pitch.value() < 0.0f) {
		return fail<SphericalLimit>(ErrorCode::InvalidValue, childPath(path, "pitch"), "Pitch must be greater than or equal to zero.");
	}
	auto yaw = readRequiredFloat(json, "yaw", path);
	if (!yaw) return yaw.error();
	if (yaw.value() < 0.0f) {
		return fail<SphericalLimit>(ErrorCode::InvalidValue, childPath(path, "yaw"), "Yaw must be greater than or equal to zero.");
	}
	auto rotation = parseUnitQuaternion(json, path);
	if (!rotation) return rotation.error();
	return SphericalLimit{pitch.value(), yaw.value(), rotation.value()};
}

} // namespace detail

// Parses the value of a joint's extensions.VRMC_springBone_limit property.
inline Result<SpringBoneLimit> parseSpringBoneLimit(const nlohmann::json& json) {
	if (auto object = detail::requireObject(json, ""); !object) return object.error();

	SpringBoneLimit result;
	auto version = detail::readRequiredString(json, "specVersion", "");
	if (!version) return version.error();
	if (version.value() != "1.0") {
		return Error{ErrorCode::UnsupportedVersion, "/specVersion", "Only VRMC_springBone_limit specification version 1.0 is supported."};
	}
	result.specVersion = std::move(version).value();

	const auto limitIt = json.find("limit");
	if (limitIt == json.end()) {
		return Error{ErrorCode::MissingField, "/limit", "Required field is missing."};
	}
	if (auto object = detail::requireObject(*limitIt, "/limit"); !object) return object.error();

	const auto coneIt = limitIt->find("cone");
	const auto hingeIt = limitIt->find("hinge");
	const auto sphericalIt = limitIt->find("spherical");
	const unsigned int count =
		static_cast<unsigned int>(coneIt != limitIt->end()) +
		static_cast<unsigned int>(hingeIt != limitIt->end()) +
		static_cast<unsigned int>(sphericalIt != limitIt->end());
	if (count != 1) {
		return Error{ErrorCode::InvalidValue, "/limit", "Exactly one of cone, hinge, or spherical must be specified."};
	}

	if (coneIt != limitIt->end()) {
		auto parsed = detail::parseConeLimit(*coneIt, "/limit/cone");
		if (!parsed) return parsed.error();
		result.limit = std::move(parsed).value();
	} else if (hingeIt != limitIt->end()) {
		auto parsed = detail::parseHingeLimit(*hingeIt, "/limit/hinge");
		if (!parsed) return parsed.error();
		result.limit = std::move(parsed).value();
	} else {
		auto parsed = detail::parseSphericalLimit(*sphericalIt, "/limit/spherical");
		if (!parsed) return parsed.error();
		result.limit = std::move(parsed).value();
	}
	return result;
}

} // namespace vrm10
