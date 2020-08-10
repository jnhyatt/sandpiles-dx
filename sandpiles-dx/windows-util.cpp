#include "windows-util.h"

namespace sandbox
{
  std::unordered_map<log::Level, std::string> levelStrings {
    {log::Level::Verbose, "Verbose"},
    {log::Level::Debug, "Debug"},
    {log::Level::Info, "Info"},
    {log::Level::Warning, "Warning"},
    {log::Level::Error, "Error"},
    {log::Level::Fatal, "Fatal"},
  };

  WindowsConsole::WindowsConsole(): m_start(log::Clock::now()) {}

  void WindowsConsole::onMessageLogged(const log::Message& message) const
  {
    std::ostringstream out;
    std::string sourceTag(message.source);
    sourceTag.resize(16, ' ');
    std::string levelTag(levelStrings[message.level]);
    levelTag.resize(7, ' ');
    out << std::setw(12) << std::fixed << std::setprecision(6)
      << ((message.timeStamp - m_start).count() / 1'000'000'000.0) << " ["
      << sourceTag << "] [" << levelTag << "] " << message.message << std::endl;
    OutputDebugString(out.str().c_str());
  }

  std::ostream& operator<<(std::ostream& lhs, const WindowsError& error)
  {
    if (!error)
    {
      return lhs;
    }
    LPSTR messageBuffer = NULL;
    DWORD size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, error.id(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL
    );
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return lhs << message;
  }
}
