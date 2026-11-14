#include <vrm10/vrm10.hpp>

#include <cstdlib>

int main() {
	const auto json = nlohmann::json::parse(R"json(
		{
			"specVersion": "1.0",
			"springs": [
				{
					"joints": [
						{"node": 0}
					]
				}
			]
		}
	)json");

	const auto parsed = vrm10::parseSpringBone(json);
	if (!parsed) return EXIT_FAILURE;
	if (parsed.value().springs.size() != 1) return EXIT_FAILURE;
	if (parsed.value().springs[0].joints[0].node.value != 0) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
