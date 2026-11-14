#include <vrm10/vrm10.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
	if (!condition) {
		++failures;
		std::cerr << "FAILED: " << message << '\n';
	}
}

nlohmann::json validVrmJson() {
	return nlohmann::json::parse(R"json(
	{
		"specVersion": "1.0",
		"meta": {
			"name": "Minimal avatar",
			"authors": ["Test author"],
			"licenseUrl": "https://vrm.dev/licenses/1.0/"
		},
		"humanoid": {
			"humanBones": {
				"hips": {"node": 0},
				"spine": {"node": 1},
				"head": {"node": 2},
				"leftUpperLeg": {"node": 3},
				"leftLowerLeg": {"node": 4},
				"leftFoot": {"node": 5},
				"rightUpperLeg": {"node": 6},
				"rightLowerLeg": {"node": 7},
				"rightFoot": {"node": 8},
				"leftUpperArm": {"node": 9},
				"leftLowerArm": {"node": 10},
				"leftHand": {"node": 11},
				"rightUpperArm": {"node": 12},
				"rightLowerArm": {"node": 13},
				"rightHand": {"node": 14}
			}
		}
	}
	)json");
}

void testRequiredDataAndNodeZero() {
	auto result = vrm10::parseVrm(validVrmJson());
	check(result.hasValue(), "minimal valid VRM should parse");
	if (!result) return;
	check(result.value().specVersion == "1.0", "specVersion should be preserved");
	check(result.value().humanoid[vrm10::HumanBone::Hips].has_value(), "hips should be assigned");
	check(result.value().humanoid[vrm10::HumanBone::Hips]->value == 0, "node index zero must be valid");
	check(result.value().meta.avatarPermission == vrm10::AvatarPermission::OnlyAuthor, "avatarPermission default should be applied");
	check(result.value().meta.commercialUsage == vrm10::CommercialUsage::PersonalNonProfit, "commercialUsage default should be applied");
}

void testUnknownEnumValue() {
	auto json = validVrmJson();
	json["meta"]["avatarPermission"] = "futurePermission";
	auto result = vrm10::parseVrm(json);
	check(!result, "unknown enum value should fail parsing");
	if (!result) {
		check(result.error().code == vrm10::ErrorCode::UnknownEnumValue, "unknown enum should have a specific error code");
		check(result.error().path == "/meta/avatarPermission", "unknown enum error should include JSON Pointer");
	}
}

void testMissingRequiredBone() {
	auto json = validVrmJson();
	json["humanoid"]["humanBones"].erase("head");
	auto result = vrm10::parseVrm(json);
	check(!result, "missing required humanoid bone should fail parsing");
	if (!result) {
		check(result.error().code == vrm10::ErrorCode::MissingField, "missing required bone should use MissingField");
		check(result.error().path == "/humanoid/humanBones/head", "missing bone error should identify its path");
	}
}

void testOptionalExtensionData() {
	auto json = validVrmJson();
	json["firstPerson"] = nlohmann::json::parse(R"json(
		{"meshAnnotations": [{"node": 0, "type": "both"}]}
	)json");
	json["expressions"] = nlohmann::json::parse(R"json(
		{
			"preset": {"happy": {"isBinary": true}},
			"custom": {"customSmile": {"morphTargetBinds": [{"node": 2, "index": 0, "weight": 1.0}]}}
		}
	)json");
	json["lookAt"] = nlohmann::json::parse(R"json(
		{
			"type": "bone",
			"offsetFromHeadBone": [0.0, 0.06, 0.0],
			"rangeMapHorizontalOuter": {"inputMaxValue": 90.0, "outputScale": 10.0}
		}
	)json");
	auto result = vrm10::parseVrm(json);
	check(result.hasValue(), "optional VRM extension fields should parse");
	if (!result) return;
	check(result.value().firstPerson.has_value(), "firstPerson should be present");
	check(result.value().expressions.has_value(), "expressions should be present");
	check(result.value().expressions->preset.count("happy") == 1, "preset expression should be parsed");
	check(result.value().expressions->custom.count("customSmile") == 1, "custom expression should be parsed");
	check(result.value().lookAt.has_value(), "lookAt should be present");
}

void testValidation() {
	auto result = vrm10::parseVrm(validVrmJson());
	check(result.hasValue(), "validation fixture should parse");
	if (!result) return;
	auto report = vrm10::validate(result.value(), vrm10::ValidationContext{14, 0, 0});
	check(report.hasErrors(), "out-of-range node reference should be reported");
}


nlohmann::json validSpringBoneJson() {
	return nlohmann::json::parse(R"json(
	{
		"specVersion": "1.0",
		"colliders": [
			{"node": 0, "shape": {"sphere": {"radius": 0.1}}},
			{"node": 0, "shape": {"capsule": {"offset": [0.0, 0.1, 0.0], "tail": [0.0, 0.5, 0.0], "radius": 0.05}}}
		],
		"colliderGroups": [
			{"name": "body", "colliders": [0, 1]}
		],
		"springs": [
			{
				"name": "hair",
				"joints": [
					{"node": 1},
					{"node": 3, "hitRadius": 0.02, "stiffness": 1.5, "gravityPower": 0.2, "gravityDir": [0.0, -1.0, 0.0], "dragForce": 0.8}
				],
				"colliderGroups": [0],
				"center": 0
			}
		]
	}
	)json");
}

