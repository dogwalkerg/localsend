import 'dart:io';

Future<void> startBundledHelperIfNeeded() async {
  if (!Platform.isWindows) {
    return;
  }

  try {
    final isRunning = await _isProcessRunning('playplus.exe');
    if (isRunning) {
      return;
    }

    final helperPath = _bundledHelperPath();
    if (!await File(helperPath).exists()) {
      return;
    }

    await Process.start(
      helperPath,
      const [],
      mode: ProcessStartMode.detached,
      runInShell: false,
      workingDirectory: File(helperPath).parent.path,
    );
  } catch (_) {
    return;
  }
}

Future<bool> _isProcessRunning(String processName) async {
  final result = await Process.run(
    'tasklist',
    ['/FI', 'IMAGENAME eq $processName', '/NH'],
    runInShell: true,
  );

  final output = '${result.stdout}\n${result.stderr}'.toLowerCase();
  return output.contains(processName.toLowerCase());
}

String _bundledHelperPath() {
  final executableDir = File(Platform.resolvedExecutable).parent.path;
  return '$executableDir\\data\\flutter_assets\\localsend.exe';
}
