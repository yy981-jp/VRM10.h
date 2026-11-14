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

} // namespace

int main() {
	testRequiredDataAndNodeZero();
	testUnknownEnumValue();
	testMissingRequiredBone();
	testOptionalExtensionData();
	testValidation();
	if (failures != 0) {
		std::cerr << failures << " test(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "All tests passed.\n";
	return EXIT_SUCCESS;
}
