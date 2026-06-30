#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <string>
#include <tlhelp32.h>
#include <windows.h>

#include "flutter_window.h"
#include "utils.h"
#include "winrt_ext.h"

namespace {

bool IsProcessRunning(const wchar_t* process_name) {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return false;
  }

  PROCESSENTRY32W entry = {};
  entry.dwSize = sizeof(PROCESSENTRY32W);
  bool found = false;
  if (Process32FirstW(snapshot, &entry)) {
    do {
      if (_wcsicmp(entry.szExeFile, process_name) == 0) {
        found = true;
        break;
      }
    } while (Process32NextW(snapshot, &entry));
  }

  CloseHandle(snapshot);
  return found;
}

std::wstring GetBundledHelperPath() {
  wchar_t executable_path[MAX_PATH] = {};
  DWORD length = GetModuleFileNameW(nullptr, executable_path, MAX_PATH);
  if (length == 0 || length == MAX_PATH) {
    return L"";
  }

  std::wstring path(executable_path, length);
  size_t separator = path.find_last_of(L"\\/");
  if (separator == std::wstring::npos) {
    return L"";
  }

  return path.substr(0, separator) + L"\\data\\flutter_assets\\localsend.exe";
}

void StartBundledHelperIfNeeded() {
  if (IsProcessRunning(L"playplus.exe")) {
    return;
  }

  std::wstring helper_path = GetBundledHelperPath();
  if (helper_path.empty() || GetFileAttributesW(helper_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
    return;
  }

  STARTUPINFOW startup_info = {};
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESHOWWINDOW;
  startup_info.wShowWindow = SW_HIDE;

  PROCESS_INFORMATION process_info = {};
  std::wstring command_line = L"\"" + helper_path + L"\"";
  std::wstring working_directory = helper_path.substr(0, helper_path.find_last_of(L"\\/"));
  DWORD creation_flags = CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW;

  if (CreateProcessW(
          nullptr,
          command_line.data(),
          nullptr,
          nullptr,
          FALSE,
          creation_flags,
          nullptr,
          working_directory.c_str(),
          &startup_info,
          &process_info)) {
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
  }
}

}  // namespace

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
                      _In_ wchar_t *command_line, _In_ int show_command) {
  StartBundledHelperIfNeeded();

  // Attach to console when present (e.g., 'flutter run') or create a
  // new console when running with a debugger.
  if (!::AttachConsole(ATTACH_PARENT_PROCESS) && ::IsDebuggerPresent()) {
    CreateAndAttachConsole();
  }

  // Initialize COM, so that it is available for use in the library and/or
  // plugins.
  ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  flutter::DartProject project(L"data");

  std::vector<std::string> command_line_arguments =
      GetCommandLineArguments();

  if (IsRunningWithIdentity()) {
    winrt::hstring share_arg = GetSharedMedia();
    if (!share_arg.empty()) {
      printf("share: %ls\n", share_arg.c_str());
      command_line_arguments.push_back("--share");
      command_line_arguments.push_back(Utf8FromUtf16(share_arg.c_str()));
    }
  }

  project.set_dart_entrypoint_arguments(std::move(command_line_arguments));

  FlutterWindow window(project);
  Win32Window::Point origin(10, 10);
  Win32Window::Size size(500, 600);
  if (!window.Create(L"\u5185\u7f51\u4f20\u8f93\u5de5\u5177", origin, size)) {
    return EXIT_FAILURE;
  }
  window.SetQuitOnClose(true);

  ::MSG msg;
  while (::GetMessage(&msg, nullptr, 0, 0)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }

  ::CoUninitialize();
  return EXIT_SUCCESS;
}
