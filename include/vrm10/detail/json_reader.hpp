#pragma once

#include <vrm10/result.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vrm10::detail {

using Json = nlohmann::json;

template<typename T>
Result<T> fail(ErrorCode code, std::string path, std::string message) {
	return Error{code, std::move(path), std::move(message)};
}

inline std::string childPath(const std::string& parent, std::string_view child) {
	std::string escaped;
	escaped.reserve(child.size());
	for (const char ch : child) {
		if (ch == '~') {
			escaped += "~0";
		} else if (ch == '/') {
			escaped += "~1";
		} else {
			escaped += ch;
		}
	}
	return parent + "/" + escaped;
}

inline std::string childPath(const std::string& parent, std::size_t index) {
	return parent + "/" + std::to_string(index);
}

inline Result<void*> requireObject(const Json& json, const std::string& path) {
	if (!json.is_object()) {
		return fail<void*>(ErrorCode::InvalidType, path, "Expected an object.");
	}
	return static_cast<void*>(nullptr);
}

inline Result<void*> requireArray(const Json& json, const std::string& path) {
	if (!json.is_array()) {
		return fail<void*>(ErrorCode::InvalidType, path, "Expected an array.");
	}
	return static_cast<void*>(nullptr);
}

inline Result<std::string> readRequiredString(
	const Json& object,
	std::string_view key,
	const std::string& parentPath
) {
	const auto path = childPath(parentPath, key);
	if (!object.is_object()) {
		return fail<std::string>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) {
		return fail<std::string>(ErrorCode::MissingField, path, "Required field is missing.");
	}
	if (!it->is_string()) {
		return fail<std::string>(ErrorCode::InvalidType, path, "Expected a string.");
	}
	return it->get<std::string>();
}

inline Result<std::string> readOptionalString(
	const Json& object,
	std::string_view key,
	const std::string& parentPath,
	std::string fallback = {}
) {
	const auto path = childPath(parentPath, key);
	if (!object.is_object()) {
		return fail<std::string>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) {
		return fallback;
	}
	if (!it->is_string()) {
		return fail<std::string>(ErrorCode::InvalidType, path, "Expected a string.");
	}
	return it->get<std::string>();
}

inline Result<bool> readOptionalBool(
	const Json& object,
	std::string_view key,
	const std::string& parentPath,
	bool fallback = false
) {
	const auto path = childPath(parentPath, key);
	if (!object.is_object()) {
		return fail<bool>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) {
		return fallback;
	}
	if (!it->is_boolean()) {
		return fail<bool>(ErrorCode::InvalidType, path, "Expected a boolean.");
	}
	return it->get<bool>();
}

inline Result<std::uint32_t> readRequiredUint32(
	const Json& object,
	std::string_view key,
	const std::string& parentPath
) {
	const auto path = childPath(parentPath, key);
	if (!object.is_object()) {
		return fail<std::uint32_t>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) {
		return fail<std::uint32_t>(ErrorCode::MissingField, path, "Required field is missing.");
	}
	if (!it->is_number_integer()) {
		return fail<std::uint32_t>(ErrorCode::InvalidType, path, "Expected a non-negative integer.");
	}
	if (it->is_number_unsigned()) {
		const auto value = it->get<std::uint64_t>();
		if (value > std::numeric_limits<std::uint32_t>::max()) {
			return fail<std::uint32_t>(ErrorCode::InvalidValue, path, "Integer exceeds the uint32 range.");
		}
		return static_cast<std::uint32_t>(value);
	}
	const auto value = it->get<std::int64_t>();
	if (value < 0 || static_cast<std::uint64_t>(value) > std::numeric_limits<std::uint32_t>::max()) {
		return fail<std::uint32_t>(ErrorCode::InvalidValue, path, "Expected a uint32 value.");
	}
	return static_cast<std::uint32_t>(value);
}

inline Result<std::uint32_t> readOptionalUint32(
	const Json& object,
	std::string_view key,
	const std::string& parentPath,
	std::uint32_t fallback = 0
) {
	const auto path = childPath(parentPath, key);
	if (!object.is_object()) {
		return fail<std::uint32_t>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) {
		return fallback;
	}
	if (!it->is_number_integer()) {
		return fail<std::uint32_t>(ErrorCode::InvalidType, path, "Expected a non-negative integer.");
	}
	if (it->is_number_unsigned()) {
		const auto value = it->get<std::uint64_t>();
		if (value > std::numeric_limits<std::uint32_t>::max()) {
			return fail<std::uint32_t>(ErrorCode::InvalidValue, path, "Integer exceeds the uint32 range.");
		}
		return static_cast<std::uint32_t>(value);
	}
	const auto value = it->get<std::int64_t>();
	if (value < 0 || static_cast<std::uint64_t>(value) > std::numeric_limits<std::uint32_t>::max()) {
		return fail<std::uint32_t>(ErrorCode::InvalidValue, path, "Expected a uint32 value.");
	}
	return static_cast<std::uint32_t>(value);
}

