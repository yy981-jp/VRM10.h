#pragma once

#include <vrm10/model.hpp>
#include <vrm10/parser/vrm.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vrm10 {

enum class ValidationSeverity : std::uint8_t {
	Warning,
	Error
};

struct ValidationIssue {
	ValidationSeverity severity{ValidationSeverity::Error};
	std::string code;
	std::string path;
	std::string message;
};

struct ValidationReport {
	std::vector<ValidationIssue> issues;

	bool hasErrors() const noexcept {
		for (const auto& issue : issues) {
			if (issue.severity == ValidationSeverity::Error) return true;
		}
		return false;
	}

	bool empty() const noexcept {
		return issues.empty();
	}
};

struct ValidationContext {
	std::size_t nodeCount{};
	std::size_t materialCount{};
	std::size_t imageCount{};
};

inline ValidationReport validate(const Vrm& vrm, const ValidationContext& context) {
	ValidationReport report;
	auto addNodeIssue = [&](std::string path, NodeIndex node) {
		if (node.value >= context.nodeCount) {
			report.issues.push_back({
				ValidationSeverity::Error,
				"node-index-out-of-range",
				std::move(path),
				"Node index " + std::to_string(node.value) + " is outside the glTF nodes array."
			});
		}
	};
	auto addMaterialIssue = [&](std::string path, MaterialIndex material) {
		if (material.value >= context.materialCount) {
			report.issues.push_back({
				ValidationSeverity::Error,
				"material-index-out-of-range",
				std::move(path),
				"Material index " + std::to_string(material.value) + " is outside the glTF materials array."
			});
		}
	};

	std::unordered_map<std::uint32_t, HumanBone> assignedBones;
	for (const auto& entry : detail::HumanBoneEntries) {
		const auto& node = vrm.humanoid[entry.bone];
		if (!node) continue;
		const auto path = detail::childPath(detail::childPath("/humanoid/humanBones", entry.name), "node");
		addNodeIssue(path, *node);
		const auto inserted = assignedBones.emplace(node->value, entry.bone);
		if (!inserted.second) {
			report.issues.push_back({
				ValidationSeverity::Error,
				"duplicate-humanoid-node",
				path,
				"A glTF node must not be assigned to more than one humanoid bone."
			});
		}
	}

	if (vrm.meta.thumbnailImage && vrm.meta.thumbnailImage.value() >= context.imageCount) {
		report.issues.push_back({
			ValidationSeverity::Error,
			"image-index-out-of-range",
			"/meta/thumbnailImage",
			"Thumbnail image index is outside the glTF images array."
		});
	}

	if (vrm.firstPerson) {
		for (std::size_t index = 0; index < vrm.firstPerson->meshAnnotations.size(); ++index) {
			addNodeIssue(detail::childPath(detail::childPath("/firstPerson/meshAnnotations", index), "node"), vrm.firstPerson->meshAnnotations[index].node);
		}
	}

	if (vrm.expressions) {
		auto validateExpressionMap = [&](const auto& expressions, const std::string& category) {
			for (const auto& namedExpression : expressions) {
				const auto expressionPath = detail::childPath(detail::childPath("/expressions", category), namedExpression.first);
				const auto& expression = namedExpression.second;
				for (std::size_t index = 0; index < expression.morphTargetBinds.size(); ++index) {
					addNodeIssue(detail::childPath(detail::childPath(detail::childPath(expressionPath, "morphTargetBinds"), index), "node"), expression.morphTargetBinds[index].node);
				}
				for (std::size_t index = 0; index < expression.materialColorBinds.size(); ++index) {
					addMaterialIssue(detail::childPath(detail::childPath(detail::childPath(expressionPath, "materialColorBinds"), index), "material"), expression.materialColorBinds[index].material);
				}
				for (std::size_t index = 0; index < expression.textureTransformBinds.size(); ++index) {
					addMaterialIssue(detail::childPath(detail::childPath(detail::childPath(expressionPath, "textureTransformBinds"), index), "material"), expression.textureTransformBinds[index].material);
				}
			}
		};
		validateExpressionMap(vrm.expressions->preset, "preset");
		validateExpressionMap(vrm.expressions->custom, "custom");
	}

	return report;
}

} // namespace vrm10
