#include <vrm10/vrm10.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

int failures = 0;

void fail(const std::string& message) {
	++failures;
	std::cerr << "FAILED: " << message << '\n';
}

bool requireObject(const nlohmann::json& value, const std::string& path) {
	if (!value.is_object()) {
		fail(path + " must be an object");
		return false;
	}
	return true;
}

bool reportIssues(const std::string& label, const vrm10::ValidationReport& report) {
	bool hasErrors = false;
	for (const auto& issue : report.issues) {
		const char* severity = issue.severity == vrm10::ValidationSeverity::Error
			? "error"
			: "warning";
		std::cerr << label << ": " << severity << " [" << issue.code << "] "
			<< issue.path << ": " << issue.message << '\n';
		if (issue.severity == vrm10::ValidationSeverity::Error) {
			hasErrors = true;
		}
	}
	if (hasErrors) {
		++failures;
	}
	return hasErrors;
}

bool reportParseError(const std::string& label, const vrm10::Error& error) {
	fail(label + " parse failed at " + error.path + ": " + error.message);
	return false;
}

std::optional<vrm10::GltfAlphaMode> parseAlphaMode(
	const nlohmann::json& material,
	const std::string& path
) {
	const auto it = material.find("alphaMode");
	const std::string value = it == material.end() ? "OPAQUE" : it->get<std::string>();
	if (value == "OPAQUE") return vrm10::GltfAlphaMode::Opaque;
	if (value == "MASK") return vrm10::GltfAlphaMode::Mask;
	if (value == "BLEND") return vrm10::GltfAlphaMode::Blend;
	fail(path + "/alphaMode has an unknown value: " + value);
	return std::nullopt;
}

bool buildValidationContext(
	const nlohmann::json& gltf,
	vrm10::ValidationContext& context
) {
	const auto nodesIt = gltf.find("nodes");
	if (nodesIt == gltf.end() || !nodesIt->is_array()) {
		fail("/nodes must be an array");
		return false;
	}
	const auto materialsIt = gltf.find("materials");
	const auto imagesIt = gltf.find("images");
	const auto texturesIt = gltf.find("textures");
	const auto meshesIt = gltf.find("meshes");

	const auto& nodes = *nodesIt;
	const std::size_t nodeCount = nodes.size();
	context.nodeCount = nodeCount;
	context.materialCount = materialsIt != gltf.end() && materialsIt->is_array()
		? materialsIt->size()
		: 0;
	context.imageCount = imagesIt != gltf.end() && imagesIt->is_array()
		? imagesIt->size()
		: 0;
	context.textureCount = texturesIt != gltf.end() && texturesIt->is_array()
		? texturesIt->size()
		: 0;
	context.nodeParents.resize(nodeCount);
	context.nodeMorphTargetCounts.resize(nodeCount);

	for (std::size_t parentIndex = 0; parentIndex < nodeCount; ++parentIndex) {
		const auto& node = nodes[parentIndex];
		if (!requireObject(node, "/nodes/" + std::to_string(parentIndex))) return false;

		const auto childrenIt = node.find("children");
		if (childrenIt != node.end()) {
			if (!childrenIt->is_array()) {
				fail("/nodes/" + std::to_string(parentIndex) + "/children must be an array");
				return false;
			}
			for (std::size_t childPosition = 0; childPosition < childrenIt->size(); ++childPosition) {
				const auto& child = (*childrenIt)[childPosition];
				if (!child.is_number_unsigned() && !child.is_number_integer()) {
					fail("/nodes/" + std::to_string(parentIndex) + "/children/" +
						std::to_string(childPosition) + " must be an integer");
					return false;
				}
				const auto childIndex = child.get<std::uint32_t>();
				if (childIndex >= nodeCount) {
					fail("/nodes/" + std::to_string(parentIndex) + "/children/" +
						std::to_string(childPosition) + " is outside /nodes");
					return false;
				}
				if (context.nodeParents[childIndex]) {
					fail("/nodes/" + std::to_string(childIndex) + " has multiple parents");
					return false;
				}
				context.nodeParents[childIndex] = vrm10::NodeIndex{
					static_cast<std::uint32_t>(parentIndex)
				};
			}
		}

		const auto meshIt = node.find("mesh");
		if (meshIt == node.end()) {
			context.nodeMorphTargetCounts[parentIndex] = std::nullopt;
			continue;
		}
		if (!meshIt->is_number_unsigned() && !meshIt->is_number_integer()) {
			fail("/nodes/" + std::to_string(parentIndex) + "/mesh must be an integer");
			return false;
		}
		if (meshesIt == gltf.end() || !meshesIt->is_array()) {
			fail("/nodes/" + std::to_string(parentIndex) + "/mesh references a missing meshes array");
			return false;
		}
		const auto meshIndex = meshIt->get<std::uint32_t>();
		if (meshIndex >= meshesIt->size()) {
			fail("/nodes/" + std::to_string(parentIndex) + "/mesh is outside /meshes");
			return false;
		}
		const auto& mesh = (*meshesIt)[meshIndex];
		const auto primitivesIt = mesh.find("primitives");
		if (primitivesIt == mesh.end() || !primitivesIt->is_array() || primitivesIt->empty()) {
			fail("/meshes/" + std::to_string(meshIndex) + "/primitives must be a non-empty array");
			return false;
		}

		std::optional<std::size_t> morphTargetCount;
		for (std::size_t primitiveIndex = 0; primitiveIndex < primitivesIt->size(); ++primitiveIndex) {
			const auto& primitive = (*primitivesIt)[primitiveIndex];
			const auto targetsIt = primitive.find("targets");
			const std::size_t count =
				targetsIt != primitive.end() && targetsIt->is_array()
					? targetsIt->size()
					: 0;
			if (!morphTargetCount) {
				morphTargetCount = count;
			} else if (*morphTargetCount != count) {
				fail("/meshes/" + std::to_string(meshIndex) +
					" has primitives with different morph target counts");
				return false;
			}
		}
		context.nodeMorphTargetCounts[parentIndex] = morphTargetCount;
	}
	return true;
}

