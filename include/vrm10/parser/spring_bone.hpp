#pragma once

#include <vrm10/detail/json_reader.hpp>
#include <vrm10/model.hpp>
#include <vrm10/parser/spring_bone_extended_collider.hpp>
#include <vrm10/parser/spring_bone_limit.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace vrm10 {
namespace detail {

template<typename Index>
inline Result<Index> readIndexValue(const Json& json, const std::string& path) {
	if (!json.is_number_integer()) {
		return fail<Index>(ErrorCode::InvalidType, path, "Expected a non-negative integer index.");
	}
	if (json.is_number_unsigned()) {
		const auto value = json.get<std::uint64_t>();
		if (value > std::numeric_limits<std::uint32_t>::max()) {
			return fail<Index>(ErrorCode::InvalidValue, path, "Index exceeds the uint32 range.");
		}
		return Index{static_cast<std::uint32_t>(value)};
	}
	const auto value = json.get<std::int64_t>();
	if (value < 0 || static_cast<std::uint64_t>(value) > std::numeric_limits<std::uint32_t>::max()) {
		return fail<Index>(ErrorCode::InvalidValue, path, "Expected a uint32 index.");
	}
	return Index{static_cast<std::uint32_t>(value)};
}

template<typename Index>
inline Result<std::vector<Index>> readIndexArray(const Json& json, const std::string& path) {
	if (!json.is_array()) {
		return fail<std::vector<Index>>(ErrorCode::InvalidType, path, "Expected an array of indices.");
	}
	if (json.empty()) {
		return fail<std::vector<Index>>(ErrorCode::InvalidValue, path, "Array must contain at least one item.");
	}
	std::vector<Index> result;
	result.reserve(json.size());
	for (std::size_t index = 0; index < json.size(); ++index) {
		auto value = readIndexValue<Index>(json[index], childPath(path, index));
		if (!value) return value.error();
		result.push_back(std::move(value).value());
	}
	return result;
}

inline Result<std::array<float, 3>> readOptionalVector3(
	const Json& object,
	std::string_view key,
	const std::string& parentPath,
	const std::array<float, 3>& fallback
) {
	if (!object.is_object()) {
		return fail<std::array<float, 3>>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) return fallback;
	return readFloatArray<3>(*it, childPath(parentPath, key), fallback, true);
}

inline Result<SphereCollider> parseSphereCollider(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	SphereCollider result;
	auto offset = readOptionalVector3(json, "offset", path, result.offset);
	if (!offset) return offset.error();
	result.offset = offset.value();
	auto radius = readOptionalFloat(json, "radius", path, 0.0f);
	if (!radius) return radius.error();
	if (radius.value() < 0.0f) {
		return fail<SphereCollider>(ErrorCode::InvalidValue, childPath(path, "radius"), "Radius must be greater than or equal to zero.");
	}
	result.radius = radius.value();
	return result;
}

inline Result<CapsuleCollider> parseCapsuleCollider(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	CapsuleCollider result;
	auto offset = readOptionalVector3(json, "offset", path, result.offset);
	if (!offset) return offset.error();
	result.offset = offset.value();
	auto radius = readOptionalFloat(json, "radius", path, 0.0f);
	if (!radius) return radius.error();
	if (radius.value() < 0.0f) {
		return fail<CapsuleCollider>(ErrorCode::InvalidValue, childPath(path, "radius"), "Radius must be greater than or equal to zero.");
	}
	result.radius = radius.value();
	auto tail = readOptionalVector3(json, "tail", path, result.tail);
	if (!tail) return tail.error();
	result.tail = tail.value();
	return result;
}

inline Result<SpringColliderShape> parseSpringColliderShape(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	const auto sphereIt = json.find("sphere");
	const auto capsuleIt = json.find("capsule");
	const bool hasSphere = sphereIt != json.end();
	const bool hasCapsule = capsuleIt != json.end();
	if (hasSphere == hasCapsule) {
		return fail<SpringColliderShape>(ErrorCode::InvalidValue, path, "Exactly one of 'sphere' or 'capsule' must be specified.");
	}
	if (hasSphere) {
		auto sphere = parseSphereCollider(*sphereIt, childPath(path, "sphere"));
		if (!sphere) return sphere.error();
		return SpringColliderShape{std::move(sphere).value()};
	}
	auto capsule = parseCapsuleCollider(*capsuleIt, childPath(path, "capsule"));
	if (!capsule) return capsule.error();
	return SpringColliderShape{std::move(capsule).value()};
}

inline Result<SpringCollider> parseSpringCollider(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	auto node = readRequiredUint32(json, "node", path);
	if (!node) return node.error();
	const auto shapeIt = json.find("shape");
	if (shapeIt == json.end()) {
		return fail<SpringCollider>(ErrorCode::MissingField, childPath(path, "shape"), "Required field is missing.");
	}
	auto shape = parseSpringColliderShape(*shapeIt, childPath(path, "shape"));
	if (!shape) return shape.error();
	SpringCollider result{NodeIndex{node.value()}, std::move(shape).value()};
	const auto extensionsIt = json.find("extensions");
	if (extensionsIt != json.end()) {
		if (!extensionsIt->is_object()) {
			return fail<SpringCollider>(ErrorCode::InvalidType, childPath(path, "extensions"), "Expected an object.");
		}
		const auto extendedIt = extensionsIt->find("VRMC_springBone_extended_collider");
		if (extendedIt != extensionsIt->end()) {
			auto extended = parseSpringBoneExtendedCollider(*extendedIt);
			if (!extended) {
				auto error = extended.error();
				error.path = childPath(childPath(path, "extensions"), "VRMC_springBone_extended_collider") + error.path;
				return error;
			}
			result.extended = std::move(extended).value();
		}
	}
	return result;
}

inline Result<SpringColliderGroup> parseSpringColliderGroup(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	SpringColliderGroup result;
	auto name = readOptionalString(json, "name", path);
	if (!name) return name.error();
	result.name = std::move(name).value();
	const auto collidersIt = json.find("colliders");
	if (collidersIt == json.end()) {
		return fail<SpringColliderGroup>(ErrorCode::MissingField, childPath(path, "colliders"), "Required field is missing.");
	}
	auto colliders = readIndexArray<SpringColliderIndex>(*collidersIt, childPath(path, "colliders"));
	if (!colliders) return colliders.error();
	result.colliders = std::move(colliders).value();
	return result;
}

inline Result<SpringJoint> parseSpringJoint(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	auto node = readRequiredUint32(json, "node", path);
	if (!node) return node.error();
	SpringJoint result;
	result.node = NodeIndex{node.value()};
	auto hitRadius = readOptionalFloat(json, "hitRadius", path, 0.0f);
	if (!hitRadius) return hitRadius.error();
	if (hitRadius.value() < 0.0f) {
		return fail<SpringJoint>(ErrorCode::InvalidValue, childPath(path, "hitRadius"), "Value must be greater than or equal to zero.");
	}
	result.hitRadius = hitRadius.value();
	auto stiffness = readOptionalFloat(json, "stiffness", path, 1.0f);
	if (!stiffness) return stiffness.error();
	if (stiffness.value() < 0.0f) {
		return fail<SpringJoint>(ErrorCode::InvalidValue, childPath(path, "stiffness"), "Value must be greater than or equal to zero.");
	}
	result.stiffness = stiffness.value();
	auto gravityPower = readOptionalFloat(json, "gravityPower", path, 0.0f);
	if (!gravityPower) return gravityPower.error();
	if (gravityPower.value() < 0.0f) {
		return fail<SpringJoint>(ErrorCode::InvalidValue, childPath(path, "gravityPower"), "Value must be greater than or equal to zero.");
	}
	result.gravityPower = gravityPower.value();
	auto gravityDir = readOptionalVector3(json, "gravityDir", path, result.gravityDir);
	if (!gravityDir) return gravityDir.error();
	result.gravityDir = gravityDir.value();
	auto dragForce = readOptionalFloat(json, "dragForce", path, 0.5f);
	if (!dragForce) return dragForce.error();
	if (dragForce.value() < 0.0f || dragForce.value() > 1.0f) {
		return fail<SpringJoint>(ErrorCode::InvalidValue, childPath(path, "dragForce"), "Value must be between zero and one.");
	}
	result.dragForce = dragForce.value();

	const extensionsIt = json.find("extensions");
	if (extensionsIt != json.end()) {
		if (!extensionsIt->is_object()) {
			return fail<SpringJoint>(ErrorCode::InvalidType, childPath(path, "extensions"), "Expected an object.");
		}
		const auto limitIt = extensionsIt->find("VRMC_springBone_limit");
		if (limitIt != extensionsIt->end()) {
			auto limit = parseSpringBoneLimit(*limitIt);
			if (!limit) {
				auto error = limit.error();
				error.path = childPath(childPath(path, "extensions"), "VRMC_springBone_limit") + error.path;
				return error;
			}
			result.limit = std::move(limit).value();
		}
	}
	return result;
}

template<typename T, typename Parser>
inline Result<std::vector<T>> parseNonEmptyArray(const Json& json, const std::string& path, Parser parseItem) {
	if (!json.is_array()) return fail<std::vector<T>>(ErrorCode::InvalidType, path, "Expected an array.");
	if (json.empty()) return fail<std::vector<T>>(ErrorCode::InvalidValue, path, "Array must contain at least one item when present.");
	std::vector<T> result;
	result.reserve(json.size());
	for (std::size_t index = 0; index < json.size(); ++index) {
		auto item = parseItem(json[index], childPath(path, index));
		if (!item) return item.error();
		result.push_back(std::move(item).value());
	}
	return result;
}

inline Result<SpringChain> parseSpringChain(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	SpringChain result;
	auto name = readOptionalString(json, "name", path);
	if (!name) return name.error();
	result.name = std::move(name).value();
	const auto jointsIt = json.find("joints");
	if (jointsIt == json.end()) {
		return fail<SpringChain>(ErrorCode::MissingField, childPath(path, "joints"), "Required field is missing.");
	}
	auto joints = parseNonEmptyArray<SpringJoint>(
		*jointsIt,
		childPath(path, "joints"),
		[](const Json& item, const std::string& itemPath) {
			return parseSpringJoint(item, itemPath);
		}
	);
	if (!joints) return joints.error();
	result.joints = std::move(joints).value();
	if (const auto it = json.find("colliderGroups"); it != json.end()) {
		auto groups = readIndexArray<SpringColliderGroupIndex>(*it, childPath(path, "colliderGroups"));
		if (!groups) return groups.error();
		result.colliderGroups = std::move(groups).value();
	}
	if (json.contains("center")) {
		auto center = readRequiredUint32(json, "center", path);
		if (!center) return center.error();
		result.center = NodeIndex{center.value()};
	}
	return result;
}

} // namespace detail

