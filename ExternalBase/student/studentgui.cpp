#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../testloader.h"

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

enum StudentTab {
    STUDENT_TAB_TESTS,
    STUDENT_TAB_RESULTS,
    STUDENT_TAB_SETTINGS,
    STUDENT_TAB_NONE
};

StudentTab currentStudentTab = STUDENT_TAB_TESTS;
bool openSolveTestPopup = false;
TestDetails currentTest;
std::string currentTestFilename; // Добавленное поле для хранения имени файла теста
std::vector<std::string> studentAnswers;

void saveStudentAnswers(const TestDetails& test, const std::vector<std::string>& answers, const std::string& filename) {
    nlohmann::json j;
    j["subject"] = test.subject;
    j["testTheme"] = test.testTheme;
    j["questionCount"] = test.questionCount;
    j["questions"] = test.questions;  // Убедимся, что вопросы правильно сохраняются
    j["answers"] = answers;
    j["grade"] = ""; // Добавляем поле для оценки

    std::string studentFilename = "data/student_data/" + std::filesystem::path(filename).filename().string();
    std::ofstream file(studentFilename);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
        std::cout << "Student answers saved to " << studentFilename << std::endl;

        std::string prepodFilename = "data/prepod_data/" + std::filesystem::path(filename).filename().string();
        if (std::filesystem::exists(prepodFilename)) {
            std::filesystem::remove(prepodFilename);
            std::cout << "Test deleted from " << prepodFilename << std::endl;
        }
        else {
            std::cerr << "Test file not found: " << prepodFilename << std::endl;
        }
    }
    else {
        std::cerr << "Unable to open file for writing: " << studentFilename << std::endl;
    }
}

void ShowStudentGUI() {
    ImGui::Columns(2, NULL, false);

    ImGui::SetColumnWidth(0, 150);
    ImGui::Text("Sidebar");
    ImGui::Separator();
    if (ImGui::Button("Tests", ImVec2(-1, 0))) {
        currentStudentTab = STUDENT_TAB_TESTS;
    }
    if (ImGui::Button("Results", ImVec2(-1, 0))) {
        currentStudentTab = STUDENT_TAB_RESULTS;
    }
    
    ImGui::NextColumn();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(pos.x - ImGui::GetStyle().ItemSpacing.x / 2, pos.y - ImGui::GetWindowSize().y),
        ImVec2(pos.x - ImGui::GetStyle().ItemSpacing.x / 2, pos.y + ImGui::GetWindowSize().y),
        ImGui::GetColorU32(ImGuiCol_Separator));

    if (currentStudentTab == STUDENT_TAB_TESTS) {
        ImGui::Text("Available Tests");
        ImGui::BeginChild("Test List", ImVec2(0, 0), true);
        auto tests = loadTestsFromJson();
        for (const auto& testPair : tests) {
            const TestDetails& test = testPair.first;
            const std::string& filename = testPair.second;
            ImGui::Text("Test Theme: %s", test.testTheme.c_str());
            ImGui::Text("Subject: %s", test.subject.c_str());
            ImGui::Text("Number of Questions: %d", test.questionCount);

            if (ImGui::Button(("Solve Test##" + test.testTheme).c_str())) {
                currentTest = test;
                currentTestFilename = filename;
                studentAnswers.resize(currentTest.questionCount);
                for (auto& answer : studentAnswers) {
                    answer.clear();
                }
                openSolveTestPopup = true;
                ImGui::OpenPopup("Solve Test");
            }

            ImGui::Separator();
        }
        ImGui::EndChild();
    }
    else if (currentStudentTab == STUDENT_TAB_RESULTS) {
        ImGui::Text("Completed Tests and Grades");
        ImGui::BeginChild("Results List", ImVec2(0, 0), true);

        for (const auto& entry : std::filesystem::directory_iterator("data/student_data")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::ifstream file(entry.path());
                nlohmann::json j;
                file >> j;

                std::string subject = j["subject"];
                std::string testTheme = j["testTheme"];
                int questionCount = j["questionCount"];
                std::string grade = j["grade"];

                ImGui::Text("Test Theme: %s", testTheme.c_str());
                ImGui::Text("Subject: %s", subject.c_str());
                ImGui::Text("Number of Questions: %d", questionCount);
                ImGui::Text("Grade: %s", grade.c_str());

                ImGui::Separator();
            }
        }
        ImGui::EndChild();
    }

    else if (currentStudentTab == STUDENT_TAB_SETTINGS) {

        const char* ascii_art_cat = R"(
//  ,_     _ 
//  |\_,-~/
//  / _  _ |    ,--.
// (  @  @ )   / ,-'  ya dumal syda ne budut smotret...
//  \  _T_/-._( (
//  /         `. \
//  |         _  \ |
//  \ \ ,  /      |
//  || |-_\__   /
//  ((_/`(____,-'
   )";

        ImGui::TextUnformatted(ascii_art_cat);
    }

    ImGui::Columns(1);

    if (openSolveTestPopup) {
        ImGui::OpenPopup("Solve Test");
        openSolveTestPopup = false;
    }

    if (ImGui::BeginPopupModal("Solve Test", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Test Theme: %s", currentTest.testTheme.c_str());
        ImGui::Text("Subject: %s", currentTest.subject.c_str());

        for (int i = 0; i < currentTest.questionCount; ++i) {
            ImGui::Text("Question %d: %s", i + 1, currentTest.questions[i].c_str());
            char buf[256];
            strncpy(buf, studentAnswers[i].c_str(), sizeof(buf));
            if (ImGui::InputText(("Answer " + std::to_string(i + 1)).c_str(), buf, sizeof(buf))) {
                studentAnswers[i] = std::string(buf);
            }
        }

        if (ImGui::Button("Submit")) {
            std::cout << "Submit button clicked." << std::endl;
            for (int i = 0; i < studentAnswers.size(); ++i) {
                std::cout << "Answer " << i + 1 << ": " << studentAnswers[i] << std::endl;
            }
            saveStudentAnswers(currentTest, studentAnswers, currentTestFilename);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