bool parseMaterialExtensions(
	const nlohmann::json& gltf,
	const vrm10::ValidationContext& baseContext
) {
	const auto materialsIt = gltf.find("materials");
	if (materialsIt == gltf.end()) return true;
	if (!materialsIt->is_array()) {
		fail("/materials must be an array");
		return false;
	}

	std::size_t mtoonCount = 0;
	std::size_t legacyHdrCount = 0;
	std::size_t khrEmissiveCount = 0;

	for (std::size_t index = 0; index < materialsIt->size(); ++index) {
		const auto& material = (*materialsIt)[index];
		if (!requireObject(material, "/materials/" + std::to_string(index))) return false;
		const std::string path = "/materials/" + std::to_string(index);
		const auto extensionsIt = material.find("extensions");
		if (extensionsIt == material.end()) continue;
		if (!requireObject(*extensionsIt, path + "/extensions")) return false;

		const auto alphaMode = parseAlphaMode(material, path);
		if (!alphaMode) return false;
		vrm10::ValidationContext context = baseContext;
		context.materialAlphaMode = *alphaMode;
		context.materialUsesUnlit = extensionsIt->find("KHR_materials_unlit") != extensionsIt->end();

		const auto mtoonIt = extensionsIt->find("VRMC_materials_mtoon");
		if (mtoonIt != extensionsIt->end()) {
			auto parsed = vrm10::parseMaterialsMToon(*mtoonIt);
			if (!parsed) {
				reportParseError(path + "/extensions/VRMC_materials_mtoon", parsed.error());
			} else if (reportIssues(
				path + "/extensions/VRMC_materials_mtoon",
				vrm10::validate(parsed.value(), context)
			)) {
				// Keep checking later materials to collect all diagnostics.
			}
			++mtoonCount;
		}

		const auto legacyHdrIt = extensionsIt->find("VRMC_materials_hdr_emissiveMultiplier");
		if (legacyHdrIt != extensionsIt->end()) {
			auto parsed = vrm10::parseHdrEmissiveMultiplier(*legacyHdrIt);
			if (!parsed) reportParseError(path + "/extensions/VRMC_materials_hdr_emissiveMultiplier", parsed.error());
			++legacyHdrCount;
		}

		const auto emissiveIt = extensionsIt->find("KHR_materials_emissive_strength");
		if (emissiveIt != extensionsIt->end()) {
			auto parsed = vrm10::parseMaterialsEmissiveStrength(*emissiveIt);
			if (!parsed) {
				reportParseError(path + "/extensions/KHR_materials_emissive_strength", parsed.error());
			} else {
				reportIssues(
					path + "/extensions/KHR_materials_emissive_strength",
					vrm10::validate(parsed.value(), context)
				);
			}
			++khrEmissiveCount;
		}
	}

	std::cout << "Materials: " << materialsIt->size()
		<< ", MToon: " << mtoonCount
		<< ", legacy HDR emissive: " << legacyHdrCount
		<< ", KHR emissive strength: " << khrEmissiveCount << '\n';
	return true;
}

