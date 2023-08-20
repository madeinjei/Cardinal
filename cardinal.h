#pragma once

enum LogPriority
{
	TracePriority, DebugPriority, InfoPriority, WarnPriority, ErrorPriority, CriticalPriority
};

class Logger
{
public:
	template<typename... Args>
	static void Trace(const char* message, Args... args)
	{
		log("[Trace]\t", TracePriority, message, args...);
	}

	template<typename... Args>
	static void Debug(const char* message, Args... args)
	{
		log("[Debug]\t", DebugPriority, message, args...);
	}

	template<typename... Args>
	static void Info(const char* message, Args... args)
	{
		log("[Info]\t", InfoPriority, message, args...);
	}

	template<typename... Args>
	static void Warn(const char* message, Args... args)
	{
		log("[Warn]\t", WarnPriority, message, args...);
	}

	template<typename... Args>
	static void Error(const char* message, Args... args)
	{
		log("[Error]\t", ErrorPriority, message, args...);
	}

	template<typename... Args>
	static void Critical(const char* message, Args... args)
	{
		log("[Critical]\t", CriticalPriority, message, args...);
	}

private:
	template<typename... Args>
	static void log(const char* message_priority_str, LogPriority message_priority, const char* message, Args... args)
	{
		switch (message_priority)
		{
		case TracePriority:
			printf("[TRACE]\t");

			break;
		case DebugPriority:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE);
			printf("[DEBUG]\t");

			break;
		case InfoPriority:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
			printf("[INFO]\t");

			break;
		case WarnPriority:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
			printf("[WARN]\t");

			break;
		case ErrorPriority:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_RED);
			printf("[ERROR]\t");

			break;
		case CriticalPriority:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
			printf("[CRITICAL]\t");

			break;
		default:
			break;
		}

		printf(message, args...);
		printf("\n");

		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
};

class Directory {
public:
	static std::vector<char> ReadFile(const std::string& fileName)
	{
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{

			//Logger::Failed(CDNL_WIN32_SYSTEM, "FAILED TO OPEN FILE");

			throw std::runtime_error("[WIN32] : FAILED TO OPEN FILE");
		}

		size_t fileSize = (size_t)file.tellg();

		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
};