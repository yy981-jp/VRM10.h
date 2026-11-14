#pragma once

#include <vrm10/model.hpp>
#include <vrm10/parser/materials_mtoon.hpp>
#include <vrm10/parser/node_constraint.hpp>
#include <vrm10/parser/spring_bone.hpp>
#include <vrm10/parser/vrm.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
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
	// Alpha mode of the glTF material being validated alongside MToon.
	GltfAlphaMode materialAlphaMode{GltfAlphaMode::Opaque};
	// KHR_materials_emissive_strength must not be used with KHR_materials_unlit.
	bool materialUsesUnlit{false};
	// One entry per node when provided. nullopt means the node has no mesh.
	std::vector<std::optional<std::size_t>> nodeMorphTargetCounts;
};


namespace detail {

struct HumanBoneHierarchyEntry {
	HumanBone bone;
	HumanBone parent;
	bool hasParent;
	bool parentRequired;
};

inline constexpr HumanBoneHierarchyEntry HumanBoneHierarchy[] = {
	{HumanBone::Hips, HumanBone::Hips, false, false},
	{HumanBone::Spine, HumanBone::Hips, true, true},
	{HumanBone::Chest, HumanBone::Spine, true, true},
	{HumanBone::UpperChest, HumanBone::Chest, true, true},
	{HumanBone::Neck, HumanBone::UpperChest, true, false},
	{HumanBone::Head, HumanBone::Neck, true, false},
	{HumanBone::LeftEye, HumanBone::Head, true, true},
	{HumanBone::RightEye, HumanBone::Head, true, true},
	{HumanBone::Jaw, HumanBone::Head, true, true},
	{HumanBone::LeftUpperLeg, HumanBone::Hips, true, true},
	{HumanBone::LeftLowerLeg, HumanBone::LeftUpperLeg, true, true},
	{HumanBone::LeftFoot, HumanBone::LeftLowerLeg, true, true},
	{HumanBone::LeftToes, HumanBone::LeftFoot, true, true},
	{HumanBone::RightUpperLeg, HumanBone::Hips, true, true},
	{HumanBone::RightLowerLeg, HumanBone::RightUpperLeg, true, true},
	{HumanBone::RightFoot, HumanBone::RightLowerLeg, true, true},
	{HumanBone::RightToes, HumanBone::RightFoot, true, true},
	{HumanBone::LeftShoulder, HumanBone::UpperChest, true, false},
	{HumanBone::LeftUpperArm, HumanBone::LeftShoulder, true, false},
	{HumanBone::LeftLowerArm, HumanBone::LeftUpperArm, true, true},
	{HumanBone::LeftHand, HumanBone::LeftLowerArm, true, true},
	{HumanBone::RightShoulder, HumanBone::UpperChest, true, false},
	{HumanBone::RightUpperArm, HumanBone::RightShoulder, true, false},
	{HumanBone::RightLowerArm, HumanBone::RightUpperArm, true, true},
	{HumanBone::RightHand, HumanBone::RightLowerArm, true, true},
	{HumanBone::LeftThumbMetacarpal, HumanBone::LeftHand, true, true},
	{HumanBone::LeftThumbProximal, HumanBone::LeftThumbMetacarpal, true, true},
	{HumanBone::LeftThumbDistal, HumanBone::LeftThumbProximal, true, true},
	{HumanBone::LeftIndexProximal, HumanBone::LeftHand, true, true},
	{HumanBone::LeftIndexIntermediate, HumanBone::LeftIndexProximal, true, true},
	{HumanBone::LeftIndexDistal, HumanBone::LeftIndexIntermediate, true, true},
	{HumanBone::LeftMiddleProximal, HumanBone::LeftHand, true, true},
	{HumanBone::LeftMiddleIntermediate, HumanBone::LeftMiddleProximal, true, true},
	{HumanBone::LeftMiddleDistal, HumanBone::LeftMiddleIntermediate, true, true},
	{HumanBone::LeftRingProximal, HumanBone::LeftHand, true, true},
	{HumanBone::LeftRingIntermediate, HumanBone::LeftRingProximal, true, true},
	{HumanBone::LeftRingDistal, HumanBone::LeftRingIntermediate, true, true},
	{HumanBone::LeftLittleProximal, HumanBone::LeftHand, true, true},
	{HumanBone::LeftLittleIntermediate, HumanBone::LeftLittleProximal, true, true},
	{HumanBone::LeftLittleDistal, HumanBone::LeftLittleIntermediate, true, true},
	{HumanBone::RightThumbMetacarpal, HumanBone::RightHand, true, true},
	{HumanBone::RightThumbProximal, HumanBone::RightThumbMetacarpal, true, true},
	{HumanBone::RightThumbDistal, HumanBone::RightThumbProximal, true, true},
	{HumanBone::RightIndexProximal, HumanBone::RightHand, true, true},
	{HumanBone::RightIndexIntermediate, HumanBone::RightIndexProximal, true, true},
	{HumanBone::RightIndexDistal, HumanBone::RightIndexIntermediate, true, true},
	{HumanBone::RightMiddleProximal, HumanBone::RightHand, true, true},
	{HumanBone::RightMiddleIntermediate, HumanBone::RightMiddleProximal, true, true},
	{HumanBone::RightMiddleDistal, HumanBone::RightMiddleIntermediate, true, true},
	{HumanBone::RightRingProximal, HumanBone::RightHand, true, true},
	{HumanBone::RightRingIntermediate, HumanBone::RightRingProximal, true, true},
	{HumanBone::RightRingDistal, HumanBone::RightRingIntermediate, true, true},
	{HumanBone::RightLittleProximal, HumanBone::RightHand, true, true},
	{HumanBone::RightLittleIntermediate, HumanBone::RightLittleProximal, true, true},
	{HumanBone::RightLittleDistal, HumanBone::RightLittleIntermediate, true, true}
};

inline const HumanBoneHierarchyEntry* findHumanBoneHierarchy(HumanBone bone) noexcept {
	for (const auto& entry : HumanBoneHierarchy) {
		if (entry.bone == bone) return &entry;
	}
	return nullptr;
}

inline std::string_view humanBoneName(HumanBone bone) noexcept {
	for (const auto& entry : HumanBoneEntries) {
		if (entry.bone == bone) return entry.name;
	}
	return "unknown";
}

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
	const bool hasNodeHierarchy = context.nodeParents.size() == context.nodeCount;
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

