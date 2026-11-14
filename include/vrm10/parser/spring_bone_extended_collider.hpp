#pragma once

#include <vrm10/detail/json_reader.hpp>
#include <vrm10/model.hpp>

#include <array>
#include <cmath>
#include <string>
#include <utility>

namespace vrm10 {
namespace detail {

inline Result<SphereExtendedCollider> parseExtendedSphere(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	SphereExtendedCollider result;
	auto offset = readExtendedVector3(json, "offset", path, result.offset);
	if (!offset) return offset.error();
	result.offset = offset.value();
	auto radius = readOptionalFloat(json, "radius", path, 0.0f);
	if (!radius) return radius.error();
	if (radius.value() < 0.0f) {
		return fail<SphereExtendedCollider>(ErrorCode::InvalidValue, childPath(path, "radius"), "Radius must be greater than or equal to zero.");
	}
	result.radius = radius.value();
	auto inside = readOptionalBool(json, "inside", path, false);
	if (!inside) return inside.error();
	result.inside = inside.value();
	return result;
}

inline Result<CapsuleExtendedCollider> parseExtendedCapsule(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	CapsuleExtendedCollider result;
	auto offset = readExtendedVector3(json, "offset", path, result.offset);
	if (!offset) return offset.error();
	result.offset = offset.value();
	auto radius = readOptionalFloat(json, "radius", path, 0.0f);
	if (!radius) return radius.error();
	if (radius.value() < 0.0f) {
		return fail<CapsuleExtendedCollider>(ErrorCode::InvalidValue, childPath(path, "radius"), "Radius must be greater than or equal to zero.");
	}
	result.radius = radius.value();
	auto tail = readExtendedVector3(json, "tail", path, result.tail);
	if (!tail) return tail.error();
	result.tail = tail.value();
	auto inside = readOptionalBool(json, "inside", path, false);
	if (!inside) return inside.error();
	result.inside = inside.value();
	return result;
}

inline Result<PlaneExtendedCollider> parseExtendedPlane(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	PlaneExtendedCollider result;
	auto offset = readExtendedVector3(json, "offset", path, result.offset);
	if (!offset) return offset.error();
	result.offset = offset.value();

	const std::array<float, 3> defaultNormal{{0.0f, 0.0f, 1.0f}};
	auto normal = readExtendedVector3(json, "normal", path, defaultNormal);
	if (!normal) return normal.error();
	result.normal = normal.value();

	double lengthSquared = 0.0;
	for (float component : result.normal) lengthSquared += static_cast<double>(component) * component;
	if (std::abs(lengthSquared - 1.0) > 1.0e-3) {
		return fail<PlaneExtendedCollider>(ErrorCode::InvalidValue, childPath(path, "normal"), "Plane normal must be normalized.");
	}
	return result;
}

inline Result<SpringBoneExtendedColliderShape> parseExtendedColliderShape(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	const auto sphere = json.find("sphere");
	const auto capsule = json.find("capsule");
	const auto plane = json.find("plane");
	const unsigned int count =
		static_cast<unsigned int>(sphere != json.end()) +
		static_cast<unsigned int>(capsule != json.end()) +
		static_cast<unsigned int>(plane != json.end());
	if (count != 1) {
		return fail<SpringBoneExtendedColliderShape>(ErrorCode::InvalidValue, path, "Exactly one of sphere, capsule, or plane must be specified.");
	}
	if (sphere != json.end()) {
		auto parsed = parseExtendedSphere(*sphere, childPath(path, "sphere"));
		if (!parsed) return parsed.error();
		return SpringBoneExtendedColliderShape{std::move(parsed).value()};
	}
	if (capsule != json.end()) {
		auto parsed = parseExtendedCapsule(*capsule, childPath(path, "capsule"));
		if (!parsed) return parsed.error();
		return SpringBoneExtendedColliderShape{std::move(parsed).value()};
	}
	auto parsed = parseExtendedPlane(*plane, childPath(path, "plane"));
	if (!parsed) return parsed.error();
	return SpringBoneExtendedColliderShape{std::move(parsed).value()};
}

} // namespace detail

// Parses a collider's extensions.VRMC_springBone_extended_collider property.
inline Result<SpringBoneExtendedCollider> parseSpringBoneExtendedCollider(const nlohmann::json& json) {
	if (auto object = detail::requireObject(json, ""); !object) return object.error();
	SpringBoneExtendedCollider result;
	auto version = detail::readRequiredString(json, "specVersion", "");
	if (!version) return version.error();
	if (version.value() != "1.0") {
		return Error{ErrorCode::UnsupportedVersion, "/specVersion", "Only VRMC_springBone_extended_collider specification version 1.0 is supported."};
	}
	result.specVersion = std::move(version).value();

	const auto shape = json.find("shape");
	if (shape != json.end()) {
		auto parsed = detail::parseExtendedColliderShape(*shape, "/shape");
		if (!parsed) return parsed.error();
		result.shape = std::move(parsed).value();
	}
	return result;
}

} // namespace vrm10
