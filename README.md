# Task-List-Kill
C++ code emulating Tasklist and Taskkill functions
TaskList & TaskKill C++ Utilities

A pair of utilities (and example source) demonstrating process and service enumeration (TaskList-like) and interactive process termination (TaskKill-like). These are example projects intended for learning, lab testing, and defensive development. Do not use these tools on systems you do not own or administer.

## Contents
- TKILL2_V2.cpp — Interactive TaskKill-like utility that lists running processes (via tasklist) and runs taskkill for local or remote targets.
- TaskList.cpp — TaskList-style utility that enumerates processes, resolves executable paths and memory usage, maps services to process IDs via the Service Control Manager, and exports results to TaskList_Output.txt and TaskList_Output.csv.
- Build notes, usage examples, detection and telemetry guidance, and license.

## Features
- TaskList.cpp
  - Enumerates processes using Toolhelp and PSAPI.
  - Resolves process executable paths and working set memory (KB).
  - Maps Windows services to hosting PIDs using EnumServicesStatusExW and resolves display names.
  - Exports human-readable text and CSV outputs.
- TKILL2_V2.cpp
  - Retrieves process list by invoking tasklist and parsing output.
  - Interactive selection by number, PID, or process name.
  - Supports remote termination via taskkill /S with optional /U /P credentials.
  - Builds and executes taskkill commands (local or remote) and shows results.

## Requirements
- Windows 7 or later (examples use Win32 APIs).
- Visual Studio (recommended) or any C++ compiler targeting the Windows API.
- Link with advapi32.lib and psapi.lib for TaskList.cpp (pragmas included).
- Run with appropriate privileges to enumerate remote services or terminate protected processes (Administrator required for some operations).

## Build Instructions
1. Create a new Visual Studio Console Project or compile from the command line.
2. Add the respective .cpp files to the project.
3. Ensure the following libraries are linked (TaskList.cpp):
   - advapi32.lib
   - psapi.lib
   These are already declared via pragma comment in the sample.
4. Build configuration: Release (x86 or x64) as appropriate for your target system.

Example command-line with Microsoft cl (Developer Command Prompt):
- Compile TaskList.cpp:
  cl /EHsc /W4 TaskList.cpp /link advapi32.lib psapi.lib
- Compile TKILL2_V2.cpp:
  cl /EHsc /W4 TKILL2_V2.cpp

## Usage
- TaskList
  - Run the compiled executable.
  - Produces:
    - TaskList_Output.txt — human-readable list of Image Name, PID, Services, Memory, and Path.
    - TaskList_Output.csv — CSV export with the same fields.
  - Typical command:
    TaskList.exe

- TKILL2_V2
  - Run the compiled executable.
  - The program will:
    - Invoke tasklist to retrieve processes.
    - Show a numbered interactive list (first 350 entries).
    - Prompt for selection by number, PID, or process name.
    - Optionally prompt for remote host (/S) and credentials (/U /P).
    - Confirm and run taskkill with /F.
  - Typical workflow:
    TKILL2_V2.exe
    Follow prompts to select and confirm termination.


## Command Switches (tasklist & taskkill)

- tasklist /V
  - What it does: Shows verbose output for each process, including window title, session, memory usage, and other extended columns.
  - Emulated behavior in TaskList.cpp: The sample already reports memory usage (working set) and executable path; /V would add extra UI-related fields such as Window Title and Session info. To fully emulate /V you can add:
    - QueryFullProcessImageName / GetProcessTimes for timing,
    - EnumWindows + GetWindowThreadProcessId to capture window titles for processes with visible windows,
    - WTSQuerySessionInformation to get session ID/username details.
  - Detection note: /V-style verbose enumeration increases API calls (window enumeration, session queries) and telemetry footprint.

- tasklist /SVC
  - What it does: Shows services hosted in service host (svchost) processes — lists services associated with each PID.
  - Emulated behavior in TaskList.cpp: The sample’s GetServiceProcessMapping() already performs EnumServicesStatusExW and maps service display names to process IDs, producing the same per-PID services column as tasklist /SVC.
  - Detection note: SCM enumeration (OpenSCManager + EnumServicesStatusExW) is a distinct telemetry indicator you can detect and correlate with process enumeration.

=====================================================================================

- taskkill /PID <pid>
  - What it does: Targets a process by its Process ID (PID) for termination.
  - Emulated behavior in TKILL2_V2.cpp: When the user selects by number or enters a PID, the tool builds a taskkill command using /PID <pid> and executes it.
  - Detection note: Look for command lines containing "/PID <number>" and subsequent Process Terminate events for that PID.

- taskkill /IM <imagename>
  - What it does: Targets processes by image name (e.g., notepad.exe) and attempts to terminate matching processes.
  - Emulated behavior in TKILL2_V2.cpp: When user types a process name, the program appends "/IM \"name.exe\"" to the command.
  - Detection note: Command lines with "/IM" can affect multiple PIDs; detect parent processes spawning taskkill with /IM followed by many terminations.

