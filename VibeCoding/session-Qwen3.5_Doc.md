# </think>Repo overview request

**Session ID:** ses_3932425b2ffej7BA7uccfck1qm
**Created:** 2/18/2026, 2:27:53 AM
**Updated:** 2/18/2026, 2:28:28 AM

---

## User

describe repo

---

## Assistant (Sisyphus · qwen3.5-plus-2026-02-15 · 7.5s)

_Thinking:_

The user wants a description of the repository. I'll gather information about the repo structure, README, and key configuration files in parallel to provide a comprehensive overview.

```
Tool: bash

**Input:**
```json
{
  "command": "ls -la",
  "description": "List root directory contents"
}
```
**Output:**
```
total 1072
drwxrwxr-x 12 hangyu5 hangyu5   4096 Feb 17 13:51 .
drwxrwxr-x 26 hangyu5 hangyu5   4096 Feb 18 01:34 ..
-rw-rw-r--  1 hangyu5 hangyu5     34 Feb  7  2024 .env.example
drwxrwxr-x 11 hangyu5 hangyu5   4096 Feb 16 22:46 .git
-rw-rw----  1 hangyu5 hangyu5    383 May 25  2024 .gitattributes
drwxrwxr-x  3 hangyu5 hangyu5   4096 Mar 16  2024 .github
-rw-rw-r--  1 hangyu5 hangyu5    276 Jan 23  2025 .gitignore
-rw-rw-r--  1 hangyu5 hangyu5    453 Jun  2  2024 .gitmodules
drwxrwxr-x  4 hangyu5 hangyu5   4096 Feb  3  2025 .idea
drwxrwxr-x  2 hangyu5 hangyu5   4096 Jan 25  2024 .vscode
-rw-rw-r--  1 hangyu5 hangyu5  11944 Feb 16 14:07 AI_CVar.md
-rw-rw-r--  1 hangyu5 hangyu5  14085 Feb 16 23:27 AI_DeviceManagerVk.md
-rw-rw-r--  1 hangyu5 hangyu5   3492 Feb 16 16:19 AI_DeviceVK.md
-rw-rw-r--  1 hangyu5 hangyu5  39858 Feb 17 13:37 AI_DeviceVK2.md
-rw-rw-r--  1 hangyu5 hangyu5  42755 Feb 17 13:37 AI_DeviceVK3.md
-rw-rw-r--  1 hangyu5 hangyu5  24532 Feb 17 13:51 AI_DeviceVK4.md
drwxrwxr-x  3 hangyu5 hangyu5   4096 Mar 31  2024 Binary
-rw-rw-r--  1 hangyu5 hangyu5   2562 Feb  7 10:45 DOC_AI.md
-rw-rw-r--  1 hangyu5 hangyu5  12414 Feb 16 00:07 DOOM3_nvrhi.md
drwxrwxr-x  4 hangyu5 hangyu5   4096 Jun  2  2024 Document
drwxrwxr-x  6 hangyu5 hangyu5   4096 Feb  7 17:15 Engine
-rwxrwxr-x  1 hangyu5 hangyu5    230 Jul  9  2025 GenerateCMakeProjects.sh
-rw-rw-r--  1 hangyu5 hangyu5   5074 Feb 16 00:39 HLVM_nvrhi.md
-rw-rw-r--  1 hangyu5 hangyu5   1063 Jan  1  2025 LICENSE
-rw-rw-r--  1 hangyu5 hangyu5   1338 Jul  8  2025 LICENSE_1_0.txt
-rw-rw-r--  1 hangyu5 hangyu5   9073 Apr 22  2024 README.md
-rw-rw-r--  1 hangyu5 hangyu5   9029 Feb  7 01:03 README_AI.md
-rw-rw-r--  1 hangyu5 hangyu5  10240 Feb  7 10:54 README_AI2.md
drwxrwxr-x  2 hangyu5 hangyu5   4096 Jan 14  2024 Samples
-rwxrwxr-x  1 hangyu5 hangyu5    961 May 11  2024 Setup.sh
-rw-rw-r--  1 hangyu5 hangyu5    964 Feb 18  2025 TODO.excalidraw
-rw-rw-r--  1 hangyu5 hangyu5   6773 Jan 23  2025 TODO.md
-rw-rw-r--  1 hangyu5 hangyu5    222 Feb 13 23:48 TODO_2026.md
drwxrwxr-x  9 hangyu5 hangyu5   4096 Feb  2 00:10 Vibe_Coding
drwxrwxr-x  4 hangyu5 hangyu5   4096 Feb 18 00:43 build
-rw-rw-r--  1 hangyu5 hangyu5  93602 Feb 16 22:14 diff.tmp
-rw-rw-r--  1 hangyu5 hangyu5  35518 Dec 14  2024 diff2.tmp
-rw-rw-r--  1 hangyu5 hangyu5   2214 Jun  2  2024 env.yaml
-rw-rw-r--  1 hangyu5 hangyu5 382863 Feb 16 16:16 session-AI_CVar.md
-rw-rw-r--  1 hangyu5 hangyu5 267848 Feb 16 00:29 session-ses_39df.md

```
```

