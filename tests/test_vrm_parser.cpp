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



void testNodeConstraintParsingAndValidation() {
	auto roll = vrm10::parseNodeConstraint(nlohmann::json::parse(R"json(
		{"specVersion":"1.0","constraint":{"roll":{"source":0,"rollAxis":"Y"}}}
	)json"));
	check(roll.hasValue(), "roll constraint should parse");
	if (roll) {
		check(std::holds_alternative<vrm10::RollConstraint>(roll.value().constraint), "roll constraint variant should be selected");
		check(std::get<vrm10::RollConstraint>(roll.value().constraint).weight == 1.0f, "constraint weight should default to one");
	}

	auto aim = vrm10::parseNodeConstraint(nlohmann::json::parse(R"json(
		{"specVersion":"1.0","constraint":{"aim":{"source":1,"aimAxis":"NegativeZ","weight":0.25}}}
	)json"));
	check(aim.hasValue(), "aim constraint should parse");
	if (aim) {
		check(std::get<vrm10::AimConstraint>(aim.value().constraint).aimAxis == vrm10::AimAxis::NegativeZ, "aim axis enum should be parsed");
	}

	auto rotation = vrm10::parseNodeConstraint(nlohmann::json::parse(R"json(
		{"specVersion":"1.0","constraint":{"rotation":{"source":2}}}
	)json"));
	check(rotation.hasValue(), "rotation constraint should parse");

	auto badWeight = vrm10::parseNodeConstraint(nlohmann::json::parse(R"json(
		{"specVersion":"1.0","constraint":{"rotation":{"source":2,"weight":1.1}}}
	)json"));
	check(!badWeight, "constraint weight above one should be rejected");

	auto both = vrm10::parseNodeConstraint(nlohmann::json::parse(R"json(
		{"specVersion":"1.0","constraint":{"roll":{"source":0,"rollAxis":"X"},"aim":{"source":0,"aimAxis":"PositiveY"}}}
	)json"));
	check(!both, "node constraint must contain exactly one constraint kind");

	std::vector<std::optional<vrm10::NodeConstraint>> graph(3);
	graph[0] = rotation.value();
	graph[1] = roll.value();
	auto validReport = vrm10::validate(graph, 3);
	check(validReport.empty(), "acyclic in-range node constraints should validate");

	graph[0] = vrm10::NodeConstraint{
		"1.0",
		vrm10::RotationConstraint{vrm10::NodeIndex{1}, 1.0f}
	};
	graph[1] = vrm10::NodeConstraint{
		"1.0",
		vrm10::RotationConstraint{vrm10::NodeIndex{0}, 1.0f}
	};
	auto cycleReport = vrm10::validate(graph, 3);
	check(cycleReport.hasErrors(), "circular node constraint dependencies should be reported");

	graph[0] = roll.value();
	graph[1].reset();
	auto selfReport = vrm10::validate(graph, 3);
	check(selfReport.hasErrors(), "constraint source equal to destination should be reported");
}

void testSpringBoneNestedExtensions() {
	auto json = validSpringBoneJson();
	json["colliders"][0]["extensions"]["VRMC_springBone_extended_collider"] = nlohmann::json::parse(R"json(
		{
			"specVersion": "1.0",
			"shape": {
				"sphere": {
					"offset": [0.0, 0.0, 0.0],
					"radius": 0.5,
					"inside": true
				}
			}
		}
	)json");
	json["springs"][0]["joints"][0]["extensions"]["VRMC_springBone_limit"] = nlohmann::json::parse(R"json(
		{
			"specVersion": "1.0",
			"limit": {
				"cone": {
					"angle": 0.5,
					"rotation": [0.0, 0.0, 0.0, 1.0]
				}
			}
		}
	)json");

	auto parsed = vrm10::parseSpringBone(json);
	check(parsed.hasValue(), "nested SpringBone extension data should parse");
	if (!parsed) return;
	check(parsed.value().colliders[0].extended.has_value(), "extended collider should be attached to its base collider");
	if (parsed.value().colliders[0].extended && parsed.value().colliders[0].extended->shape) {
		check(std::holds_alternative<vrm10::SphereExtendedCollider>(*parsed.value().colliders[0].extended->shape), "extended sphere shape should be parsed");
		check(std::get<vrm10::SphereExtendedCollider>(*parsed.value().colliders[0].extended->shape).inside, "inside collider flag should be preserved");
	}
	check(parsed.value().springs[0].joints[0].limit.has_value(), "joint limit extension should be attached to its joint");
	if (parsed.value().springs[0].joints[0].limit) {
		check(std::holds_alternative<vrm10::ConeLimit>(parsed.value().springs[0].joints[0].limit->limit), "cone limit should be parsed");
	}

	auto badNormal = nlohmann::json::parse(R"json(
		{"specVersion":"1.0","shape":{"plane":{"normal":[0.0,2.0,0.0]}}}
	)json");
	check(!vrm10::parseSpringBoneExtendedCollider(badNormal), "non-normalized extended plane normal should be rejected");

	auto badQuaternion = nlohmann::json::parse(R"json(
		{"specVersion":"1.0","limit":{"hinge":{"angle":1.0,"rotation":[0.0,0.0,0.0,0.5]}}}
	)json");
	check(!vrm10::parseSpringBoneLimit(badQuaternion), "non-unit SpringBone limit quaternion should be rejected");

	auto tailLimitJson = validSpringBoneJson();
	tailLimitJson["springs"][0]["joints"][1]["extensions"]["VRMC_springBone_limit"] = nlohmann::json::parse(R"json(
		{"specVersion":"1.0","limit":{"cone":{"angle":1.0}}}
	)json");
	auto tailLimit = vrm10::parseSpringBone(tailLimitJson);
	check(tailLimit.hasValue(), "tail joint limit should remain parseable for diagnostics");
	if (tailLimit) {
		vrm10::ValidationContext context;
		context.nodeCount = 4;
		auto report = vrm10::validate(tailLimit.value(), context);
		bool foundWarning = false;
		for (const auto& issue : report.issues) {
			if (issue.code == "spring-limit-on-tail-ignored" && issue.severity == vrm10::ValidationSeverity::Warning) {
				foundWarning = true;
			}
		}
		check(foundWarning, "limit on the final tail joint should produce an ignore warning");
	}
}

void testMToonAndHdrParsing() {
	auto mtoon = vrm10::parseMaterialsMToon(nlohmann::json::parse(R"json(
		{
			"specVersion": "1.0",
			"shadeColorFactor": [0.7, 0.8, 0.9],
			"shadeMultiplyTexture": {"index": 0},
			"shadingShiftTexture": {"index": 1, "scale": -0.2},
			"outlineWidthMode": "screenCoordinates",
			"uvAnimationScrollXSpeedFactor": 0.5
		}
	)json"));
	check(mtoon.hasValue(), "MToon material should parse");
	if (mtoon) {
		check(mtoon.value().shadingToonyFactor == 0.9f, "MToon shadingToonyFactor default should be applied");
		check(mtoon.value().giEqualizationFactor == 0.9f, "MToon giEqualizationFactor default should be applied");
		check(mtoon.value().shadeMultiplyTexture.has_value(), "MToon texture info should be parsed");
		check(mtoon.value().shadingShiftTexture->scale == -0.2f, "shading shift texture scale should be preserved");
		vrm10::ValidationContext context;
		context.textureCount = 2;
		check(vrm10::validate(mtoon.value(), context).empty(), "in-range MToon texture references should validate");
		context.textureCount = 1;
		check(vrm10::validate(mtoon.value(), context).hasErrors(), "out-of-range MToon texture references should be reported");
	}

	auto badMtoon = vrm10::parseMaterialsMToon(nlohmann::json::parse(R"json(
		{"specVersion":"1.0","shadingToonyFactor":1.1}
	)json"));
	check(!badMtoon, "MToon factors above one should be rejected");

	auto hdr = vrm10::parseHdrEmissiveMultiplier(nlohmann::json::parse(R"json(
		{"emissiveMultiplier":2.5}
	)json"));
	check(hdr.hasValue() && hdr.value().emissiveMultiplier == 2.5f, "HDR emissive multiplier should parse");
	auto badHdr = vrm10::parseHdrEmissiveMultiplier(nlohmann::json::parse(R"json(
		{"emissiveMultiplier":-1.0}
	)json"));
	check(!badHdr, "negative HDR emissive multiplier should be rejected");
}



void testExpressionSemanticParsing() {
	auto invalidWeight = validVrmJson();
	invalidWeight["expressions"] = nlohmann::json::parse(R"json(
		{
			"custom": {
				"smile": {
					"morphTargetBinds": [
						{"node": 2, "index": 0, "weight": 1.01}
					]
				}
			}
		}
	)json");
	auto weightResult = vrm10::parseVrm(invalidWeight);
	check(!weightResult, "MorphTargetBind weight above one should be rejected");
	if (!weightResult) {
		check(weightResult.error().code == vrm10::ErrorCode::InvalidValue, "invalid morph weight should report InvalidValue");
		check(
			weightResult.error().path == "/expressions/custom/smile/morphTargetBinds/0/weight",
			"invalid morph weight should identify its JSON Pointer"
		);
	}

	auto duplicateName = validVrmJson();
	duplicateName["expressions"] = nlohmann::json::parse(R"json(
		{
			"preset": {"happy": {}},
			"custom": {"happy": {}}
		}
	)json");
	auto duplicateResult = vrm10::parseVrm(duplicateName);
	check(!duplicateResult, "custom expression names must not duplicate preset names");
	if (!duplicateResult) {
		check(
			duplicateResult.error().path == "/expressions/custom/happy",
			"duplicate expression error should identify the custom expression name"
		);
	}
}

void testVrmHierarchyAndMorphTargetValidation() {
	auto json = validVrmJson();
	json["expressions"] = nlohmann::json::parse(R"json(
		{
			"custom": {
				"smile": {
					"morphTargetBinds": [
						{"node": 2, "index": 0, "weight": 0.5}
					]
				}
			}
		}
	)json");
	auto parsed = vrm10::parseVrm(json);
	check(parsed.hasValue(), "VRM semantic validation fixture should parse");
	if (!parsed) return;

	vrm10::ValidationContext context;
	context.nodeCount = 15;
	context.nodeParents = {
		std::nullopt,
		vrm10::NodeIndex{0},
		vrm10::NodeIndex{1},
		vrm10::NodeIndex{0},
		vrm10::NodeIndex{3},
		vrm10::NodeIndex{4},
		vrm10::NodeIndex{0},
		vrm10::NodeIndex{6},
		vrm10::NodeIndex{7},
		vrm10::NodeIndex{1},
		vrm10::NodeIndex{9},
		vrm10::NodeIndex{10},
		vrm10::NodeIndex{1},
		vrm10::NodeIndex{12},
		vrm10::NodeIndex{13}
	};
	context.nodeMorphTargetCounts.resize(context.nodeCount);
	context.nodeMorphTargetCounts[2] = 1;
	auto validReport = vrm10::validate(parsed.value(), context);
	check(validReport.empty(), "valid humanoid hierarchy and morph target reference should pass");

	auto invalidMorph = parsed.value();
	invalidMorph.expressions->custom.at("smile").morphTargetBinds[0].index = 1;
	auto morphReport = vrm10::validate(invalidMorph, context);
	bool foundMorphIndexIssue = false;
	for (const auto& issue : morphReport.issues) {
		if (issue.code == "morph-target-index-out-of-range") foundMorphIndexIssue = true;
	}
	check(foundMorphIndexIssue, "out-of-range morph target index should be reported");

	auto nodeWithoutMesh = context;
	nodeWithoutMesh.nodeMorphTargetCounts[2] = std::nullopt;
	auto noMeshReport = vrm10::validate(parsed.value(), nodeWithoutMesh);
	bool foundNoMeshIssue = false;
	for (const auto& issue : noMeshReport.issues) {
		if (issue.code == "morph-target-node-has-no-mesh") foundNoMeshIssue = true;
	}
	check(foundNoMeshIssue, "morph target bind to node without mesh should be reported");

	auto invalidHierarchy = context;
	invalidHierarchy.nodeParents[2] = vrm10::NodeIndex{0};
	auto hierarchyReport = vrm10::validate(parsed.value(), invalidHierarchy);
	bool foundHierarchyIssue = false;
	for (const auto& issue : hierarchyReport.issues) {
		if (issue.code == "humanoid-bone-not-descendant-of-parent") foundHierarchyIssue = true;
	}
	check(foundHierarchyIssue, "invalid humanoid parent-child relationship should be reported");

	auto missingRequiredParent = parsed.value();
	missingRequiredParent.humanoid[vrm10::HumanBone::UpperChest] = vrm10::NodeIndex{15};
	auto expandedContext = context;
	expandedContext.nodeCount = 16;
	expandedContext.nodeParents.resize(16);
	expandedContext.nodeMorphTargetCounts.resize(16);
	auto parentReport = vrm10::validate(missingRequiredParent, expandedContext);
	bool foundMissingParentIssue = false;
	for (const auto& issue : parentReport.issues) {
		if (issue.code == "missing-humanoid-parent-bone") foundMissingParentIssue = true;
	}
	check(foundMissingParentIssue, "upperChest without its required chest parent should be reported");
}

} // namespace

int main() {
	testRequiredDataAndNodeZero();
	testUnknownEnumValue();
	testMissingRequiredBone();
	testOptionalExtensionData();
	testValidation();
	testExpressionSemanticParsing();
	testVrmHierarchyAndMorphTargetValidation();
	testSpringBoneParsingAndDefaults();
	testSpringBoneInvalidInput();
	testSpringBoneCrossReferenceValidation();
	testNodeConstraintParsingAndValidation();
	testSpringBoneNestedExtensions();
	testMToonAndHdrParsing();
	if (failures != 0) {
		std::cerr << failures << " test(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "All tests passed.\n";
	return EXIT_SUCCESS;
}
