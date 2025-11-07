#pragma once
#include "../imgui/imgui.h"
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

enum LogType { INFO, EVENT, WARNING, ERROR };

struct LogEntry
{
	std::string Message;
	LogType Type;
};

class Logger 
{
public:
	static Logger& GetInstance();
	void ToFile();
	void Clear();
	void Info(const std::string& message);
	void Event(const std::string& message);
	void Warn(const std::string& message);
	void Error(const std::string& message);
	void Draw(const std::string& title);
private:
	static std::vector<LogEntry> _buffer;
	static bool _scrollToBottom;
	void AddLog(const std::string& tag, const std::string& message, LogType type);
};