		const auto* hierarchy = detail::findHumanBoneHierarchy(entry.bone);
		if (!hierarchy || !hierarchy->hasParent) continue;

		HumanBone parentBone = hierarchy->parent;
		if (!vrm.humanoid[parentBone]) {
			if (hierarchy->parentRequired) {
				report.issues.push_back({
					ValidationSeverity::Error,
					"missing-humanoid-parent-bone",
					path,
					"Bone '" + std::string(entry.name) + "' requires parent bone '" +
						std::string(detail::humanBoneName(parentBone)) + "'."
				});
				continue;
			}

			bool foundAssignedAncestor = false;
			while (true) {
				if (vrm.humanoid[parentBone]) {
					foundAssignedAncestor = true;
					break;
				}
				const auto* parentHierarchy = detail::findHumanBoneHierarchy(parentBone);
				if (!parentHierarchy || !parentHierarchy->hasParent) break;
				parentBone = parentHierarchy->parent;
			}
			if (!foundAssignedAncestor) continue;
		}

		if (hasNodeHierarchy && node->value < context.nodeCount) {
			const auto& parentNode = vrm.humanoid[parentBone];
			if (parentNode && parentNode->value < context.nodeCount &&
				!detail::isAncestorOrSelf(*parentNode, *node, context)) {
				report.issues.push_back({
					ValidationSeverity::Error,
					"humanoid-bone-not-descendant-of-parent",
					path,
					"Bone '" + std::string(entry.name) + "' must be a descendant of its assigned parent bone."
				});
			}
		}
	}

	if (vrm.meta.thumbnailImage && vrm.meta.thumbnailImage->value >= context.imageCount) {
		report.issues.push_back({
			ValidationSeverity::Error,
			"image-index-out-of-range",
			"/meta/thumbnailImage",
			"Thumbnail image index is outside the glTF images array."
		});
	}

	if (vrm.firstPerson) {
		const bool hasMorphTargetCounts = context.nodeMorphTargetCounts.size() == context.nodeCount;
		for (std::size_t index = 0; index < vrm.firstPerson->meshAnnotations.size(); ++index) {
			const auto path = detail::childPath(detail::childPath("/firstPerson/meshAnnotations", index), "node");
			const NodeIndex node = vrm.firstPerson->meshAnnotations[index].node;
			addNodeIssue(path, node);
			if (hasMorphTargetCounts && node.value < context.nodeCount && !context.nodeMorphTargetCounts[node.value]) {
				report.issues.push_back({
					ValidationSeverity::Error,
					"first-person-node-has-no-mesh",
					path,
					"First-person mesh annotations must reference a node that has a mesh."
				});
			}
		}
	}

	if (vrm.expressions) {
		auto validateExpressionMap = [&](const auto& expressions, const std::string& category) {
			for (const auto& namedExpression : expressions) {
				const auto expressionPath = detail::childPath(detail::childPath("/expressions", category), namedExpression.first);
				const auto& expression = namedExpression.second;
				for (std::size_t index = 0; index < expression.morphTargetBinds.size(); ++index) {
					const auto bindPath = detail::childPath(detail::childPath(expressionPath, "morphTargetBinds"), index);
					const auto nodePath = detail::childPath(bindPath, "node");
					const auto indexPath = detail::childPath(bindPath, "index");
					const auto& bind = expression.morphTargetBinds[index];
					addNodeIssue(nodePath, bind.node);

					if (context.nodeMorphTargetCounts.size() == context.nodeCount && bind.node.value < context.nodeCount) {
						const auto& targetCount = context.nodeMorphTargetCounts[bind.node.value];
						if (!targetCount) {
							report.issues.push_back({
								ValidationSeverity::Error,
								"morph-target-node-has-no-mesh",
								nodePath,
								"Morph target binds must reference a node that has a mesh."
							});
						} else if (bind.index.value >= *targetCount) {
							report.issues.push_back({
								ValidationSeverity::Error,
								"morph-target-index-out-of-range",
								indexPath,
								"Morph target index " + std::to_string(bind.index.value) +
									" is outside the target mesh morph target array."
							});
						}
					}
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

	const bool hasNodeHierarchy = context.nodeParents.size() == context.nodeCount;
	std::vector<std::unordered_map<std::uint32_t, std::string>> chainNodePaths(springBone.springs.size());

	// A SpringChain also owns nodes skipped between two explicitly listed joints.
	// These intermediate nodes participate in the spec's cross-chain overlap rule.
	for (std::size_t springIndex = 0; springIndex < springBone.springs.size(); ++springIndex) {
		const auto& spring = springBone.springs[springIndex];
		const auto jointsPath = detail::childPath(detail::childPath("/springs", springIndex), "joints");
		auto& members = chainNodePaths[springIndex];

		for (std::size_t jointIndex = 0; jointIndex < spring.joints.size(); ++jointIndex) {
			const NodeIndex node = spring.joints[jointIndex].node;
			const std::string nodePath = detail::childPath(detail::childPath(jointsPath, jointIndex), "node");
			members.emplace(node.value, nodePath);

			if (!hasNodeHierarchy || jointIndex == 0) continue;
			const NodeIndex previous = spring.joints[jointIndex - 1].node;
			if (previous.value >= context.nodeCount || node.value >= context.nodeCount ||
				!detail::isStrictAncestor(previous, node, context)) {
				continue;
			}

			std::uint32_t current = node.value;
			while (current != previous.value) {
				members.emplace(current, nodePath);
				const auto& parent = context.nodeParents[current];
				if (!parent || parent->value >= context.nodeCount) break;
				current = parent->value;
			}
			members.emplace(previous.value, detail::childPath(detail::childPath(jointsPath, jointIndex - 1), "node"));
		}
	}

	std::unordered_map<std::uint32_t, std::pair<std::size_t, std::string>> memberOwners;
	for (std::size_t springIndex = 0; springIndex < chainNodePaths.size(); ++springIndex) {
		for (const auto& member : chainNodePaths[springIndex]) {
			const auto inserted = memberOwners.emplace(
				member.first,
				std::make_pair(springIndex, member.second)
			);
			if (!inserted.second && inserted.first->second.first != springIndex) {
				report.issues.push_back({
					ValidationSeverity::Error,
					"duplicate-spring-joint",
					member.second,
					"A node, including a skipped intermediate node, must not belong to more than one SpringChain."
				});
			}
		}
	}

	for (std::size_t springIndex = 0; springIndex < springBone.springs.size(); ++springIndex) {
		const auto& spring = springBone.springs[springIndex];
		const auto springPath = detail::childPath("/springs", springIndex);
		const auto jointsPath = detail::childPath(springPath, "joints");
		for (std::size_t jointIndex = 0; jointIndex < spring.joints.size(); ++jointIndex) {
			const auto jointPath = detail::childPath(jointsPath, jointIndex);
			const auto nodePath = detail::childPath(jointPath, "node");
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
			for (std::size_t otherSpringIndex = 0; otherSpringIndex < chainNodePaths.size(); ++otherSpringIndex) {
				if (otherSpringIndex == springIndex) continue;
				for (const auto& member : chainNodePaths[otherSpringIndex]) {
					const NodeIndex otherNode{member.first};
					const bool sameNode = center == otherNode;
					const bool descendantOfOtherChainNode =
						hasNodeHierarchy && otherNode.value < context.nodeCount &&
						center.value < context.nodeCount &&
						detail::isAncestorOrSelf(otherNode, center, context);
					if (sameNode || descendantOfOtherChainNode) {
						report.issues.push_back({
							ValidationSeverity::Error,
							"spring-center-overlaps-other-chain",
							centerPath,
							"The center node must not be a member of another SpringChain or a descendant of one of its nodes."
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

	const bool validRenderQueueOffset =
		context.materialAlphaMode == GltfAlphaMode::Blend
			? (mtoon.transparentWithZWrite
				? mtoon.renderQueueOffsetNumber >= 0
				: mtoon.renderQueueOffsetNumber <= 0)
			: mtoon.renderQueueOffsetNumber == 0;
	if (!validRenderQueueOffset) {
		report.issues.push_back({
			ValidationSeverity::Error,
			"mtoon-render-queue-offset-invalid-for-alpha-mode",
			"/renderQueueOffsetNumber",
			"Render queue offset must be zero for OPAQUE/MASK, non-negative for BLEND with transparentWithZWrite, and non-positive for BLEND without it."
		});
	}
	return report;
}

inline ValidationReport validate(
	const MaterialsEmissiveStrength& emissiveStrength,
	const ValidationContext& context
) {
	ValidationReport report;
	if (context.materialUsesUnlit) {
		report.issues.push_back({
			ValidationSeverity::Error,
			"emissive-strength-with-unlit-material",
			"/extensions/KHR_materials_emissive_strength",
			"KHR_materials_emissive_strength must not be used on a material that also uses KHR_materials_unlit."
		});
	}
	return report;
}

inline ValidationReport validate(
	const std::vector<std::optional<NodeConstraint>>& constraints,
	std::size_t nodeCount
) {
	ValidationReport report;
	if (constraints.size() != nodeCount) {
		report.issues.push_back({
			ValidationSeverity::Error,
			"constraint-array-size-mismatch",
			"/nodes",
			"The node-indexed constraint array must contain exactly one entry per glTF node."
		});
	}
	const std::size_t availableNodes = std::min(constraints.size(), nodeCount);
	auto sourceOf = [](const NodeConstraint& constraint) -> NodeIndex {
		return std::visit([](const auto& value) { return value.source; }, constraint.constraint);
	};

	for (std::size_t destination = 0; destination < availableNodes; ++destination) {
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

	std::vector<std::uint8_t> state(availableNodes, 0);
	for (std::size_t start = 0; start < availableNodes; ++start) {
		if (!constraints[start] || state[start] != 0) continue;
		std::vector<std::size_t> path;
		std::size_t current = start;
		while (current < availableNodes && constraints[current] && state[current] == 0) {
			state[current] = 1;
			path.push_back(current);
			const NodeIndex source = sourceOf(*constraints[current]);
			if (source.value >= nodeCount || source.value >= availableNodes) {
				current = availableNodes;
				break;
			}
			current = source.value;
			if (!constraints[current]) break;
		}
		if (current < availableNodes && constraints[current] && state[current] == 1) {
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
