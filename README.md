# os-simulation
大三操作系统课设

## 项目说明

这是一个操作系统课程设计项目，旨在模拟操作系统的核心功能。项目采用模块化设计，由小组成员分工协作完成，每个成员负责一个核心模块的开发。

**项目模块：**
- **进程调度模块 (scheduler/)** - 沈卓负责，实现进程调度算法（如 FCFS、RR 等）
- **内存管理模块 (memory_manager/)** - 孙煜彬负责，实现内存分配与管理功能
- **存储管理模块 (storage/)** - 许欣宇负责，实现文件系统和存储管理功能
- **同步机制模块 (sync/)** - 黄海喆负责，实现进程同步与互斥机制

所有模块通过 `main.cpp` 统一整合，形成一个完整的操作系统模拟系统。

## 项目结构

```
os-simulation/           # main 分支
├── main.cpp             # 程序入口，整合所有模块
├── README.md            # 项目说明文档
├── scheduler/           # 进程调度模块（沈卓）
│   ├── scheduler.h      # 调度器头文件
│   └── scheduler.cpp    # 调度器实现
├── memory_manager/      # 内存管理模块（孙煜彬）
│   └── ...
├── storage/             # 存储管理模块（许欣宇）
│   └── ...
└── sync/                # 同步机制模块（黄海喆）
    └── ...
```

## 开发说明

本项目采用分支协作开发模式：
- 每个成员在自己的分支上开发对应的模块
- 完成开发后通过 Pull Request 合并到 main 分支
- main 分支始终保持完整的、可运行的项目代码

## 开发环境配置

### 1. 安装 C++ 编译器

**Windows 系统推荐以下方式之一：**

- **方式一：MinGW-w64（推荐）**
  - 下载地址：https://www.mingw-w64.org/downloads/
  - 或者使用 MSYS2：https://www.msys2.org/
  - 安装后将 `bin` 目录添加到系统 PATH 环境变量

- **方式二：Visual Studio**
  - 安装 Visual Studio 并选择 C++ 工作负载
  - 使用 Visual Studio 自带的 MSVC 编译器

### 2. 配置 VS Code

由于每个人的开发环境不同，`.vscode` 目录已被忽略，需要自己配置：

1. **创建 `.vscode/tasks.json`**（用于编译）：
```json
{
  "version": "2.0.0",
  "tasks": [
      {
          "type": "shell",
          "label": "g++ build active file",
          "command": "g++",  // 如果 g++ 在 PATH 中，直接写 "g++"，否则写完整路径
          "args": [
              "-g",
              "-std=c++11",
              "${fileDirname}\\*.cpp",
              "-o",
              "${fileDirname}\\${fileBasenameNoExtension}.exe"
          ],
          "options": {
              "cwd": "${fileDirname}"
          },
          "problemMatcher": [
              "$gcc"
          ],
          "group": {
              "kind": "build",
              "isDefault": true
          }
      }
  ]
}
```

2. **创建 `.vscode/c_cpp_properties.json`**（用于代码提示）：
```json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "compilerPath": "g++",  // 改为你自己的编译器路径
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-gcc-x64"
        }
    ],
    "version": 4
}
```

**注意：** 将 `"g++"` 替换为你电脑上实际的编译器路径，例如：
- MinGW: `C:/msys64/mingw64/bin/g++.exe`
- Visual Studio: `C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.xx.xxxxx/bin/Hostx64/x64/cl.exe`

### 3. 手动编译运行

如果不想配置 VS Code，也可以直接在终端手动编译：

```bash
# 编译完整项目（所有模块）
g++ -std=c++11 main.cpp scheduler/*.cpp memory_manager/*.cpp storage/*.cpp sync/*.cpp -o os-simulation.exe

# 运行
./os-simulation.exe
```

**注意：** 如果某个模块还未完成，可以暂时注释掉相关代码，或使用条件编译来处理。

## 使用方法

1. 确保所有模块代码已合并到 main 分支
2. 编译主程序（参考上方编译命令）
3. 运行生成的可执行文件