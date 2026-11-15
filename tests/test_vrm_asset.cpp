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

bool parseMaterialExtensions(const nlohmann::json& gltf) {
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

		const auto mtoonIt = extensionsIt->find("VRMC_materials_mtoon");
		if (mtoonIt != extensionsIt->end()) {
			auto parsed = vrm10::parseMaterialsMToon(*mtoonIt);
			if (!parsed) {
				reportParseError(path + "/extensions/VRMC_materials_mtoon", parsed.error());
			} else if (reportIssues(
				path + "/extensions/VRMC_materials_mtoon",
				vrm10::validate(parsed.value(), gltf, index)
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
					vrm10::validate(parsed.value(), gltf, index)
				);
			}
			++khrEmissiveCount;
		}
	}

	std::cout << "Materials: " << materialsIt->size()
		<< ", MToon: " << mtoonCount
		<< ", legacy HDR emissive: " << legacyHdrCount
		<< ", KHR emissive strength: " << khrEmissiveCount << '\n';
	if (mtoonCount == 0) {
		fail("The real VRM regression asset is expected to exercise VRMC_materials_mtoon");
	}
	return true;
}

bool parseNodeConstraints(const nlohmann::json& gltf) {
	const auto& nodes = gltf.at("nodes");
	std::vector<std::optional<vrm10::NodeConstraint>> constraints(nodes.size());
	std::size_t count = 0;

	for (std::size_t index = 0; index < nodes.size(); ++index) {
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

	const auto report = vrm10::validate(constraints, gltf);
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
		reportIssues("VRMC_vrm", vrm10::validate(vrm.value(), gltf));
	}

	std::size_t springBoneCount = 0;
	std::size_t extendedColliderCount = 0;
	const auto springBoneIt = extensionsIt->find("VRMC_springBone");
	if (springBoneIt != extensionsIt->end()) {
		auto springBone = vrm10::parseSpringBone(*springBoneIt);
		if (!springBone) {
			reportParseError("extensions.VRMC_springBone", springBone.error());
		} else {
			reportIssues("VRMC_springBone", vrm10::validate(springBone.value(), gltf));
			springBoneCount = springBone.value().springs.size();
			for (const auto& collider : springBone.value().colliders) {
				if (collider.extended) ++extendedColliderCount;
			}
		}
	}

	bool usesExtendedCollider = false;
	const auto extensionsUsedIt = gltf.find("extensionsUsed");
	if (extensionsUsedIt != gltf.end() && extensionsUsedIt->is_array()) {
		for (const auto& extension : *extensionsUsedIt) {
			if (extension.is_string() && extension.get<std::string>() == "VRMC_springBone_extended_collider") {
				usesExtendedCollider = true;
				break;
			}
		}
	}
	if (usesExtendedCollider && extendedColliderCount == 0) {
		fail("The asset declares VRMC_springBone_extended_collider but no collider extension was parsed");
	}
	if (springBoneCount == 0) {
		fail("The real VRM regression asset is expected to contain SpringBone springs");
	}

	parseNodeConstraints(gltf);
	parseMaterialExtensions(gltf);

	std::cout << "Nodes: " << nodesIt->size()
		<< ", springs: " << springBoneCount
		<< ", extended colliders: " << extendedColliderCount
		<< ", materials: " << (gltf.contains("materials") && gltf["materials"].is_array() ? gltf["materials"].size() : 0)
		<< ", textures: " << (gltf.contains("textures") && gltf["textures"].is_array() ? gltf["textures"].size() : 0)
		<< ", images: " << (gltf.contains("images") && gltf["images"].is_array() ? gltf["images"].size() : 0) << '\n';

	if (failures != 0) {
		std::cerr << failures << " real-asset check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Real VRM asset parsed and semantically validated.\n";
	return EXIT_SUCCESS;
}
