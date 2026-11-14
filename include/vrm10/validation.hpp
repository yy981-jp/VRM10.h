#pragma once

#include <vrm10/model.hpp>
#include <vrm10/parser/materials_mtoon.hpp>
#include <vrm10/parser/node_constraint.hpp>
#include <vrm10/parser/spring_bone.hpp>
#include <vrm10/parser/vrm.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
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
	std::vector<std::optional<NodeIndex>> nodeParents;
	std::size_t textureCount{};
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


namespace detail {

inline bool isAncestorOrSelf(NodeIndex ancestor, NodeIndex descendant, const ValidationContext& context) {
	if (ancestor == descendant) return true;
	if (context.nodeParents.size() != context.nodeCount) return false;
	std::uint32_t current = descendant.value;
	for (std::size_t step = 0; step < context.nodeCount; ++step) {
		if (current >= context.nodeParents.size()) return false;
		const auto& parent = context.nodeParents[current];
		if (!parent || parent->value >= context.nodeCount) return false;
		if (*parent == ancestor) return true;
		current = parent->value;
	}
	return false;
}

inline bool isStrictAncestor(NodeIndex ancestor, NodeIndex descendant, const ValidationContext& context) {
	return ancestor != descendant && isAncestorOrSelf(ancestor, descendant, context);
}

} // namespace detail

inline ValidationReport validate(const SpringBone& springBone, const ValidationContext& context) {
	ValidationReport report;
	auto addNodeIssue = [&](const std::string& path, NodeIndex node) {
		if (node.value >= context.nodeCount) {
			report.issues.push_back({
				ValidationSeverity::Error,
				"node-index-out-of-range",
				path,
				"Node index " + std::to_string(node.value) + " is outside the glTF nodes array."
			});
		}
	};
	auto addIndexIssue = [&](const std::string& path, std::uint32_t index, std::size_t count, const char* code, const char* target) {
		if (index >= count) {
			report.issues.push_back({
				ValidationSeverity::Error,
				code,
				path,
				std::string("Index ") + std::to_string(index) + " is outside the " + target + " array."
			});
		}
	};

	for (std::size_t index = 0; index < springBone.colliders.size(); ++index) {
		addNodeIssue(detail::childPath(detail::childPath("/colliders", index), "node"), springBone.colliders[index].node);
	}

	for (std::size_t groupIndex = 0; groupIndex < springBone.colliderGroups.size(); ++groupIndex) {
		const auto& group = springBone.colliderGroups[groupIndex];
		for (std::size_t colliderIndex = 0; colliderIndex < group.colliders.size(); ++colliderIndex) {
			addIndexIssue(
				detail::childPath(detail::childPath(detail::childPath("/colliderGroups", groupIndex), "colliders"), colliderIndex),
				group.colliders[colliderIndex].value,
				springBone.colliders.size(),
				"collider-index-out-of-range",
				"colliders"
			);
		}
	}

	std::unordered_map<std::uint32_t, std::string> assignedJointNodes;
	const bool hasNodeHierarchy = context.nodeParents.size() == context.nodeCount;
	for (std::size_t springIndex = 0; springIndex < springBone.springs.size(); ++springIndex) {
		const auto& spring = springBone.springs[springIndex];
		const auto springPath = detail::childPath("/springs", springIndex);
		const auto jointsPath = detail::childPath(springPath, "joints");
		for (std::size_t jointIndex = 0; jointIndex < spring.joints.size(); ++jointIndex) {
			const auto nodePath = detail::childPath(detail::childPath(jointsPath, jointIndex), "node");
			const NodeIndex node = spring.joints[jointIndex].node;
			addNodeIssue(nodePath, node);
			if (jointIndex + 1 == spring.joints.size() && spring.joints[jointIndex].limit) {
				report.issues.push_back({
					ValidationSeverity::Warning,
					"spring-limit-on-tail-ignored",
					detail::childPath(detail::childPath(jointPath, "extensions"), "VRMC_springBone_limit"),
					"The limit extension on the final tail joint must be ignored."
				});
			}
			if (!assignedJointNodes.emplace(node.value, nodePath).second) {
				report.issues.push_back({
					ValidationSeverity::Error,
					"duplicate-spring-joint",
					nodePath,
					"A node must not be assigned to more than one SpringBone joint."
				});
			}
			if (hasNodeHierarchy && jointIndex > 0) {
				const NodeIndex previous = spring.joints[jointIndex - 1].node;
				if (previous.value < context.nodeCount && node.value < context.nodeCount &&
					!detail::isStrictAncestor(previous, node, context)) {
					report.issues.push_back({
						ValidationSeverity::Error,
						"spring-joint-not-descendant",
						nodePath,
						"Each joint after the first must be a descendant of the preceding joint."
					});
				}
			}
		}

		for (std::size_t groupIndex = 0; groupIndex < spring.colliderGroups.size(); ++groupIndex) {
			const auto path = detail::childPath(detail::childPath(springPath, "colliderGroups"), groupIndex);
			addIndexIssue(
				path,
				spring.colliderGroups[groupIndex].value,
				springBone.colliderGroups.size(),
				"collider-group-index-out-of-range",
				"colliderGroups"
			);
		}

		if (spring.center) {
			const NodeIndex center = *spring.center;
			const auto centerPath = detail::childPath(springPath, "center");
			addNodeIssue(centerPath, center);
			if (center.value < context.nodeCount && !spring.joints.empty() &&
				spring.joints.front().node.value < context.nodeCount && hasNodeHierarchy &&
				!detail::isAncestorOrSelf(center, spring.joints.front().node, context)) {
				report.issues.push_back({
					ValidationSeverity::Error,
					"spring-center-not-ancestor",
					centerPath,
					"The center node must be the first joint or one of its ancestors."
				});
			}
			for (std::size_t otherSpringIndex = 0; otherSpringIndex < springBone.springs.size(); ++otherSpringIndex) {
				if (otherSpringIndex == springIndex) continue;
				for (const auto& otherJoint : springBone.springs[otherSpringIndex].joints) {
					const NodeIndex otherNode = otherJoint.node;
					const bool sameNode = center == otherNode;
					const bool descendantOfOtherJoint =
						hasNodeHierarchy && otherNode.value < context.nodeCount &&
						center.value < context.nodeCount && detail::isAncestorOrSelf(otherNode, center, context);
					if (sameNode || descendantOfOtherJoint) {
						report.issues.push_back({
							ValidationSeverity::Error,
							"spring-center-overlaps-other-chain",
							centerPath,
							"The center node must not be a joint of another SpringChain or its descendant."
						});
						break;
					}
				}
			}
		}
	}
	return report;
}


