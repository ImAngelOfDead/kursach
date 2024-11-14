#pragma once

//#include "D:/projects/cpp/ImGui-External-Base-main/ExternalBase/nlohmann/json.hpp"
#include "json/json.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <utility>

struct TestDetails {
    std::string subject;
    std::string testTheme;
    int questionCount;
    std::vector<std::string> questions;

    TestDetails() : questionCount(0) {}
};

std::vector<std::pair<TestDetails, std::string>> loadTestsFromJson();
