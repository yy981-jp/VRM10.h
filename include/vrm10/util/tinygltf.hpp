#pragma once

// Include tiny_gltf.h before this file, or make it available on the include path.
#include <tiny_gltf.h>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>

namespace vrm10::util {

// Converts TinyGLTF's JSON-like Value to nlohmann::json.
// Binary TinyGLTF values are rejected because extension JSON values must be JSON data.
inline nlohmann::json toJson(const tinygltf::Value& value) {
	if (value.IsBool()) return nlohmann::json(value.Get<bool>());
	if (value.IsInt()) return nlohmann::json(value.Get<int>());
	if (value.IsReal()) return nlohmann::json(value.Get<double>());
	if (value.IsString()) return nlohmann::json(value.Get<std::string>());
	if (value.IsArray()) {
		auto result = nlohmann::json::array();
		result.get_ref<nlohmann::json::array_t&>().reserve(value.ArrayLen());
		for (std::size_t index = 0; index < value.ArrayLen(); ++index) {
			result.push_back(toJson(value.Get(index)));
		}
		return result;
	}
	if (value.IsObject()) {
		auto result = nlohmann::json::object();
		for (const auto& key : value.Keys()) {
			result[key] = toJson(value.Get(key));
		}
		return result;
	}
	if (value.Type() == tinygltf::NULL_TYPE) return nlohmann::json(nullptr);
	throw std::invalid_argument("TinyGLTF Value is not a JSON value.");
}

} // namespace vrm10::util