```
Tool: bash

**Input:**
```json
{
  "command": "find . -maxdepth 2 -type f -name \"*.md\" | head -20",
  "description": "Find markdown files (docs)"
}
```
**Output:**
```
./session-AI_CVar.md
./AI_DeviceVK3.md
./README.md
./AI_DeviceVK.md
./TODO_2026.md
./README_AI.md
./README_AI2.md
./TODO.md
./AI_DeviceVK2.md
./DOOM3_nvrhi.md
./Document/README.md
./HLVM_nvrhi.md
./AI_DeviceManagerVk.md
./AI_DeviceVK4.md
./AI_CVar.md
./DOC_AI.md
./session-ses_39df.md

```
```

```
Tool: bash

**Input:**
```json
{
  "command": "export CI=true DEBIAN_FRONTEND=noninteractive GIT_TERMINAL_PROMPT=0 GCM_INTERACTIVE=never HOMEBREW_NO_AUTO_UPDATE=1 GIT_EDITOR=: EDITOR=: VISUAL='' GIT_SEQUENCE_EDITOR=: GIT_MERGE_AUTOEDIT=no GIT_PAGER=cat PAGER=cat npm_config_yes=true PIP_NO_INPUT=1 YARN_ENABLE_IMMUTABLE_INSTALLS=false; git remote -v 2>/dev/null || echo \"No remote configured\"",
  "description": "Check git remote"
}
```
**Output:**
```
origin	https://github.com/yhyu13/HLVM-Engine.git (fetch)
origin	https://github.com/yhyu13/HLVM-Engine.git (push)

[Category+Skill Reminder]

You are an orchestrator agent. Consider whether this work should be delegated:

**DELEGATE when:**
- UI/Frontend work → category: "visual-engineering", skills: ["frontend-ui-ux"]
- Complex logic/architecture → category: "ultrabrain"
- Quick/trivial tasks → category: "quick"
- Git operations → skills: ["git-master"]
- Browser automation → skills: ["playwright"] or ["agent-browser"]

**DO IT YOURSELF when:**
- Gathering context/exploring codebase
- Simple edits that are part of a larger task you're coordinating
- Tasks requiring your full context understanding

Example delegation:
```
delegate_task(
  category="visual-engineering",
  load_skills=["frontend-ui-ux"],
  description="Implement responsive navbar with animations",
  run_in_background=true
)
```

```
```

```
Tool: bash

**Input:**
```json
{
  "command": "find . -maxdepth 3 -type d -not -path '*/\\.*' | head -30",
  "description": "Show directory structure"
}
```
**Output:**
```
.
./Samples
./build
./build/CMakeFiles
./build/CMakeFiles/pkgRedirects
./Binary
./Binary/GNULinux-x64
./Binary/GNULinux-x64/lib
./Binary/GNULinux-x64/CMake-3.31.8
./Binary/GNULinux-x64/Ninja-1.12.0
./Binary/GNULinux-x64/CMake-3.30.1
./Binary/GNULinux-x64/Clang-17
./Binary/GNULinux-x64/CMake-3.29.3
./Document
./Document/Engine
./Document/Engine/Sphinx
./Document/Engine/Doxygen
./Engine
./Engine/Scripts
./Engine/Scripts/pycmake
./Engine/Scripts/Redis
./Engine/Scripts/pyarg
./Engine/Source
./Engine/Source/Dependency
./Engine/Source/Runtime
./Engine/Source/Editor
./Engine/Source/Common
./Engine/Source/Plugin
./Engine/Binary
./Engine/Binary/GNULinux-x64

```
```

---

## Assistant (Sisyphus · qwen3.5-plus-2026-02-15 · 8.9s)

