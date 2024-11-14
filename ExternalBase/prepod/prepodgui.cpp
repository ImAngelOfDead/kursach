#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "prepodgui.h"
#include "../json/json.hpp"
#include "../testloader.h"
using json = nlohmann::json;

#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <random>
#include <iomanip>

enum Tab {
    TAB_TESTS,
    TAB_RESULTS,
    TAB_SETTINGS,
    TAB_NONE
};

TestDetails newTest;
TestDetails editTest;
Tab currentTab = TAB_TESTS;
bool openEnterQuestionsPopup = false;
bool openEditTestPopup = false;
bool openDeleteTestPopup = false;
bool openCheckTestPopup = false;
std::string editTestFilename;
std::string deleteTestFilename;
std::string checkTestFilename;
std::vector<std::string> checkTestAnswers;
std::vector<std::string> checkTestQuestions;
std::string studentGrade;

// Функция для генерации UUID
std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);

    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << part1
        << std::setw(16) << std::setfill('0') << part2;
    return ss.str();
}

void saveTestToJson(const TestDetails& test, const std::string& filename) {
    nlohmann::json j;

    // Заполняем объект json данными теста
    j["subject"] = test.subject;
    j["testTheme"] = test.testTheme;
    j["questionCount"] = test.questionCount;
    j["questions"] = test.questions;  // Вектор вопросов будет автоматически преобразован в JSON-массив

    // Открываем файловый поток для записи
    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4);  // Сохраняем JSON с отступами для читабельности
        file.close();
        std::cout << "Test saved to " << filename << std::endl;
    }
    else {
        std::cerr << "Unable to open file for writing: " << filename << std::endl;
    }
}

void deleteTestFromJson(const std::string& filename) {
    std::cout << "Attempting to delete file: " << filename << std::endl;
    if (std::filesystem::exists(filename)) {
        std::filesystem::remove(filename);
        std::cout << "Test deleted: " << filename << std::endl;
    }
    else {
        std::cerr << "File not found: " << filename << std::endl;
    }
}

void saveStudentGrade(const std::string& filename, const std::string& grade) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Unable to open file for reading: " << filename << std::endl;
        return;
    }

    nlohmann::json j;
    file >> j;
    file.close();

    j["grade"] = grade;

    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << j.dump(4);
        outFile.close();
        std::cout << "Grade saved to " << filename << std::endl;
    }
    else {
        std::cerr << "Unable to open file for writing: " << filename << std::endl;
    }
}