// Parses the value of glTF extensions.VRMC_springBone, not the full glTF document.
inline Result<SpringBone> parseSpringBone(const nlohmann::json& json) {
	if (auto object = detail::requireObject(json, ""); !object) return object.error();
	SpringBone result;
	auto version = detail::readRequiredString(json, "specVersion", "");
	if (!version) return version.error();
	if (version.value() != "1.0") {
		return Error{ErrorCode::UnsupportedVersion, "/specVersion", "Only VRMC_springBone specification version 1.0 is supported."};
	}
	result.specVersion = std::move(version).value();
	if (const auto it = json.find("colliders"); it != json.end()) {
		auto values = detail::parseNonEmptyArray<SpringCollider>(
			*it, "/colliders",
			[](const detail::Json& item, const std::string& path) {
				return detail::parseSpringCollider(item, path);
			}
		);
		if (!values) return values.error();
		result.colliders = std::move(values).value();
	}
	if (const auto it = json.find("colliderGroups"); it != json.end()) {
		auto values = detail::parseNonEmptyArray<SpringColliderGroup>(
			*it, "/colliderGroups",
			[](const detail::Json& item, const std::string& path) {
				return detail::parseSpringColliderGroup(item, path);
			}
		);
		if (!values) return values.error();
		result.colliderGroups = std::move(values).value();
	}
	if (const auto it = json.find("springs"); it != json.end()) {
		auto values = detail::parseNonEmptyArray<SpringChain>(
			*it, "/springs",
			[](const detail::Json& item, const std::string& path) {
				return detail::parseSpringChain(item, path);
			}
		);
		if (!values) return values.error();
		result.springs = std::move(values).value();
	}
	return result;
}

} // namespace vrm10
