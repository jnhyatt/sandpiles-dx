#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

namespace sandbox {
  namespace log
  {
    enum class Level { Verbose, Debug, Info, Warning, Error, Fatal };

    typedef std::chrono::high_resolution_clock Clock;

    struct Message
    {
      Message(std::string source, Level level, std::string message, Clock::time_point timeStamp):
        source(std::move(source)), level(level), message(std::move(message)), timeStamp(timeStamp) {}
      Message(const Message&) = default;
      Message(Message&&) = default;

      const std::string source;
      const Level level;
      const std::string message;
      const Clock::time_point timeStamp;
    };

    class Target
    {
    public:
      void logMessage(std::string source, Level level, std::string message) const
      {
        onMessageLogged(Message(source, level, message, Clock::now()));
      }

      virtual void onMessageLogged(const Message& message) const = 0;
    };

    class Splitter: public Target
    {
    public:
      void addTarget(const Target& target)
      {
        m_targets.push_back(&target);
      }

      virtual void onMessageLogged(const Message& message) const override
      {
        for (const Target* pTarget : m_targets)
        {
          pTarget->onMessageLogged(message);
        }
      }

    private:
      std::vector<const Target*> m_targets;
    };
  }

  class Logger
  {
  private:
    class LogBuffer
    {
    public:
      LogBuffer(const log::Target& target, std::string source, log::Level level): m_target(target), m_source(std::move(source)), m_level(level) {}
      LogBuffer(const LogBuffer& other): m_target(other.m_target), m_source(other.m_source), m_level(other.m_level) {}
      LogBuffer(LogBuffer&& other): m_target(other.m_target), m_source(std::move(other.m_source)), m_level(std::move(other.m_level)) {}
      ~LogBuffer() { m_target.logMessage(m_source, m_level, m_buffer.str()); }

      template <typename T> LogBuffer& operator<<(const T& t)
      {
        m_buffer << t;
        return *this;
      }

    private:
      const log::Target& m_target;
      log::Level m_level;
      std::string m_source;
      std::ostringstream m_buffer;
    };

  public:
    Logger(const log::Target& target, std::string source): m_target(target), source(std::move(source)) {}

    LogBuffer verbose() const { return LogBuffer(m_target, source, log::Level::Verbose); }
    LogBuffer debug() const { return LogBuffer(m_target, source, log::Level::Debug); }
    LogBuffer info() const { return LogBuffer(m_target, source, log::Level::Info); }
    LogBuffer warning() const { return LogBuffer(m_target, source, log::Level::Warning); }
    LogBuffer error() const { return LogBuffer(m_target, source, log::Level::Error); }
    LogBuffer fatal() const { return LogBuffer(m_target, source, log::Level::Fatal); }

  public:
    std::string source;

  private:
    const log::Target& m_target;
  };
}