inline ValidationReport validate(const MaterialsMToon& mtoon, const ValidationContext& context) {
	ValidationReport report;
	auto checkTexture = [&](const std::string& path, const std::optional<TextureInfo>& texture) {
		if (texture && texture->index.value >= context.textureCount) {
			report.issues.push_back({
				ValidationSeverity::Error,
				"texture-index-out-of-range",
				path,
				"Texture index " + std::to_string(texture->index.value) + " is outside the glTF textures array."
			});
		}
	};
	checkTexture("/shadeMultiplyTexture", mtoon.shadeMultiplyTexture);
	checkTexture("/matcapTexture", mtoon.matcapTexture);
	checkTexture("/rimMultiplyTexture", mtoon.rimMultiplyTexture);
	checkTexture("/outlineWidthMultiplyTexture", mtoon.outlineWidthMultiplyTexture);
	checkTexture("/uvAnimationMaskTexture", mtoon.uvAnimationMaskTexture);
	if (mtoon.shadingShiftTexture) {
		checkTexture("/shadingShiftTexture", mtoon.shadingShiftTexture->texture);
	}
	return report;
}

inline ValidationReport validate(
	const std::vector<std::optional<NodeConstraint>>& constraints,
	std::size_t nodeCount
) {
	ValidationReport report;
	auto sourceOf = [](const NodeConstraint& constraint) -> NodeIndex {
		return std::visit([](const auto& value) { return value.source; }, constraint.constraint);
	};

	for (std::size_t destination = 0; destination < constraints.size(); ++destination) {
		if (!constraints[destination]) continue;
		const NodeIndex source = sourceOf(*constraints[destination]);
		const std::string path = detail::childPath(
			detail::childPath(detail::childPath("/nodes", destination), "extensions"),
			"VRMC_node_constraint"
		) + "/constraint";
		if (source.value >= nodeCount) {
			report.issues.push_back({
				ValidationSeverity::Error,
				"constraint-source-index-out-of-range",
				path,
				"Constraint source node index is outside the glTF nodes array."
			});
		}
		if (source.value == destination) {
			report.issues.push_back({
				ValidationSeverity::Error,
				"constraint-source-is-destination",
				path,
				"A constraint source must not be the destination node itself."
			});
		}
	}

	std::vector<std::uint8_t> state(constraints.size(), 0);
	for (std::size_t start = 0; start < constraints.size(); ++start) {
		if (!constraints[start] || state[start] != 0) continue;
		std::vector<std::size_t> path;
		std::size_t current = start;
		while (current < constraints.size() && constraints[current] && state[current] == 0) {
			state[current] = 1;
			path.push_back(current);
			const NodeIndex source = sourceOf(*constraints[current]);
			if (source.value >= constraints.size()) {
				current = constraints.size();
				break;
			}
			current = source.value;
			if (!constraints[current]) break;
		}
		if (current < constraints.size() && constraints[current] && state[current] == 1) {
			report.issues.push_back({
				ValidationSeverity::Error,
				"constraint-circular-dependency",
				detail::childPath(
					detail::childPath(detail::childPath("/nodes", current), "extensions"),
					"VRMC_node_constraint"
				) + "/constraint",
				"Node constraints must not form a circular dependency."
			});
		}
		for (const std::size_t node : path) state[node] = 2;
	}
	return report;
}

} // namespace vrm10
