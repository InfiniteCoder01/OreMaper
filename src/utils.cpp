#include "00Names.hpp"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#else
#include <unistd.h>
#endif

std::filesystem::path getProgramPath() {
  std::string buffer(1024, '\0');
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
  GetModuleFileName(nullptr, buffer, sizeof(buffer));
#else
  (void)readlink("/proc/self/exe", buffer.data(), buffer.size());
#endif
  return {buffer};
}
