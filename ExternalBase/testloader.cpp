#define _CRT_SECURE_NO_WARNINGS
#include "testloader.h"
#include <filesystem>
#include <fstream>


std::vector<std::pair<TestDetails, std::string>> loadTestsFromJson() {
    std::vector<std::pair<TestDetails, std::string>> tests;
    std::string path = "data/prepod_data/";
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            nlohmann::json j;
            file >> j;
            TestDetails test;
            test.subject = j["subject"];
            test.testTheme = j["testTheme"];
            test.questionCount = j["questionCount"];
            test.questions = j["questions"].get<std::vector<std::string>>();
            tests.emplace_back(test, entry.path().string());
        }
    }
    return tests;
}