void testSpringBoneParsingAndDefaults() {
	auto result = vrm10::parseSpringBone(validSpringBoneJson());
	check(result.hasValue(), "valid VRMC_springBone data should parse");
	if (!result) return;

	const auto& springBone = result.value();
	check(springBone.specVersion == "1.0", "springBone specVersion should be preserved");
	check(springBone.colliders.size() == 2, "both collider shapes should be parsed");
	check(std::holds_alternative<vrm10::SphereCollider>(springBone.colliders[0].shape), "sphere collider should use the sphere variant");
	check(std::holds_alternative<vrm10::CapsuleCollider>(springBone.colliders[1].shape), "capsule collider should use the capsule variant");
	check(springBone.colliders[0].node.value == 0, "collider node index zero must be valid");
	check(springBone.colliderGroups.size() == 1, "collider group should be parsed");
	check(springBone.colliderGroups[0].colliders[0].value == 0, "collider index zero must be valid");
	check(springBone.springs.size() == 1, "spring chain should be parsed");
	check(springBone.springs[0].joints.size() == 2, "spring joints should be parsed");
	check(springBone.springs[0].joints[0].hitRadius == 0.0f, "hitRadius default should be zero");
	check(springBone.springs[0].joints[0].stiffness == 1.0f, "stiffness default should be one");
	check(springBone.springs[0].joints[0].gravityPower == 0.0f, "gravityPower default should be zero");
	check(springBone.springs[0].joints[0].gravityDir[1] == -1.0f, "gravityDir default should point downward");
	check(springBone.springs[0].joints[0].dragForce == 0.5f, "dragForce default should be one half");
	check(springBone.springs[0].colliderGroups[0].value == 0, "collider group index zero must be valid");
	check(springBone.springs[0].center.has_value() && springBone.springs[0].center->value == 0, "center node index zero must be valid");
}

void testSpringBoneInvalidInput() {
	auto bothShapes = validSpringBoneJson();
	bothShapes["colliders"][0]["shape"]["capsule"] = nlohmann::json::object();
	auto bothResult = vrm10::parseSpringBone(bothShapes);
	check(!bothResult, "collider shape must not contain both sphere and capsule");
	if (!bothResult) {
		check(bothResult.error().code == vrm10::ErrorCode::InvalidValue, "exclusive collider shape should report InvalidValue");
		check(bothResult.error().path == "/colliders/0/shape", "exclusive collider shape error should identify its JSON Pointer");
	}

	auto invalidDrag = validSpringBoneJson();
	invalidDrag["springs"][0]["joints"][0]["dragForce"] = 1.01;
	auto dragResult = vrm10::parseSpringBone(invalidDrag);
	check(!dragResult, "dragForce above one should be rejected");

	auto invalidRadius = validSpringBoneJson();
	invalidRadius["colliders"][0]["shape"]["sphere"]["radius"] = -0.1;
	auto radiusResult = vrm10::parseSpringBone(invalidRadius);
	check(!radiusResult, "negative collider radius should be rejected");

	auto unknownVersion = validSpringBoneJson();
	unknownVersion["specVersion"] = "2.0";
	auto versionResult = vrm10::parseSpringBone(unknownVersion);
	check(!versionResult, "unsupported SpringBone version should be rejected");
	if (!versionResult) {
		check(versionResult.error().code == vrm10::ErrorCode::UnsupportedVersion, "unsupported SpringBone version should have a specific error code");
	}
}

void testSpringBoneCrossReferenceValidation() {
	auto parsed = vrm10::parseSpringBone(validSpringBoneJson());
	check(parsed.hasValue(), "cross-reference fixture should parse");
	if (!parsed) return;

	vrm10::ValidationContext context;
	context.nodeCount = 4;
	context.nodeParents = {
		std::nullopt,
		vrm10::NodeIndex{0},
		vrm10::NodeIndex{1},
		vrm10::NodeIndex{2}
	};
	auto report = vrm10::validate(parsed.value(), context);
	check(report.empty(), "valid SpringBone references and ancestor relationships should pass validation");

	auto invalid = parsed.value();
	invalid.colliders[0].node = vrm10::NodeIndex{4};
	invalid.colliderGroups[0].colliders[0] = vrm10::SpringColliderIndex{3};
	invalid.springs[0].colliderGroups[0] = vrm10::SpringColliderGroupIndex{2};
	invalid.springs[0].center = vrm10::NodeIndex{4};
	auto invalidReport = vrm10::validate(invalid, context);
	check(invalidReport.hasErrors(), "out-of-range SpringBone references should be reported");

	auto brokenChain = parsed.value();
	brokenChain.springs[0].joints[1].node = vrm10::NodeIndex{0};
	auto chainReport = vrm10::validate(brokenChain, context);
	check(chainReport.hasErrors(), "non-descendant SpringBone joints should be reported");

	auto duplicateChain = parsed.value();
	duplicateChain.springs.push_back(duplicateChain.springs[0]);
	auto duplicateReport = vrm10::validate(duplicateChain, context);
	check(duplicateReport.hasErrors(), "a node shared by multiple SpringChains should be reported");
}

} // namespace

int main() {
	testRequiredDataAndNodeZero();
	testUnknownEnumValue();
	testMissingRequiredBone();
	testOptionalExtensionData();
	testValidation();
	testSpringBoneParsingAndDefaults();
	testSpringBoneInvalidInput();
	testSpringBoneCrossReferenceValidation();
	if (failures != 0) {
		std::cerr << failures << " test(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "All tests passed.\n";
	return EXIT_SUCCESS;
}