void ShowPrepodGUI() {
    ImGui::Columns(2, NULL, false);

    ImGui::SetColumnWidth(0, 150);
    ImGui::Text("Sidebar");
    ImGui::Separator();
    if (ImGui::Button("Tests", ImVec2(-1, 0))) {
        currentTab = TAB_TESTS;
    }
    if (ImGui::Button("Results", ImVec2(-1, 0))) {
        currentTab = TAB_RESULTS;
    }

    ImGui::NextColumn();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(pos.x - ImGui::GetStyle().ItemSpacing.x / 2, pos.y - ImGui::GetWindowSize().y),
        ImVec2(pos.x - ImGui::GetStyle().ItemSpacing.x / 2, pos.y + ImGui::GetWindowSize().y),
        ImGui::GetColorU32(ImGuiCol_Separator));

    if (currentTab == TAB_TESTS) {
        ImGui::Text("Test List");
        if (ImGui::Button("Create Test")) {
            ImGui::OpenPopup("Create New Test");
        }
        ImGui::BeginChild("Test List", ImVec2(0, 0), true);

        auto tests = loadTestsFromJson();
        for (auto& test : tests) {
            std::string filename = test.second;
            TestDetails testDetails = test.first;
            ImGui::Text("Test Theme: %s", testDetails.testTheme.c_str());
            ImGui::Text("Subject: %s", testDetails.subject.c_str());
            ImGui::Text("Number of Questions: %d", testDetails.questionCount);

            ImGui::SameLine();
            if (ImGui::Button(("Edit##" + testDetails.testTheme).c_str())) {
                editTest = testDetails;
                editTestFilename = filename;
                openEditTestPopup = true;
                ImGui::OpenPopup("Edit Test");
            }

            ImGui::SameLine();
            if (ImGui::Button(("Delete##" + testDetails.testTheme).c_str())) {
                deleteTestFilename = filename;
                openDeleteTestPopup = true;
                ImGui::OpenPopup("Delete Test");
            }

            ImGui::Separator();
        }
        ImGui::EndChild();
    }
    else if (currentTab == TAB_RESULTS) {
        ImGui::Text("Results");
        ImGui::BeginChild("Results List", ImVec2(0, 0), true);
        for (const auto& entry : std::filesystem::directory_iterator("data/student_data")) {
            if (entry.path().extension() == ".json") {
                std::ifstream file(entry.path());
                nlohmann::json j;
                file >> j;

                std::string subject = j["subject"];
                std::string testTheme = j["testTheme"];
                int questionCount = j["questionCount"];
                std::vector<std::string> questions = j["questions"];
                std::vector<std::string> answers = j["answers"];
                std::string grade = j["grade"];

                ImGui::Text("Test Theme: %s", testTheme.c_str());
                ImGui::Text("Subject: %s", subject.c_str());
                ImGui::Text("Number of Questions: %d", questionCount);
                ImGui::Text("Grade: %s", grade.c_str());

                ImGui::SameLine();
                if (ImGui::Button(("Check##" + testTheme).c_str())) {
                    checkTestQuestions = questions;
                    checkTestAnswers = answers;
                    checkTestFilename = entry.path().string();
                    openCheckTestPopup = true;
                    ImGui::OpenPopup("Check Test");
                }

                ImGui::Separator();
            }
        }
        ImGui::EndChild();
    }
    else if (currentTab == TAB_SETTINGS) {

    }

    ImGui::Columns(1);

    if (ImGui::BeginPopupModal("Create New Test", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char subjectBuf[100] = "";
        static char themeBuf[100] = "";
        static int questionCount = 1;

        ImGui::InputText("Subject", subjectBuf, IM_ARRAYSIZE(subjectBuf));
        ImGui::InputText("Test Theme", themeBuf, IM_ARRAYSIZE(themeBuf));
        ImGui::InputInt("Number of Questions", &questionCount);

        
        if (questionCount < 1) questionCount = 1;

        if (ImGui::Button("Next")) {
            newTest.subject = subjectBuf;
            newTest.testTheme = themeBuf;
            newTest.questionCount = questionCount;
            newTest.questions.resize(questionCount);
            openEnterQuestionsPopup = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    static bool newTestInitialized = false;
    static std::vector<std::string> newTestQuestionBuffers;
    if (openEnterQuestionsPopup) {
        ImGui::OpenPopup("Enter Questions");
        openEnterQuestionsPopup = false;
    }

    if (ImGui::BeginPopupModal("Enter Questions", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!newTestInitialized) {
            newTestQuestionBuffers.resize(newTest.questionCount);
            newTestInitialized = true;
        }

        for (int i = 0; i < newTest.questionCount; ++i) {
            if (newTestQuestionBuffers[i].size() < 256)
                newTestQuestionBuffers[i].resize(256);
            ImGui::InputTextMultiline(("Question " + std::to_string(i + 1)).c_str(), &newTestQuestionBuffers[i][0], 256);
        }

        if (ImGui::Button("Save Test")) {
            for (int i = 0; i < newTest.questionCount; ++i) {
                newTest.questions[i] = newTestQuestionBuffers[i];
            }
            std::string filename = "data/prepod_data/" + generateUUID() + ".json";
            saveTestToJson(newTest, filename);
            newTestInitialized = false;
            newTestQuestionBuffers.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            newTestInitialized = false;
            newTestQuestionBuffers.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (openEditTestPopup) {
        ImGui::OpenPopup("Edit Test");
        openEditTestPopup = false;
    }

    static bool editTestInitialized = false;
    static std::vector<std::string> editTestQuestionBuffers;
    if (ImGui::BeginPopupModal("Edit Test", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!editTestInitialized) {
            editTestQuestionBuffers.resize(editTest.questionCount);
            for (int i = 0; i < editTest.questionCount; i++) {
                editTestQuestionBuffers[i] = editTest.questions[i];
                if (editTestQuestionBuffers[i].size() < 256)
                    editTestQuestionBuffers[i].resize(256);
            }
            editTestInitialized = true;
        }

        for (int i = 0; i < editTest.questionCount; ++i) {
            ImGui::InputTextMultiline(("Question " + std::to_string(i + 1)).c_str(), &editTestQuestionBuffers[i][0], 256);
        }

        if (ImGui::Button("Save Changes")) {
            for (int i = 0; i < editTest.questionCount; ++i) {
                editTest.questions[i] = editTestQuestionBuffers[i];
            }
            saveTestToJson(editTest, editTestFilename);
            editTestInitialized = false;
            editTestQuestionBuffers.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            editTestInitialized = false;
            editTestQuestionBuffers.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (openDeleteTestPopup) {
        ImGui::OpenPopup("Delete Test");
        openDeleteTestPopup = false;
    }

    if (ImGui::BeginPopupModal("Delete Test", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete this test?");
        if (ImGui::Button("Yes")) {
            deleteTestFromJson(deleteTestFilename);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (openCheckTestPopup) {
        ImGui::OpenPopup("Check Test");
        openCheckTestPopup = false;
    }

    if (ImGui::BeginPopupModal("Check Test", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        for (size_t i = 0; i < checkTestQuestions.size(); ++i) {
            ImGui::Text("Question: %s", checkTestQuestions[i].c_str());
            ImGui::Text("Answer: %s", checkTestAnswers[i].c_str());
            ImGui::Separator();
        }
        static char gradeBuf[100] = "";
        ImGui::InputText("Grade", gradeBuf, IM_ARRAYSIZE(gradeBuf));

        if (ImGui::Button("Save Grade")) {
            studentGrade = gradeBuf;
            saveStudentGrade(checkTestFilename, studentGrade);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}