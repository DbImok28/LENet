#pragma once
#include <iostream>
#include <format>
#include <chrono>

namespace LimeEngine::Net
{
	class NetLogger
	{
        NetLogger() = delete;

    private:
        static int ElapsedSeconds()
        {
            static auto start = std::chrono::system_clock::now();
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end - start;
            return elapsed_seconds.count();
        }

    public:
        static void LogCore(const std::string& msg)
        {
            std::cout << "[Net " << ElapsedSeconds() << "s] " << msg << std::endl;
        }
        template <typename... TArgs>
        static void LogCore(const std::format_string<TArgs...> formatMsg, TArgs&&... args)
        {
            std::cout << "[Net " << ElapsedSeconds() << "s] " << std::format(formatMsg, std::forward<TArgs>(args)...) << std::endl;
        }

        static void LogUser(const std::string& msg)
        {
            std::cout << "[Net " << ElapsedSeconds() << "s] " << msg << std::endl;
        }
        template <typename... TArgs>
        static void LogUser(const std::format_string<TArgs...> formatMsg, TArgs&&... args)
        {
            std::cout << "[User " << ElapsedSeconds() << "s] " << std::format(formatMsg, std::forward<TArgs>(args)...) << std::endl;
        }

    private:
        inline static auto runTime = std::chrono::system_clock::now();
	};
}