- taskkill /F
  - What it does: Forces termination (kill) of the target process(es) rather than requesting graceful exit.
  - Emulated behavior in TKILL2_V2.cpp: The sample always appends /F to force termination; this results in immediate termination events.
  - Detection note: Forced kills often produce abrupt termination logs; high volume of /F uses is more suspicious.

- taskkill /S <system>
  - What it does: Specifies a remote system to connect to for terminating processes (uses RPC to the remote Service Control Manager).
  - Emulated behavior in TKILL2_V2.cpp: The program prompts for remote host and, if supplied, includes "/S <host>" in the taskkill invocation, optionally with credentials.
  - Detection note: Remote /S usage may produce network connections and authentication events; correlate with network telemetry and remote service RPC activity.

- taskkill /U <username>
  - What it does: Specifies credentials (user) to use when connecting to a remote system (/S).
  - Emulated behavior in TKILL2_V2.cpp: If a remote host is provided, the program prompts for /U and includes it in the taskkill command line.
  - Detection note: Credentials on the command line may be exposed in process command-line logs; watch for unexpected credentialed remote management actions.

- taskkill /T
  - What it does: Terminates the specified process and any child processes it spawned (tree kill).
  - Emulated behavior in TKILL2_V2.cpp: Not implemented by default; could be added by appending /T when building the taskkill command to include child process termination.
  - Detection note: /T leads to multiple terminations across a process tree—detect by correlating originator process and cascading termination events.

## Output Examples
- TaskList_Output.csv (columns):
  "Image Name","PID","Services","Memory (KB)","Path"
- TaskList_Output.txt:
  Image Name: svchost.exe
  PID: 1234
  Services: Windows Update, Time Service
  Memory: 10240 KB
  Path: C:\Windows\System32\svchost.exe
  -----------------------------------------

- TKILL2_V2.exe
  Interactive console showing process list, built taskkill command (e.g., taskkill /PID 1234 /F), and command result output.

## Detection & Defensive Notes
These tools demonstrate observable behaviors useful for defenders:
- TaskList-like program: Process enumeration (Toolhelp APIs), OpenProcess with PROCESS_VM_READ, GetModuleFileNameEx, EnumServicesStatusExW, and file writes (text/CSV) — visible in Procmon, Sysmon (ProcessCreate, ProcessAccess, FileCreate/Write), ETW, and EDR telemetry.
- TaskKill-like program: Spawns tasklist.exe and taskkill.exe using a shell, command lines containing /PID or /IM and /F, and may contact remote hosts when using /S. Detect by correlating parent-child process creation, command-line arguments, ProcessAccess/Termination events, and unusual execution contexts or unsigned binaries.
Use these samples in a controlled lab to collect representative telemetry for detection tuning. Do not run on production systems without authorization.

## Security & Responsible Use
- These are demonstration utilities. Unauthorized use to enumerate or terminate processes on systems you do not administer is unethical and may be illegal.
- Avoid embedding credentials in scripts or command lines; supplying credentials to taskkill /P can expose secrets in process command-line logs.
- When testing remote termination functions, use isolated lab networks and accounts created for testing.

## Limitations & Known Issues
- TKILL2_V2 uses tasklist/taskkill via _popen and relies on command-line parsing; localized Windows installations or different tasklist output formats may break parsing.
- TaskList uses OpenProcess(PROCESS_VM_READ) which may fail for protected or elevated system processes when run without Administrator privileges; memory values or paths may be missing for such processes.
- Service mapping depends on EnumServicesStatusExW; some system/service configs may return pid 0 for services running in shared svchost instances or when services are not currently assigned a process.

## Extending / Customization Ideas
- TaskList:
  - Add JSON output option.
  - Include CPU time and threads count (QueryProcessCycleTime / GetProcessTimes).
  - Add per-process loaded modules listing.
- TKILL2_V2:
  - Add non-interactive CLI flags for automation (e.g., --pid, --im, --remote).
  - Add dry-run mode and logging to file.
  - Replace tasklist parsing with native Toolhelp enumeration to avoid locale issues.

## Testing & Telemetry Collection
- Install Sysmon with Process Create, Process Access, and FileCreate/FileWrite enabled to capture relevant telemetry.
- Use Procmon to capture exact API call sequences for lab runs.
- Record command-line arguments and parent process chains to build high-fidelity detection rules.

## License
- GNU GENERAL PUBLIC LICENSE -- Version 3, 29 June 2007 — see LICENSE file.

## Contributing
- Fork the repository, create a feature branch, and open a pull request.
- Keep changes focused: bug fixes, cross-locale parsing, improved error handling, or telemetry hooks for defensive testing are welcome.

## Attribution
Author: Private
Date: March 22, 2026

## Contact
For issues, feature requests, or discussion, open an issue on the repository.
