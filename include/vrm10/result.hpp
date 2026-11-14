#pragma once

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace vrm10 {

enum class ErrorCode {
	InvalidType,
	MissingField,
	InvalidValue,
	UnknownEnumValue,
	UnsupportedVersion
};

struct Error {
	ErrorCode code{};
	std::string path;
	std::string message;
};

template<typename T>
class Result {
public:
	Result(T value): storage_(std::in_place_index<0>, std::move(value)) {}
	Result(Error error): storage_(std::in_place_index<1>, std::move(error)) {}

	bool hasValue() const noexcept {
		return std::holds_alternative<T>(storage_);
	}

	explicit operator bool() const noexcept {
		return hasValue();
	}

	T& value() & {
		if (!hasValue()) {
			throw std::logic_error("vrm10::Result has no value");
		}
		return std::get<T>(storage_);
	}

	const T& value() const& {
		if (!hasValue()) {
			throw std::logic_error("vrm10::Result has no value");
		}
		return std::get<T>(storage_);
	}

	T&& value() && {
		if (!hasValue()) {
			throw std::logic_error("vrm10::Result has no value");
		}
		return std::get<T>(std::move(storage_));
	}

	Error& error() & {
		if (hasValue()) {
			throw std::logic_error("vrm10::Result has no error");
		}
		return std::get<Error>(storage_);
	}

	const Error& error() const& {
		if (hasValue()) {
			throw std::logic_error("vrm10::Result has no error");
		}
		return std::get<Error>(storage_);
	}

private:
	std::variant<T, Error> storage_;
};

} // namespace vrm10
