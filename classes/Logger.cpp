#include "Logger.h"

std::vector<LogEntry> Logger::_buffer;
bool Logger::_scrollToBottom = false;

Logger& Logger::GetInstance() 
{
    static Logger instance;
    return instance;
}

void Logger::ToFile() 
{
    std::string filename = "debug_log.txt";
    std::filesystem::path filePath = std::filesystem::current_path() / filename;
    std::ofstream logFile(filePath);
    for (const auto& entry : _buffer) logFile << entry.Message;
    Logger::Info("Debug log saved to " + filePath.string());
}

void Logger::Clear() { _buffer.clear(); }

void Logger::AddLog(const std::string& tag, const std::string& message, LogType type) 
{
    LogEntry entry = { tag + " " + message + "\n", type };
    _buffer.push_back(entry);
    _scrollToBottom = true;
}

void Logger::Info(const std::string& message) { Logger::AddLog("[INFO]", message, INFO); }

void Logger::Event(const std::string& message) { Logger::AddLog("[EVENT]", message, EVENT); }

void Logger::Warn(const std::string& message) { Logger::AddLog("[WARNING]", message, WARNING); }

void Logger::Error(const std::string& message) { Logger::AddLog("[ERROR]", message, ERROR); }

void Logger::Draw(const std::string& title) 
{
    ImGui::Begin(title.c_str());

    // Draw each message in its respective color
    for (const auto& entry : _buffer) 
    {
        ImVec4 Color;
        switch (entry.Type) 
        {
            case INFO:    Color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break; // White
            case EVENT:   Color = ImVec4(0.0f, 0.5f, 1.0f, 1.0f); break; // Blue
            case WARNING: Color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; // Yellow
            case ERROR:   Color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break; // Red
        }
        ImGui::PushStyleColor(ImGuiCol_Text, Color);
        ImGui::TextWrapped("%s", entry.Message.c_str());
        ImGui::PopStyleColor();
    }

    // Scroll to the bottom of messages
    if (_scrollToBottom) ImGui::SetScrollHereY(1.0f);
    _scrollToBottom = false;

    ImGui::End();
}
