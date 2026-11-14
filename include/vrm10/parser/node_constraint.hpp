#pragma once

#include <vrm10/detail/json_reader.hpp>
#include <vrm10/model.hpp>

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace vrm10 {
namespace detail {

inline constexpr std::array<std::pair<std::string_view, RollAxis>, 3> RollAxisValues{{
	{"X", RollAxis::X},
	{"Y", RollAxis::Y},
	{"Z", RollAxis::Z}
}};

inline constexpr std::array<std::pair<std::string_view, AimAxis>, 6> AimAxisValues{{
	{"PositiveX", AimAxis::PositiveX},
	{"NegativeX", AimAxis::NegativeX},
	{"PositiveY", AimAxis::PositiveY},
	{"NegativeY", AimAxis::NegativeY},
	{"PositiveZ", AimAxis::PositiveZ},
	{"NegativeZ", AimAxis::NegativeZ}
}};

template<typename Constraint>
inline Result<float> parseConstraintWeight(const Json& json, const std::string& path) {
	auto weight = readOptionalFloat(json, "weight", path, 1.0f);
	if (!weight) return weight.error();
	if (weight.value() < 0.0f || weight.value() > 1.0f) {
		return fail<float>(ErrorCode::InvalidValue, childPath(path, "weight"), "Weight must be between zero and one.");
	}
	return weight.value();
}

inline Result<RollConstraint> parseRollConstraint(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	auto source = readRequiredUint32(json, "source", path);
	if (!source) return source.error();
	auto axis = readEnum(json, "rollAxis", path, RollAxisValues, true, RollAxis::X);
	if (!axis) return axis.error();
	auto weight = parseConstraintWeight<RollConstraint>(json, path);
	if (!weight) return weight.error();
	return RollConstraint{NodeIndex{source.value()}, axis.value(), weight.value()};
}

inline Result<AimConstraint> parseAimConstraint(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	auto source = readRequiredUint32(json, "source", path);
	if (!source) return source.error();
	auto axis = readEnum(json, "aimAxis", path, AimAxisValues, true, AimAxis::PositiveZ);
	if (!axis) return axis.error();
	auto weight = parseConstraintWeight<AimConstraint>(json, path);
	if (!weight) return weight.error();
	return AimConstraint{NodeIndex{source.value()}, axis.value(), weight.value()};
}

inline Result<RotationConstraint> parseRotationConstraint(const Json& json, const std::string& path) {
	if (auto object = requireObject(json, path); !object) return object.error();
	auto source = readRequiredUint32(json, "source", path);
	if (!source) return source.error();
	auto weight = parseConstraintWeight<RotationConstraint>(json, path);
	if (!weight) return weight.error();
	return RotationConstraint{NodeIndex{source.value()}, weight.value()};
}

} // namespace detail

// Parses the value of a node's extensions.VRMC_node_constraint property.
inline Result<NodeConstraint> parseNodeConstraint(const nlohmann::json& json) {
	if (auto object = detail::requireObject(json, ""); !object) return object.error();

	NodeConstraint result;
	auto version = detail::readRequiredString(json, "specVersion", "");
	if (!version) return version.error();
	if (version.value() != "1.0") {
		return Error{ErrorCode::UnsupportedVersion, "/specVersion", "Only VRMC_node_constraint specification version 1.0 is supported."};
	}
	result.specVersion = std::move(version).value();

	const auto constraintIt = json.find("constraint");
	if (constraintIt == json.end()) {
		return Error{ErrorCode::MissingField, "/constraint", "Required field is missing."};
	}
	if (auto object = detail::requireObject(*constraintIt, "/constraint"); !object) return object.error();

	const auto rollIt = constraintIt->find("roll");
	const auto aimIt = constraintIt->find("aim");
	const auto rotationIt = constraintIt->find("rotation");
	const unsigned int count =
		static_cast<unsigned int>(rollIt != constraintIt->end()) +
		static_cast<unsigned int>(aimIt != constraintIt->end()) +
		static_cast<unsigned int>(rotationIt != constraintIt->end());
	if (count != 1) {
		return Error{ErrorCode::InvalidValue, "/constraint", "Exactly one of roll, aim, or rotation must be specified."};
	}

	if (rollIt != constraintIt->end()) {
		auto parsed = detail::parseRollConstraint(*rollIt, "/constraint/roll");
		if (!parsed) return parsed.error();
		result.constraint = std::move(parsed).value();
	} else if (aimIt != constraintIt->end()) {
		auto parsed = detail::parseAimConstraint(*aimIt, "/constraint/aim");
		if (!parsed) return parsed.error();
		result.constraint = std::move(parsed).value();
	} else {
		auto parsed = detail::parseRotationConstraint(*rotationIt, "/constraint/rotation");
		if (!parsed) return parsed.error();
		result.constraint = std::move(parsed).value();
	}
	return result;
}

} // namespace vrm10