```
Tool: read

**Input:**
```json
{
  "filePath": "/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/README.md"
}
```
**Output:**
```
<file>
00001| # HLVM-Engine
00002| 
00003| ## Brief
00004| 
00005| 是工作中受UE启发的个人游戏引擎练习项目，在搭建游戏引擎关键基础设施的同时，额外目标是创造比UE5更友好更便捷的pak文件查读和内存管理、遥测系统，同时磨刀linux开发工具链和c++20特性熟练度，不涉及渲染动画特效等一系列游戏内容开发的工作流程，欢迎交流学习
00006| 
00007| ## Build
00008| 
00009| ### Linux-x64
00010| 
00011| #### Prerequisites:
00012| 
00013| 配置暂时放在zhihu上，有时间再放到项目bundle在一起
00014| https://zhuanlan.zhihu.com/p/677704467
00015| 
00016| - Anaconda3
00017| - git
00018| - clang-16
00019| - cmake 3.28
00020| 
00021| 然后
00022| ```
00023| ./Setup.sh
00024| ./GenerateCMakeProjects.sh
00025| ```
00026| 
00027| ## Features
00028| 
00029|  - Implement custom build system called `PyCMake` python package
00030|    - Internally generates CMake projects
00031|    - Unit testing with CTest
00032|  - CI/DI
00033|    - Use [cmake-multi-platform.yml](.github/workflows/cmake-multi-platform.yml) to deploy on github actions (WIP, not working yet as it is incredibly slow due to boost build)
00034|    - Anaconda adaptation to work with conda action (i.e. `conda init && conda activate` in `Setup.sh` scripts)
00035|  - Platform-specific code for Linux and Windows
00036|    - Platform specific Compare Swap Exchange [link](Engine/Source/Common/Public/Platform/GenericPlatformAtomicPointer.h)
00037|    - Platform specific debugging utilities [link](Engine/Source/Common/Public/Platform/GenericPlatformDebuggerUtil.h)
00038|  - File system handling with Boost [link](Engine/Source/Common/Public/Platform/FileSystem/Boost/BoostMapFileHandle.h)
00039|    - Use `Boost` for local file mapping
00040|    - Use `fstream` for file reading and writing
00041|    - Support `std` compatible file modes
00042|    - Implement a set of standard C file operations in cpp
00043|  - Compression 
00044|    - Using `Zstd` with a cpp wrapper [link](Engine/Source/Common/Public/Core/Compress/Zstd.h)
00045|  - Encryption
00046|    - Using `Botan3` wrapped in an interface [link](Engine/Source/Common/Public/Core/Encrypt/RSA.h)
00047|    - Use RSA OAEP for encryption
00048|    - Use RSA EMSA for signatures
00049|    - Generate PKCS8 RSA 256 keys in binary but obfuscated
00050|  - Logging system [link](Engine/Source/Common/Public/Core/Log.h)
00051|    - Implement UE5 like logging macro `HLVM_LOG(LogXXX, debug, TXT(...), ...)`
00052|      - Easy define new `LogXXX` log category
00053|      - Compile time eliminate unwanted logging levels based on settings made in each log category
00054|    - Implement UE5 like log device redirector
00055|      - Using `spdlog` as one of the log device to pump log into various sinks (e.g. files, console)
00056|      - Pump `spdlog`'s `trace debug and infor` into async thread so that these logs won't block
00057|      - `warn, err and critical` log are sync flushed
00058|  - Custom memory management withe heap and stack allocation 
00059|    - Implement global mallocator interface class where mimalloc and stack allocator can override & exchange in runtime [link](Engine/Source/Common/Private/Core/Mallocator/Mallocator.cpp) 
00060|      - Implement TLS `GMallocator` which use TLS mimallocator as default
00061|      - Override global `new` and `delete` to use `GMallocator`
00062|      - Combine with `Scoped variable` to swap in & out TLS `GMallocator` with temporal allocator
00063|      - Optimized stack allocator and is 2/3 cost of time comparing to `mimalloc` in average case
00064|    - Implement custom mimalloc-like VM mallocator  (WIP) 
00065|      - the goal is to be thread-local and lock-free and replicate a simplified version of free list sharding (WIP)
00066|      - Virtual memory arena [link](Engine/Source/Common/Public/Core/Mallocator/VMMallocator/VMArena.h)
00067|       - Multiple Heap live inside a single virtual memory arena, heaps are used to large blocks allocation [link](Engine/Source/Common/Public/Core/Mallocator/VMMallocator/VMHeap.h)
00068|       - small binned allocator designed to be fast and lock-free [link](Engine/Source/Common/Public/Core/Mallocator/VMMallocator/SmallBinnedMallocator.h)
00069|  - Obfuscation techniques for sensitive strings [link](Engine/Source/Common/Public/Template/Obfuscate/MetaString.tpp)
00070|    - Use `AdvoObfuscator` to obfuscate short strings
00071|    - Implement simpler stack free obfuscation method for long string (e.g. RSA private keys)
00072|  - Custom string handling
00073|    - Use `chat8_t` as string char type, as it is compatible with `char` simply by reinterpreting cast
00074|    - Implement UE5 like string handling, e.g. `TXT("...")` and `FString` type to replace `std::string`
00075|    - Implement `FPath` which internally uses `boost::filesystem::path` and cached path fast hash
00076|    - Stack allocated string, useful for critical error handling where dynamic allocation is prohibited (WIP)
00077|    - Const String pooling like UE5 `FName` and `FText` (WIP)
00078|  - Custom file system handling with packing support
00079|    - Token file [link](Engine/Source/Common/Public/Platform/FileSystem/Packed/PackedToken.h)
00080|      - describes the file structure (e.g. offset, size, compression, etc)
00081|      - Store `size_t` hash instead of string path (smaller token file size but not able to search and parse file paths in runtime)
00082|      - debug json file that describes the token file content in human-readable format
00083|    - Container file [link](Engine/Source/Common/Public/Platform/FileSystem/Packed/PackedContainerFragment.h)
00084|      - tightly packs the data files for memory mapping
00085|      - Runtime fragment that loads a minimal 4MB of mapped file region per fragment
00086|      - Proxy that load actual content using token entry and container fragment
00087|  - Custom exception handling [link](Engine/Source/Common/Public/Core/Assert.h)
00088|    - No-inline exception handling function (avoid lengthy inline assembly code for string formatting)
00089|    - Using `backwardcpp` from debugging utilities to print stack trace
00090|    - Using `spdlog` for logging and `fmt` to combine exception message, stacktrace and file & lino
00091|  - Boost hashing functions [link](Engine/Source/Common/Public/Utility/Hash.h)
00092|    - MD5 digest hash and converting to hex string
00093|    - SHA1 digest hash and converting to hex string
00094|  - Custom template meta-programming for common tasks [link](Engine/Source/Common/Public/Template/GlobalTemplate.tpp)
00095|    - String obfuscation (e.g. in binary private key, password, etc)
00096|    - Compile time regex matching `ctre` for assertion checkings
00097|    - Scoped variable templates (e.g. scoped file handle, scoped timer, etc)
00098|    - Optional object reference removal
00099|    - Reference object removal
00100|    - Non-null pointer template
00101|    - Memory-alloc free `printf` template
00102|  - Debugging utilities for Linux and Windows [link](Engine/Source/Common/Public/Platform/GenericPlatformDebuggerUtil.h)
00103|    - Using `ptrace` under linux to determine if a debugger is present
00104|    - Using `WinAPI` under windows to determine if a debugger is present
00105|    - Using `backwardcpp` library to print stack trace, support user skipping `N` frames from bottom
00106|  - Parallelism
00107|    - Spin lock [link](Engine/Source/Common/Public/Core/Parallel/Lock.h)
00108|      - Use `std::atomic_flag` which is mutex free
00109|      - Use `_mm_pause` and thread yielding to avoid busy waiting
00110|      - Deaklock timer and lock guard
00111|    - Rival lock [link](Engine/Source/Common/Public/Core/Parallel/Lock.h)
00112|      - Avoid using `spin lock` on cases where only reading and writing are enough are mutually exclusive
00113|      - Use `rival lock` so that all reading are permissive and mutually exclusive to writing, and vice versa
00114|    - Concurrent `SPSC/MPSC/MPMC` queue [link](Engine/Source/Common/Public/Core/Parallel/ConcurrentQueue.h)
00115|      - Inspired from UE5 lock free (i.e. compare swap exchange)`TQueue` implementation but extend to MPMC
00116|      - 1.5x performance of Boost `lockfree::queue` (See test case TestParallel) 
00117|      - Found out that use `cv` and `std::mutex` to block on poping (if empty) actually increase performance
00118|    - Work Steal thread pool [link](Engine/Source/Common/Public/Core/Parallel/Async/WorkStealThreadPool.h)
00119|      - Use a pool of `std::thread` and return `std::future` to use
00120|      - Use `Concurrent MPMC queue` (with block on empty feat) to store tasks internally
00121|      - Implement `work stealing` algorithm to get tasks efficiently to an idle worker
00122|      - Implement various thread affinity mask to customize threading mode based on hardware and platform
00123|    - Fiber pool (Not work so well) [link](Engine/Source/Common/Public/Core/Parallel/Async/WorkStealFiberPool.h)
00124|      - Using `boost::fibers` as base to implement fiber pool, though its a mess inside (should consider native async scheduling?)
00125|      - Scheduling method `work stealing` and launch method `post` does not work all the time (might stuck for no apparent reason)
00126|      - Scheduling method `shared` and launch method `dispatch` generally work but fiber pool still blocks on `round robin` when running low threads
00127|      - Fiber pool generally require more threads to run to achieve high performance than a thread pool
00128| 
00129| ### Spcial Thanks, Credits, 3rdParty
00130| 
00131| - [AdvoObfuscator](https://github.com/andrivet/ADVobfuscator)
00132| - fetch_packages:
00133|     - yalantinlibs
00134|     - backward
00135|     - parallel_hashmap
00136|     - ctre
00137|     - string_pool
00138| 
00139| - find_packages:
00140|     - spdlog
00141|     - mimalloc
00142|     - magic_enum
00143|     - Boost
00144|     - botan3
00145|     - zstd
00146|     - rapidjson
00147|     - opentelemetry
00148|     - protobuf
00149|     - grpc
00150|     - curl
00151|     - nlohmann_json
00152| 

(End of file - total 152 lines)
</file>

[Project README: /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/README.md]
# HLVM-Engine

## Brief

是工作中受UE启发的个人游戏引擎练习项目，在搭建游戏引擎关键基础设施的同时，额外目标是创造比UE5更友好更便捷的pak文件查读和内存管理、遥测系统，同时磨刀linux开发工具链和c++20特性熟练度，不涉及渲染动画特效等一系列游戏内容开发的工作流程，欢迎交流学习

## Build

### Linux-x64

#### Prerequisites:

配置暂时放在zhihu上，有时间再放到项目bundle在一起
https://zhuanlan.zhihu.com/p/677704467

- Anaconda3
- git
- clang-16
- cmake 3.28

然后
```
./Setup.sh
./GenerateCMakeProjects.sh
```

## Features

 - Implement custom build system called `PyCMake` python package
   - Internally generates CMake projects
   - Unit testing with CTest
 - CI/DI
   - Use [cmake-multi-platform.yml](.github/workflows/cmake-multi-platform.yml) to deploy on github actions (WIP, not working yet as it is incredibly slow due to boost build)
   - Anaconda adaptation to work with conda action (i.e. `conda init && conda activate` in `Setup.sh` scripts)
 - Platform-specific code for Linux and Windows
   - Platform specific Compare Swap Exchange [link](Engine/Source/Common/Public/Platform/GenericPlatformAtomicPointer.h)
   - Platform specific debugging utilities [link](Engine/Source/Common/Public/Platform/GenericPlatformDebuggerUtil.h)
 - File system handling with Boost [link](Engine/Source/Common/Public/Platform/FileSystem/Boost/BoostMapFileHandle.h)
   - Use `Boost` for local file mapping
   - Use `fstream` for file reading and writing
   - Support `std` compatible file modes
   - Implement a set of standard C file operations in cpp
 - Compression 
   - Using `Zstd` with a cpp wrapper [link](Engine/Source/Common/Public/Core/Compress/Zstd.h)
 - Encryption
   - Using `Botan3` wrapped in an interface [link](Engine/Source/Common/Public/Core/Encrypt/RSA.h)
   - Use RSA OAEP for encryption
   - Use RSA EMSA for signatures
   - Generate PKCS8 RSA 256 keys in binary but obfuscated
 - Logging system [link](Engine/Source/Common/Public/Core/Log.h)
   - Implement UE5 like logging macro `HLVM_LOG(LogXXX, debug, TXT(...), ...)`
     - Easy define new `LogXXX` log category
     - Compile time eliminate unwanted logging levels based on settings made in each log category
   - Implement UE5 like log device redirector
     - Using `spdlog` as one of the log device to pump log into various sinks (e.g. files, console)
     - Pump `spdlog`'s `trace debug and infor` into async thread so that these logs won't block
     - `warn, err and critical` log are sync flushed
 - Custom memory management withe heap and stack allocation 
   - Implement global mallocator interface class where mimalloc and stack allocator can override & exchange in runtime [link](Engine/Source/Common/Private/Core/Mallocator/Mallocator.cpp) 
     - Implement TLS `GMallocator` which use TLS mimallocator as default
     - Override global `new` and `delete` to use `GMallocator`
     - Combine with `Scoped variable` to swap in & out TLS `GMallocator` with temporal allocator
     - Optimized stack allocator and is 2/3 cost of time comparing to `mimalloc` in average case
   - Implement custom mimalloc-like VM mallocator  (WIP) 
     - the goal is to be thread-local and lock-free and replicate a simplified version of free list sharding (WIP)
     - Virtual memory arena [link](Engine/Source/Common/Public/Core/Mallocator/VMMallocator/VMArena.h)
      - Multiple Heap live inside a single virtual memory arena, heaps are used to large blocks allocation [link](Engine/Source/Common/Public/Core/Mallocator/VMMallocator/VMHeap.h)
      - small binned allocator designed to be fast and lock-free [link](Engine/Source/Common/Public/Core/Mallocator/VMMallocator/SmallBinnedMallocator.h)
 - Obfuscation techniques for sensitive strings [link](Engine/Source/Common/Public/Template/Obfuscate/MetaString.tpp)
   - Use `AdvoObfuscator` to obfuscate short strings
   - Implement simpler stack free obfuscation method for long string (e.g. RSA private keys)
 - Custom string handling
   - Use `chat8_t` as string char type, as it is compatible with `char` simply by reinterpreting cast
   - Implement UE5 like string handling, e.g. `TXT("...")` and `FString` type to replace `std::string`
   - Implement `FPath` which internally uses `boost::filesystem::path` and cached path fast hash
   - Stack allocated string, useful for critical error handling where dynamic allocation is prohibited (WIP)
   - Const String pooling like UE5 `FName` and `FText` (WIP)
 - Custom file system handling with packing support
   - Token file [link](Engine/Source/Common/Public/Platform/FileSystem/Packed/PackedToken.h)
     - describes the file structure (e.g. offset, size, compression, etc)
     - Store `size_t` hash instead of string path (smaller token file size but not able to search and parse file paths in runtime)
     - debug json file that describes the token file content in human-readable format
   - Container file [link](Engine/Source/Common/Public/Platform/FileSystem/Packed/PackedContainerFragment.h)
     - tightly packs the data files for memory mapping
     - Runtime fragment that loads a minimal 4MB of mapped file region per fragment
     - Proxy that load actual content using token entry and container fragment
 - Custom exception handling [link](Engine/Source/Common/Public/Core/Assert.h)
   - No-inline exception handling function (avoid lengthy inline assembly code for string formatting)
   - Using `backwardcpp` from debugging utilities to print stack trace
   - Using `spdlog` for logging and `fmt` to combine exception message, stacktrace and file & lino
 - Boost hashing functions [link](Engine/Source/Common/Public/Utility/Hash.h)
   - MD5 digest hash and converting to hex string
   - SHA1 digest hash and converting to hex string
 - Custom template meta-programming for common tasks [link](Engine/Source/Common/Public/Template/GlobalTemplate.tpp)
   - String obfuscation (e.g. in binary private key, password, etc)
   - Compile time regex matching `ctre` for assertion checkings
   - Scoped variable templates (e.g. scoped file handle, scoped timer, etc)
   - Optional object reference removal
   - Reference object removal
   - Non-null pointer template
   - Memory-alloc free `printf` template
 - Debugging utilities for Linux and Windows [link](Engine/Source/Common/Public/Platform/GenericPlatformDebuggerUtil.h)
   - Using `ptrace` under linux to determine if a debugger is present
   - Using `WinAPI` under windows to determine if a debugger is present
   - Using `backwardcpp` library to print stack trace, support user skipping `N` frames from bottom
 - Parallelism
   - Spin lock [link](Engine/Source/Common/Public/Core/Parallel/Lock.h)
     - Use `std::atomic_flag` which is mutex free
     - Use `_mm_pause` and thread yielding to avoid busy waiting
     - Deaklock timer and lock guard
   - Rival lock [link](Engine/Source/Common/Public/Core/Parallel/Lock.h)
     - Avoid using `spin lock` on cases where only reading and writing are enough are mutually exclusive
     - Use `rival lock` so that all reading are permissive and mutually exclusive to writing, and vice versa
   - Concurrent `SPSC/MPSC/MPMC` queue [link](Engine/Source/Common/Public/Core/Parallel/ConcurrentQueue.h)
     - Inspired from UE5 lock free (i.e. compare swap exchange)`TQueue` implementation but extend to MPMC
     - 1.5x performance of Boost `lockfree::queue` (See test case TestParallel) 
     - Found out that use `cv` and `std::mutex` to block on poping (if empty) actually increase performance
   - Work Steal thread pool [link](Engine/Source/Common/Public/Core/Parallel/Async/WorkStealThreadPool.h)
     - Use a pool of `std::thread` and return `std::future` to use
     - Use `Concurrent MPMC queue` (with block on empty feat) to store tasks internally
     - Implement `work stealing` algorithm to get tasks efficiently to an idle worker
     - Implement various thread affinity mask to customize threading mode based on hardware and platform
   - Fiber pool (Not work so well) [link](Engine/Source/Common/Public/Core/Parallel/Async/WorkStealFiberPool.h)
     - Using `boost::fibers` as base to implement fiber pool, though its a mess inside (should consider native async scheduling?)
     - Scheduling method `work stealing` and launch method `post` does not work all the time (might stuck for no apparent reason)
     - Scheduling method `shared` and launch method `dispatch` generally work but fiber pool still blocks on `round robin` when running low threads
     - Fiber pool generally require more threads to run to achieve high performance than a thread pool

### Spcial Thanks, Credits, 3rdParty

- [AdvoObfuscator](https://github.com/andrivet/ADVobfuscator)
- fetch_packages:
    - yalantinlibs
    - backward
    - parallel_hashmap
    - ctre
    - string_pool

- find_packages:
    - spdlog
    - mimalloc
    - magic_enum
    - Boost
    - botan3
    - zstd
    - rapidjson
    - opentelemetry
    - protobuf
    - grpc
    - curl
    - nlohmann_json

```
```

```
Tool: read

**Input:**
```json
{
  "filePath": "/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/TODO.md"
}
```
**Output:**
```
<file>
00001| 2.1 Implement IDE independent build script mimicing CLion tool chains
00002|     - [x] Tool chain env path scripts
00003|     - [x] Cmake build directory scripts
00004|     - [x] Cmake build target scripts
00005|     - [x] Execute test scripts
00006| 3, bundle cmake, ninja, clang, clang++, and cmake command for IDE independent build process for linux-x64
00007|     - [x] Determine all tool chain binary, and copy to bundle location
00008|     - [x] Adjust previous step env path
00009| 
00010|  --6, Filehandle refactory,--
00011|  - [x] Packed file use offset to open packed container file in chunks, and manage loading/freeing chunks smartly behind the scene
00012|  - [x] Refactor boost file handle into streamed file handle and mapped file handle
00013| 
00014|  7, FPath platform type, local or packed
00015|  - [?] Check packed "file exists", "directory exists", "find all" implementation
00016|     - in order to do that, we actually need to use the token debug json as token file, instead of packed token
00017|  - [x] Packed entry handle refer counting opened entries (add packed file handle pointer in the quick find struct?)
00018| 
00019|  8, Allocator
00020|  - [x] Stack Allocator (with Mimalloc fallback on overflow allocation)
00021|  - [ ] Portable VMA/Heap/SmallBinned mallocator (MT free list sharding left)
00022|     - [x] Per thread fixed size free lists, if pull,
00023|     - [x] Otherwise, push pointer to global concurrent free list to be free-ed asynchronusly
00024|       - [ ] Async thread job that find correct VM thread and push free pointer to that VM
00025|     - [x] Free list sharding for VMHeap free to avoid excessive while loop looking up for the correct insertion location
00026|     - [ ] Enlarge binned allocation mechanism to bin size beyond 256 bytes, e.g. 128 KB max size or even greater
00027|     - [ ] Implement underlying OSPage allocator that similar to smallbin allocator
00028|           Using 32 bit to represent free bins but instead allocate 32MB per bin and there is no bin head
00029|  - [x] Integrate mimalloc
00030|  - [x] Refactor mallocator with GFallBackMallocator to allocate/free when Gmallocator fail
00031| 
00032|  9, Lock
00033|  - [x] atomic_flag spin lock
00034|  - [x] recusrive lock spin lock
00035|  - [x] rival lock spin lock
00036| 
00037|  10, WorkSteal Thread Pool--
00038|  - [x] Support adding task to thread pool, independent threads
00039|  - [x] Support thread pool worker thread priority
00040|  - [x] Support all three types of three affinity setting mode
00041|  - [x] Support various task priority
00042|  - [?] Support thread id recursive-collision test(i.e. thread that enqueue-ed task should not come from any known worker thread)
00043|  - [x] Async interface, UE5 like, async::launch
00044| 
00045|  11, [x] FName (a String type in UE5 that points to a string object)
00046| 
00047|  12, [ ] Integrate bullet3 cpp module
00048| 
00049|  13, [ ] Integrate Vulkan scene graph for easy GUI/game scene framework
00050| 
00051|  14, Assertion
00052|   - [x] replace assert with ensure
00053|   - [x] Use stack allocator during assertion handling to avoid malloc errors
00054| 
00055|  15, Integrate Catch2 test framework, while keeping our custom test framework (as my work done on those is not trivial)
00056|   - [x] catch2 is too slow in linking, so we better just scoop its idea
00057|     - [x] section macro lambda method with copy capture
00058|     - [x] reproducible random seed
00059| 
00060|  16, Integrate taskflow system
00061|   - [x] Use taskflow as front end and our work stead thread pool as backend
00062| 
00063|  17, Integrate profiler toolset: gperf, minitrace, tracy
00064|   - [x] gperf lib integration
00065|   - [x] gperf auto test and profile script
00066|   - [ ] minitrace lib integration
00067|   - [ ] tracy integration
00068|     - [x] tracy cpu event profiler
00069|     - [x] tracy cpu memory profiler
00070|     - [ ] tracy gpu event profiler (need wait until we starting working on vulkan stuff)
00071|     - [ ] tracy gpu memory profiler
00072|     - [ ] Add tracy user manul pdf to document
00073| 
00074|  18, Consider cpp->c# binding (cppsharp) and lua->c# binding (unlua)
00075|   - [/] Nope, instead we should consider lua + cpp bindings
00076| 
00077|  19, Integrate cpp anti debugger
00078|   - [/] https://github.com/BaumFX/cpp-anti-debug (win64 only, and some techniques are outdated)
00079| 
00080|  20, Improve stack trace and signal handler
00081|   - [x] Boost signal handler and stack trace dump https://stackoverflow.com/a/54427899/6658943
00082|   - [/] Integrate backward cpp-- (canceled due to backward cpp really lack details)
00083|   - [/] Integrate cpptrace-- (canceled due to cpptrace has heavy memory footprint in demangling stack trace)
00084| 
00085|  21, Concurrent queue
00086|   - [x] Use pmr mallocator instead of new and delete
00087| 
00088|  22, Luajit
00089|   - [/] Cmake build tool for tarantool cmake lacks install process (canceled due to tarantool luajit is hard to integrate)
00090|   - [x] Integrate vcpkg luajit
00091|    - [x] Implement more luajit tests (from https://github.com/openresty/luajit2-test-suite)
00092|      - [ ] Add Setup.sh script to build openresty clib for multi platform
00093| 
00094|  23, TSAN
00095|   - [x] Add tasn compile and link target
00096|   - [x] Refactor tsan switch into the cmakepy options (tsan mightbe too slow, and it does not spot mt problem right away)
00097| 
00098|  24, Sol2
00099|   - [x] Integrate vcpkg sol2
00100|   - [x] Test sol2 integration ability with reference count objects
00101|   - [x] Figure out different safety/feat config for sol2
00102|   - [ ] Implement UE4 like UBT cpp file preprocessor (another py package) that generate static reflections for cpp and for lua
00103|   - [ ] EFFIL lua native multithreading integration (better just copy its structure and idea but implement on our own)
00104| 
00105|  25, Refactor Build.sh and Test.sh
00106|   - [x] Build.sh
00107|     - [x] to build specific config only (i.e. --config=Release,Debug,RelWithDeb),
00108|     - [x] and test flag to enable test after build (i.e. --test)
00109|     - [x] Specific targets (i.e. --target=TestMallocator)
00110|   - [\] Test.sh (canceled because we have integrate test.sh into build.sh)
00111|     - [\] script to test specific target (i.e. --target=TestMallocator)
00112|     - [\] to enable gperf on specific target (i.e. --gperf)
00113| 
00114|  26, pybind11 or boost.python
00115|   - [ ] pybind11 test case
00116| 
00117|  27, dynamic link libcommon.so with executable and fix new/delete overriding issues
00118|   - [x] the objective is achieve same build types (shared lib) for windows/linux
00119|         vcpkg build windows lib with dll by default but for linux, vcpkg build static
00120|         lib by default (due to unix/linux ), fortunately, vcpkg applies -fpic flag to static lib
00121|         so that they can link to .so w/o much issue 
00122|         https://github.com/microsoft/vcpkg/blob/b1e352c60e706098ac3b5421856ae012eeeb832e/scripts/toolchains/linux.cmake#L39-L40
00123|   - [x] cmakepy support build shared lib
00124|   - [ ] fix new/delete overriding not working for pool test #1
00125| 
00126|  28, Read the Doc documentation
00127|   - [ ] Build docs in document folder
00128| 
00129|  29, Debuggable serialization sytstem
00130|   - [ ] Even though integrated Yalantinlib, but not quite customizable
00131|   - [ ] Consider copy UE's Archive system but improve with debuggable
00132| 
00133|  30, PyCmake
00134|   - [ ] Finish Common and runtime project setup (TODO, moving vulkan/glfw/glm stuff to runtime only)
00135|   - [ ] Fix Py cmake bug if there is any left

(End of file - total 135 lines)
</file>
```
```

```
Tool: bash

**Input:**
```json
{
  "command": "ls -la Engine/Source/Runtime/",
  "description": "List runtime modules"
}
```
**Output:**
```
total 3160
drwxrwxr-x 9 hangyu5 hangyu5   4096 Feb 16 10:49 .
drwxrwxr-x 7 hangyu5 hangyu5   4096 Jan 23  2025 ..
-rw-rw-r-- 1 hangyu5 hangyu5   1452 Feb 13  2024 .clang-format
-rw-rw-r-- 1 hangyu5 hangyu5    751 Jan 26  2024 .clang-tidy
drwxrwxr-x 3 hangyu5 hangyu5   4096 Feb 18 00:09 .idea
drwxrwxr-x 5 hangyu5 hangyu5   4096 Feb  7 18:42 Binary
drwxrwxr-x 5 hangyu5 hangyu5   4096 Feb  7 18:41 Build
-rwxrwxr-x 1 hangyu5 hangyu5  11022 Feb  7 11:07 Build.sh
-rw-rw-r-- 1 hangyu5 hangyu5  12448 Feb 17 13:27 CMakeLists.txt
-rw-rw-r-- 1 hangyu5 hangyu5  11825 Feb  8 14:12 CMakeLists_vk.txt
-rwxrwxr-x 1 hangyu5 hangyu5     83 Jan 22  2025 Clean.sh
drwxrwxr-x 3 hangyu5 hangyu5   4096 Feb 16 10:17 Private
drwxrwxr-x 4 hangyu5 hangyu5   4096 Feb 16 10:16 Public
-rw-rw-r-- 1 hangyu5 hangyu5  13976 Feb  8 14:29 Runtime_cmake.py
-rwxrwxr-x 1 hangyu5 hangyu5    571 Feb  8 11:49 Setup.sh
drwxrwxr-x 5 hangyu5 hangyu5   4096 Feb 16 12:01 Test
drwxrwxr-x 2 hangyu5 hangyu5   4096 Feb 16 12:09 Testing
-rw-rw-r-- 1 hangyu5 hangyu5 339125 Feb 16 22:09 build_Debug.log
-rw-rw-r-- 1 hangyu5 hangyu5 540744 Feb 16 22:10 build_RelWithDebInfo.log
-rw-rw-r-- 1 hangyu5 hangyu5 522150 Feb 16 22:10 build_Release.log
-rw-rw-r-- 1 hangyu5 hangyu5 588542 Feb  7 12:29 rebuild_Debug.log
-rw-rw-r-- 1 hangyu5 hangyu5 577025 Feb  7 12:30 rebuild_RelWithDebInfo.log
-rw-rw-r-- 1 hangyu5 hangyu5 543514 Feb  7 12:31 rebuild_Release.log
-rw-rw-r-- 1 hangyu5 hangyu5    996 Feb 17 13:27 vcpkg.json

```
```

```
Tool: bash

**Input:**
```json
{
  "command": "ls -la Engine/Source/",
  "description": "List engine source structure"
}
```
**Output:**
```
total 36
drwxrwxr-x  7 hangyu5 hangyu5  4096 Jan 23  2025 .
drwxrwxr-x  6 hangyu5 hangyu5  4096 Feb  7 17:15 ..
drwxrwxr-x 12 hangyu5 hangyu5 12288 Feb  8 10:34 Common
drwxrwxr-x  3 hangyu5 hangyu5  4096 May 27  2024 Dependency
drwxrwxr-x  2 hangyu5 hangyu5  4096 Jan 25  2024 Editor
drwxrwxr-x  2 hangyu5 hangyu5  4096 Jan 25  2024 Plugin
drwxrwxr-x  9 hangyu5 hangyu5  4096 Feb 16 10:49 Runtime

```
```

---

## Assistant (Sisyphus · qwen3.5-plus-2026-02-15 · 18.6s)

## HLVM-Engine Repository Overview

**HLVM-Engine** is a personal game engine practice project inspired by Unreal Engine, focused on building core engine infrastructure with improved pak file handling, memory management, and telemetry systems.

### Key Focus Areas
- **NOT** focused on rendering/animation/VFX pipelines
- Core infrastructure: memory allocators, file systems, parallelism, logging
- Linux development toolchain and C++20 features
- Better UE5-style pak file reading and memory management

### Repository Structure

```
HLVM-Engine/
├── Engine/
│   ├── Source/
│   │   ├── Common/        # Shared utilities (logging, memory, parallelism, etc.)
│   │   ├── Runtime/       # Engine runtime modules
│   │   ├── Editor/        # Editor code
│   │   ├── Plugin/        # Plugin system
│   │   └── Dependency/    # External dependencies
│   ├── Scripts/           # Build scripts (PyCMake, Redis, pyarg)
│   └── Binary/            # Compiled binaries
├── Samples/               # Sample projects
├── Document/              # Documentation (Doxygen, Sphinx)
├── Binary/                # Toolchain bundles (CMake, Ninja, Clang)
└── Vibe_Coding/           # Experimental/exploratory code
```

### Core Features

**Build System:**
- Custom `PyCMake` Python package that generates CMake projects
- CTest integration for unit testing
- GitHub Actions CI/CD (WIP)

**Systems Implemented:**
- **Memory Management**: Custom allocators (mimalloc integration, stack allocator, VM allocator WIP)
- **File System**: Boost-based file handling with packed container support (token + container files)
- **Parallelism**: Spin locks, reader-writer locks, lock-free queues (SPSC/MPSC/MPMC), work-stealing thread pool, fiber pool
- **Logging**: UE5-style logging macros with spdlog backend, async log pumping
- **Compression**: Zstd wrapper
- **Encryption**: Botan3 RSA OAEP encryption and EMSA signatures
- **String Handling**: Custom `FString`, `FPath`, FName-like string pooling (WIP)
- **Debugging**: Cross-platform debugger detection, stack traces with backward-cpp
- **Obfuscation**: Compile-time string obfuscation for sensitive data

### Tech Stack
- **Languages**: C++20, Python (build system)
- **Compilers**: Clang-16+, CMake 3.28+
- **Key Dependencies**: Boost, spdlog, mimalloc, Botan3, Zstd, RapidJSON, gRPC, OpenTelemetry

### Current Status (from TODO)
- ✅ Work-stealing thread pool with priority and affinity
- ✅ Lock-free concurrent queues
- ✅ Stack allocator with mimalloc fallback
- ✅ Tracy profiler integration (CPU events/memory)
- 🔄 VM allocator (free list sharding in progress)
- 🔄 Vulkan scene graph (planned)
- 🔄 Lua integration via sol2 (partial)

**Remote**: https://github.com/yhyu13/HLVM-Engine.git

---