inline Result<float> readRequiredFloat(
	const Json& object,
	std::string_view key,
	const std::string& parentPath
) {
	const auto path = childPath(parentPath, key);
	if (!object.is_object()) {
		return fail<float>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) {
		return fail<float>(ErrorCode::MissingField, path, "Required field is missing.");
	}
	if (!it->is_number()) {
		return fail<float>(ErrorCode::InvalidType, path, "Expected a number.");
	}
	const double value = it->get<double>();
	if (!std::isfinite(value) || value > std::numeric_limits<float>::max() || value < -std::numeric_limits<float>::max()) {
		return fail<float>(ErrorCode::InvalidValue, path, "Number cannot be represented as a finite float.");
	}
	return static_cast<float>(value);
}

inline Result<float> readOptionalFloat(
	const Json& object,
	std::string_view key,
	const std::string& parentPath,
	float fallback = 0.0f
) {
	const auto path = childPath(parentPath, key);
	if (!object.is_object()) {
		return fail<float>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) {
		return fallback;
	}
	if (!it->is_number()) {
		return fail<float>(ErrorCode::InvalidType, path, "Expected a number.");
	}
	const double value = it->get<double>();
	if (!std::isfinite(value) || value > std::numeric_limits<float>::max() || value < -std::numeric_limits<float>::max()) {
		return fail<float>(ErrorCode::InvalidValue, path, "Number cannot be represented as a finite float.");
	}
	return static_cast<float>(value);
}

inline Result<std::vector<std::string>> readStringArray(
	const Json& object,
	std::string_view key,
	const std::string& parentPath,
	bool required,
	bool requireNonEmptyItems,
	bool requireNonEmptyArray
) {
	const auto path = childPath(parentPath, key);
	if (!object.is_object()) {
		return fail<std::vector<std::string>>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) {
		if (required) {
			return fail<std::vector<std::string>>(ErrorCode::MissingField, path, "Required field is missing.");
		}
		return std::vector<std::string>{};
	}
	if (!it->is_array()) {
		return fail<std::vector<std::string>>(ErrorCode::InvalidType, path, "Expected an array of strings.");
	}
	if (requireNonEmptyArray && it->empty()) {
		return fail<std::vector<std::string>>(ErrorCode::InvalidValue, path, "Array must contain at least one item.");
	}
	std::vector<std::string> values;
	values.reserve(it->size());
	for (std::size_t index = 0; index < it->size(); ++index) {
		const auto& item = (*it)[index];
		if (!item.is_string()) {
			return fail<std::vector<std::string>>(ErrorCode::InvalidType, childPath(path, index), "Expected a string.");
		}
		auto value = item.get<std::string>();
		if (requireNonEmptyItems && value.empty()) {
			return fail<std::vector<std::string>>(ErrorCode::InvalidValue, childPath(path, index), "String must not be empty.");
		}
		values.push_back(std::move(value));
	}
	return values;
}

template<typename T, std::size_t N>
Result<T> readEnum(
	const Json& object,
	std::string_view key,
	const std::string& parentPath,
	const std::array<std::pair<std::string_view, T>, N>& values,
	bool required,
	T fallback
) {
	const auto path = childPath(parentPath, key);
	if (!object.is_object()) {
		return fail<T>(ErrorCode::InvalidType, parentPath, "Expected an object.");
	}
	const auto it = object.find(std::string(key));
	if (it == object.end()) {
		if (required) {
			return fail<T>(ErrorCode::MissingField, path, "Required field is missing.");
		}
		return fallback;
	}
	if (!it->is_string()) {
		return fail<T>(ErrorCode::InvalidType, path, "Expected a string enum value.");
	}
	const auto text = it->get<std::string>();
	for (const auto& entry : values) {
		if (entry.first == text) {
			return entry.second;
		}
	}
	return fail<T>(ErrorCode::UnknownEnumValue, path, "Unknown enum value: " + text);
}

template<std::size_t N>
Result<std::array<float, N>> readFloatArray(
	const Json& value,
	const std::string& path,
	const std::array<float, N>& fallback,
	bool required
) {
	if (value.is_discarded()) {
		if (required) {
			return fail<std::array<float, N>>(ErrorCode::MissingField, path, "Required field is missing.");
		}
		return fallback;
	}
	if (!value.is_array()) {
		return fail<std::array<float, N>>(ErrorCode::InvalidType, path, "Expected an array.");
	}
	if (value.size() != N) {
		return fail<std::array<float, N>>(ErrorCode::InvalidValue, path, "Array has an invalid number of elements.");
	}
	std::array<float, N> result{};
	for (std::size_t index = 0; index < N; ++index) {
		const auto& item = value[index];
		const auto itemPath = childPath(path, index);
		if (!item.is_number()) {
			return fail<std::array<float, N>>(ErrorCode::InvalidType, itemPath, "Expected a number.");
		}
		const double number = item.get<double>();
		if (!std::isfinite(number) || number > std::numeric_limits<float>::max() || number < -std::numeric_limits<float>::max()) {
			return fail<std::array<float, N>>(ErrorCode::InvalidValue, itemPath, "Number cannot be represented as a finite float.");
		}
		result[index] = static_cast<float>(number);
	}
	return result;
}

} // namespace vrm10::detail
