#pragma once

#include "log.h"

#include <Windows.h>

namespace sandbox
{
  class WindowsConsole: public log::Target
  {
  public:
    WindowsConsole();

    virtual void onMessageLogged(const log::Message& message) const override;

  private:
    log::Clock::time_point m_start;
  };

  class WindowsError
  {
  public:
    WindowsError(const DWORD id): m_id(id) {}

    operator bool() const { return m_id; }
    DWORD id() const { return m_id; }

    static WindowsError last() { return WindowsError(GetLastError()); }

  private:
    const DWORD m_id;
  };

  std::ostream& operator<<(std::ostream& lhs, const WindowsError& error);
}