bool parseNodeConstraints(
	const nlohmann::json& gltf,
	std::size_t nodeCount
) {
	const auto& nodes = gltf.at("nodes");
	std::vector<std::optional<vrm10::NodeConstraint>> constraints(nodeCount);
	std::size_t count = 0;

	for (std::size_t index = 0; index < nodeCount; ++index) {
		const auto extensionsIt = nodes[index].find("extensions");
		if (extensionsIt == nodes[index].end()) continue;
		if (!requireObject(*extensionsIt, "/nodes/" + std::to_string(index) + "/extensions")) return false;
		const auto constraintIt = extensionsIt->find("VRMC_node_constraint");
		if (constraintIt == extensionsIt->end()) continue;

		auto parsed = vrm10::parseNodeConstraint(*constraintIt);
		if (!parsed) {
			reportParseError(
				"/nodes/" + std::to_string(index) + "/extensions/VRMC_node_constraint",
				parsed.error()
			);
		} else {
			constraints[index] = std::move(parsed).value();
		}
		++count;
	}

	const auto report = vrm10::validate(constraints, nodeCount);
	reportIssues("VRMC_node_constraint", report);
	std::cout << "Node constraints: " << count << '\n';
	return true;
}

} // namespace

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Usage: vrm10_asset_tests <extracted-glb-json>\n";
		return EXIT_FAILURE;
	}

	std::ifstream input(argv[1], std::ios::binary);
	if (!input) {
		std::cerr << "Could not open extracted JSON: " << argv[1] << '\n';
		return EXIT_FAILURE;
	}

	nlohmann::json gltf;
	try {
		input >> gltf;
	} catch (const std::exception& error) {
		std::cerr << "Could not parse glTF JSON: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
	if (!requireObject(gltf, "/")) return EXIT_FAILURE;

	const auto assetIt = gltf.find("asset");
	if (assetIt == gltf.end() || !assetIt->is_object() ||
		assetIt->value("version", std::string{}) != "2.0") {
		fail("The real asset must contain a glTF 2.0 asset header");
		return EXIT_FAILURE;
	}

	const auto nodesIt = gltf.find("nodes");
	if (nodesIt == gltf.end() || !nodesIt->is_array() || nodesIt->empty()) {
		fail("The real asset must contain a non-empty nodes array");
		return EXIT_FAILURE;
	}

	vrm10::ValidationContext context;
	if (!buildValidationContext(gltf, context)) return EXIT_FAILURE;

	const auto extensionsIt = gltf.find("extensions");
	if (extensionsIt == gltf.end() || !requireObject(*extensionsIt, "/extensions")) {
		fail("The real asset must contain root extensions");
		return EXIT_FAILURE;
	}

	const auto vrmIt = extensionsIt->find("VRMC_vrm");
	if (vrmIt == extensionsIt->end()) {
		fail("The real asset must contain extensions.VRMC_vrm");
		return EXIT_FAILURE;
	}
	auto vrm = vrm10::parseVrm(*vrmIt);
	if (!vrm) {
		reportParseError("extensions.VRMC_vrm", vrm.error());
	} else {
		reportIssues("VRMC_vrm", vrm10::validate(vrm.value(), context));
	}

	std::size_t springBoneCount = 0;
	const auto springBoneIt = extensionsIt->find("VRMC_springBone");
	if (springBoneIt != extensionsIt->end()) {
		auto springBone = vrm10::parseSpringBone(*springBoneIt);
		if (!springBone) {
			reportParseError("extensions.VRMC_springBone", springBone.error());
		} else {
			reportIssues("VRMC_springBone", vrm10::validate(springBone.value(), context));
			springBoneCount = springBone.value().springs.size();
		}
	}

	parseNodeConstraints(gltf, context.nodeCount);
	parseMaterialExtensions(gltf, context);

	std::cout << "Nodes: " << context.nodeCount
		<< ", springs: " << springBoneCount
		<< ", materials: " << context.materialCount
		<< ", textures: " << context.textureCount
		<< ", images: " << context.imageCount << '\n';

	if (failures != 0) {
		std::cerr << failures << " real-asset check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Real VRM asset parsed and semantically validated.\n";
	return EXIT_SUCCESS;
}
