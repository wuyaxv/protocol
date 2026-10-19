# protocol (C++ edition)

```
                  ____            _                  _
                 |  _ \ _ __ ___ | |_ ___   ___ ___ | |
                 | |_) | '__/ _ \| __/ _ \ / __/ _ \| |
                 |  __/| | | (_) | || (_) | (_| (_) | |
                 |_|   |_|  \___/ \__\___/ \___\___/|_|
```

A command-line tool that draws **network protocol header diagrams**. This is a
C++ rewrite of the original Python tool
[protocol](https://github.com/luismartingarcia/protocol) by Luis MartinGarcia,
extended with:

* **PNG image export** — crisp pixel-art diagrams, no external dependencies
* **SVG image export** — clean vector diagrams for documents and slides
* **12 color themes** — for images *and* terminal output (ANSI truecolor)
* Multiple protocols can be stacked into a single image

The ASCII output is byte-for-byte compatible with the original Python tool
(verified by the regression tests in `tests/`).

[中文文档请见下方](#中文文档)

![TCP header, light theme](examples/tcp_light.png)

## Table of contents

- [protocol (C++ edition)](#protocol-c-edition)
  - [Table of contents](#table-of-contents)
  - [Features](#features)
  - [Building](#building)
    - [Prerequisites](#prerequisites)
    - [Quick build](#quick-build)
    - [macOS](#macos)
    - [Debian / Ubuntu](#debian--ubuntu)
    - [Fedora / RHEL / CentOS](#fedora--rhel--centos)
    - [Arch Linux](#arch-linux)
    - [Windows](#windows)
    - [Build types](#build-types)
    - [Installing](#installing)
    - [Building without CMake](#building-without-cmake)
    - [Verifying the build](#verifying-the-build)
  - [Usage](#usage)
    - [Command-line syntax](#command-line-syntax)
    - [Option reference](#option-reference)
    - [Displaying built-in protocols](#displaying-built-in-protocols)
    - [Built-in protocols](#built-in-protocols)
    - [Custom protocol specifications](#custom-protocol-specifications)
    - [Reading specs from a file](#reading-specs-from-a-file)
    - [Output formats](#output-formats)
    - [Color themes](#color-themes)
    - [Exit status](#exit-status)
  - [Examples](#examples)
  - [Project layout](#project-layout)
  - [Testing](#testing)
  - [Third-party components](#third-party-components)
  - [License](#license)
- [中文文档](#中文文档)
  - [目录](#目录)
  - [功能特性](#功能特性)
  - [编译安装](#编译安装)
    - [环境要求](#环境要求)
    - [快速构建（Unix 通用）](#快速构建unix-通用)
    - [macOS](#macos-1)
    - [Debian / Ubuntu](#debian--ubuntu-1)
    - [Fedora / RHEL / CentOS](#fedora--rhel--centos-1)
    - [Arch Linux](#arch-linux-1)
    - [Windows](#windows-1)
    - [构建类型](#构建类型)
    - [安装到系统](#安装到系统)
    - [不用 CMake 直接编译](#不用-cmake-直接编译)
    - [验证构建](#验证构建)
  - [使用方法](#使用方法)
    - [命令语法](#命令语法)
    - [选项一览](#选项一览)
    - [显示内置协议](#显示内置协议)
    - [内置协议列表](#内置协议列表)
    - [自定义协议规格](#自定义协议规格)
    - [从文件读取规格](#从文件读取规格)
    - [输出格式](#输出格式)
    - [颜色主题](#颜色主题)
    - [退出状态码](#退出状态码)
  - [命令示例](#命令示例)
  - [许可证](#许可证)

## Features

* Print RFC-like ASCII diagrams of the most common protocol headers directly
  in your terminal — 40 built-in protocols (Ethernet, 802.1q, TCP, UDP, IPv4,
  IPv6, ICMP, ICMPv6, DHCP, Modbus-TCP, Profinet-RT, DNP3, TSAP, COTP, S7, ...).
* Generate diagrams for your **own protocols** with a compact one-line spec
  syntax: `"Type:8,Code:8,Checksum:16,Payload:64"`.
* Fields of any bit length; fields that do not fit on one line are wrapped
  automatically with merged border lines, just like in RFC diagrams.
* Configurable bits per line, optional bit-number rows, custom border
  characters.
* Three output formats: **text** (plain or ANSI-colored), **SVG**, **PNG**.
* **Zero external dependencies**: the PNG encoder (stb_image_write) and the
  8x8 bitmap font are embedded in the repository (both public domain). Only a
  C++17 compiler and CMake are needed.

## Building

### Prerequisites

| Requirement | Notes |
|---|---|
| C++17 compiler | GCC ≥ 7, Clang ≥ 5, Apple Clang, MSVC ≥ 2017 |
| CMake ≥ 3.16 | Only for the standard build; you can also [compile directly](#building-without-cmake) |

There are **no library dependencies** (no libpng, no zlib, nothing else) —
everything needed ships in this repository.

### Quick build

On any Unix-like system:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The binary is created at `build/protocol`. Run it:

```sh
./build/protocol tcp
```

### macOS

```sh
# Install the toolchain if needed:
xcode-select --install          # Apple Clang
brew install cmake              # CMake (via Homebrew)

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Debian / Ubuntu

```sh
sudo apt update
sudo apt install build-essential cmake

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Fedora / RHEL / CentOS

```sh
sudo dnf install gcc-c++ cmake

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Arch Linux

```sh
sudo pacman -S base-devel cmake

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Windows

**With MSVC (Visual Studio 2017 or later, CMake included):**

```bat
cmake -B build
cmake --build build --config Release
```

The executable is `build\Release\protocol.exe`.

**With MinGW-w64:**

```bat
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**With clang-cl / Ninja** also works out of the box. Note that colored
terminal output (`--color`) requires a terminal with ANSI support, e.g.
Windows Terminal.

### Build types

* `Release` (default): optimized (`-O2`-level) — recommended.
* `Debug`: no optimizations, debug symbols, useful for development.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Installing

To install system-wide (default prefix `/usr/local`):

```sh
sudo cmake --install build
```

To install into a custom prefix:

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build
cmake --install build        # binary goes to $HOME/.local/bin
```

### Building without CMake

The project is a small set of translation units, so a single compiler
invocation is enough:

```sh
# GCC or Clang on Linux / macOS
c++ -std=c++17 -O2 -fno-strict-aliasing -Isrc -Ithird_party \
    src/main.cpp src/protocol.cpp src/specs.cpp src/theme.cpp \
    src/render_text.cpp src/render_svg.cpp src/render_png.cpp \
    -o protocol
```

(`-fno-strict-aliasing` is a safety recommendation from the stb_image_write
documentation; the CMake build applies it to that file only.)

### Verifying the build

Run the regression test suite (compares the output against golden files
produced by the original Python implementation):

```sh
./tests/run_tests.sh
# or through CTest:
ctest --test-dir build --output-on-failure
```

Expected result: `passed: 55, failed: 0`.

## Usage

### Command-line syntax

```
protocol {<protocol> or <spec>} [OPTIONS]
```

* `<protocol>` — the name of a built-in protocol (see `protocol --list`).
* `<spec>` — a field-by-field specification of a custom protocol, e.g.
  `"Type:8,Code:8,Checksum:16"`. Any argument containing `:` is treated as a
  spec.

You may pass several protocols/specs at once; they are rendered one after
another (stacked into a single canvas for image formats).

### Option reference

| Option | Description |
|---|---|
| **Input** | |
| `-f, --file <file>` | Read protocol specs from a text file (one per line) |
| **Diagram layout** | |
| `-b, --bits <n>` | Number of bits per line (default: 32, per-spec `?bits=` overrides, this option overrides everything) |
| `-n, --no-numbers` | Do not print the bit-number rows on top of the header |
| `--evenchar <c>` | Character for the even positions of horizontal borders (default: `-`) |
| `--oddchar <c>` | Character for the odd positions of horizontal borders (default: `+`) |
| `--startchar <c>` | Character that starts horizontal borders (default: `+`) |
| `--endchar <c>` | Character that ends horizontal borders (default: `+`) |
| `--sepchar <c>` | Character that separates protocol fields (default: `\|`) |
| **Output** | |
| `-o, --output <file>` | Write output to a file; the format is taken from the extension (`.txt`, `.svg`, `.png`) unless `--format` is given |
| `--format <fmt>` | Output format: `text`, `svg` or `png` (default: `text`) |
| **Color** | |
| `-t, --theme <name>` | Color theme (default: `none` for text output, `light` for svg/png); see `protocol --list-themes` |
| `--color <mode>` | ANSI colors for terminal text output: `auto` (only when stdout is a TTY), `always`, `never` (default: `auto`) |
| **PNG / SVG** | |
| `--scale <n>` | PNG pixel scale, integer 1–8 (default: 2; one character cell = 8x12 font pixels x scale) |
| `--transparent` | Transparent canvas background for PNG/SVG output |
| **Information** | |
| `--list` | List the built-in protocols |
| `--list-themes` | List the available color themes |
| `-h, --help` | Display help |
| `-V, --version` | Display version |

### Displaying built-in protocols

```sh
protocol tcp
protocol tcp udp          # several at once
protocol dot1q ethernet
```

Protocol names don't need to be fully specified as long as there is no
ambiguity — `protocol et` is equivalent to `protocol ethernet`. If the
prefix is ambiguous, the candidates are listed:

```
$ protocol icmpv6-r
Ambiguous protocol specifier 'icmpv6-r'. Did you mean any of these?
  icmpv6-radv
  icmpv6-redirect
  icmpv6-rsol
```

### Built-in protocols

| Name | Description |
|---|---|
| `ethernet` | Ethernet |
| `8021q`, `dot1q` | IEEE 802.1q VLAN tag |
| `tcp` | Transmission Control Protocol |
| `udp` | User Datagram Protocol |
| `ip` | Internet Protocol, version 4 |
| `ipv6` | Internet Protocol, version 6 |
| `icmp` | Internet Control Message Protocol (ICMP) |
| `icmp-destination` | ICMP Destination Unreachable |
| `icmp-time` | ICMP Time Exceeded |
| `icmp-parameter` | ICMP Parameter Problem |
| `icmp-source` | ICMP Source Quench |
| `icmp-redirect` | ICMP Redirect |
| `icmp-echo` | ICMP Echo Request/Reply |
| `icmp-timestamp` | ICMP Timestamp Request/Reply |
| `icmp-information` | ICMP Information Request/Reply |
| `icmpv6` | ICMP for IPv6 (ICMPv6) |
| `icmpv6-destination` | ICMPv6 Destination Unreachable |
| `icmpv6-big` | ICMPv6 Packet Too Big |
| `icmpv6-time` | ICMPv6 Time Exceeded |
| `icmpv6-parameter` | ICMPv6 Parameter Problem |
| `icmpv6-echo` | ICMPv6 Echo Request/Reply |
| `icmpv6-rsol` | ICMPv6 Router Solicitation |
| `icmpv6-radv` | ICMPv6 Router Advertisement |
| `icmpv6-nsol` | ICMPv6 Neighbor Solicitation |
| `icmpv6-nadv` | ICMPv6 Neighbor Advertisement |
| `icmpv6-redirect` | ICMPv6 Redirect |
| `dhcp` | Dynamic Host Configuration Protocol |
| `modbus_tcp` | Modbus-TCP |
| `profinet_rt` | Profinet-RT |
| `dnp3` | DNP3 |
| `tsap` | TSAP |
| `cotp_cr` | COTP Connection Request |
| `cotp_dt` | COTP Data |
| `cotp_dr` | COTP Disconnect Request |
| `s7_header` | S7 communication header |
| `s7_item` | S7 communication item |
| `s7_data` | S7 communication data |
| `example` | Demo spec using custom border characters |
| `test` | Stress-test spec (fields of 1..127 bits) |

### Custom protocol specifications

A spec has the form `"<FIELD_LIST>[?OPTIONS]"`:

* **FIELD_LIST** — a comma-separated list of `name:length` tuples. `name` is
  the field label (it may contain any character except `?`, `,` and `:`;
  quote the whole spec in your shell if it contains spaces). `length` is an
  integer > 0: the field width in bits. Do **not** put whitespace after the
  commas.
* **OPTIONS** — an optional comma-separated list of `key=value` modifiers,
  introduced by a `?`.

Examples of field lists:

```
Type:8,Code:8,Checksum:16,Message Body:64
Source Port:16,Destination Port:16,Length:16,Checksum:16
```

Field lengths don't need to align to the line width; long fields wrap to the
next line automatically:

```
$ protocol "Source:16,TTL:8,Reserved:40"
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             Source            |      TTL      |               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               +
|                            Reserved                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

Available per-spec options:

| Option | Values | Description |
|---|---|---|
| `bits=<n>` | integer > 0 | Bits per line (default: 32, same as IETF) |
| `numbers=<y/n>` | `1 y yes true` / `0 n no false` | Print or hide the bit-number rows |
| `evenchar=<c>` | single char | Character for even positions of horizontal borders (default `-`) |
| `oddchar=<c>` | single char | Character for odd positions of horizontal borders (default `+`) |
| `startchar=<c>` | single char | First character of horizontal borders (default `+`) |
| `endchar=<c>` | single char | Last character of horizontal borders (default `+`) |
| `sepchar=<c>` | single char | Field separator character (default `\|`) |

The following diagram shows where each character modifier applies:

```
   Bit counts                                                     sepchar(|)
     ^   ^                                                              |
     |   |                                                              |
     0   |               1                   2                   3      |
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   |
    |             Source            |      TTL      |    Reserved   | <-+
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ^              ^ ^ ^              ^ ^ ^                         ^
    |              | | |              | | |                         |
 startchar(+)     evenchar(-)        oddchar(+)                  endchar(+)

    |<------------------------------------------------------------->|
                               bits(32)
```

### Reading specs from a file

`-f/--file` reads specs from a text file: one spec per line, empty lines are
ignored and lines starting with `#` are comments.

```
# my_protocols.txt — custom protocol collection
Type:8,Code:8,Checksum:16,Message Body:64
Length:16,Checksum:16?bits=16,numbers=n
```

```sh
protocol -f my_protocols.txt                # print both as ASCII
protocol -f my_protocols.txt -o all.svg     # stack both into one SVG
```

### Output formats

**Text (default).** The classic RFC-like ASCII diagram. With no `-o`, it goes
to standard output; with `-o file.txt` it is written to the file. ANSI colors
are applied when a non-`none` theme is selected *and* color is enabled
(`--color auto` enables colors only if stdout is a terminal). Files never get
escape sequences.

**SVG.** Vector output, ideal for documentation and slides. Field backgrounds
are drawn as rectangles, borders as continuous lines, labels as real text
(monospace web-font stack). Write it to a file with `-o file.svg` or pipe it
from stdout (`--format svg`). `--transparent` omits the background rectangle.

**PNG.** Bitmap output rasterized with an embedded 8x8 pixel font (pixel-art
look). `--scale <n>` (1–8, default 2) multiplies the pixel size: at scale 2
one character cell is 16x24 px. `--transparent` gives a transparent canvas
(field fills and glyphs stay opaque). Encoding is done by the bundled
stb_image_write — no libpng needed.

Format selection order: `--format` wins over the `-o` file extension; without
either, the format is `text`.

### Color themes

```
$ protocol --list-themes
```

| Theme | Look |
|---|---|
| `none` | Plain black on white (classic RFC look, no field colors) |
| `rfc` | White background, light gray alternation |
| `light` | Light background with pastel field colors (default for images) |
| `dark` | Dark background with vibrant accents |
| `solarized` | Solarized light palette |
| `solarized-dark` | Solarized dark palette |
| `monokai` | Monokai editor palette |
| `ocean` | Deep ocean blues |
| `forest` | Forest greens |
| `sunset` | Warm sunset tones on a light background |
| `candy` | Bright candy colors |
| `grayscale` | Grayscale alternation, print friendly |

Fields are colored cyclically from the theme palette so neighbouring fields
always differ; a field spanning several lines keeps a single color. Themes
apply to all three output formats (text via ANSI truecolor escapes).

| `light` | `dark` | `ocean` |
|---|---|---|
| ![light](examples/tcp_light.png) | ![dark](examples/stack_dark.png) | ![ocean](examples/ipv6_ocean.png) |

### Exit status

* `0` — success
* `1` — any error (invalid spec, unknown protocol/theme/option, unreadable
  file, unwritable output, ...). Error messages go to standard error.

## Examples

```sh
# 1. Classic ASCII diagram of the TCP header
protocol tcp

# 2. Several protocols at once
protocol ip tcp udp

# 3. Custom protocol from a one-line spec
protocol "Type:8,Code:8,Checksum:16,Payload:64"

# 4. Custom bits-per-line and hidden bit numbers
protocol "Length:16,Checksum:16?bits=16,numbers=n"
protocol tcp -b 16 -n

# 5. Custom border characters
protocol "Field4:4,Field4:4,Field8:8,Field16:16?bits=16,startchar=*,endchar=*"
protocol tcp --startchar '*' --endchar '*' --evenchar '=' --oddchar '-' --sepchar '!'

# 6. Colored terminal output (ANSI truecolor)
protocol ipv6 --theme ocean --color always

# 7. SVG image (default "light" theme)
protocol tcp -o tcp.svg

# 8. PNG image with theme and 3x scale
protocol tcp -o tcp.png --theme dark --scale 3

# 9. Several protocols stacked into one image
protocol ip tcp udp -o stack.svg --theme solarized
protocol ip tcp udp -o stack.png --theme dark

# 10. Transparent background PNG (e.g. for slides)
protocol icmp -o icmp.png --theme sunset --transparent

# 11. Write SVG / PNG to stdout (e.g. for pipelines)
protocol tcp --format svg > tcp.svg
protocol udp --format png > udp.png

# 12. Render specs from a file
protocol -f my_protocols.txt -o all.svg --theme forest

# 13. Short unique prefixes work
protocol et              # == protocol ethernet
protocol s7_h            # == protocol s7_header
```

## Project layout

```
protocol/
├── CMakeLists.txt          # CMake build definition (C++17, no dependencies)
├── README.md               # This file
├── LICENSE.txt             # GNU GPL v3
├── src/
│   ├── main.cpp            # Command-line interface
│   ├── protocol.hpp/.cpp   # Core layout engine (spec parsing, diagram grid)
│   ├── specs.hpp/.cpp      # The 40 built-in protocol specifications
│   ├── theme.hpp/.cpp      # Color theme definitions
│   ├── border_lines.hpp    # Border cells -> continuous line segments
│   ├── render_text.hpp/.cpp    # ASCII / ANSI truecolor renderer
│   ├── render_svg.hpp/.cpp     # SVG renderer
│   └── render_png.hpp/.cpp     # PNG renderer (embedded 8x8 bitmap font)
├── third_party/
│   ├── stb_image_write.h   # PNG encoder (public domain)
│   ├── font8x8_basic.h     # 8x8 bitmap font (public domain)
│   └── README.md           # Attribution & license notes
├── tests/
│   ├── run_tests.sh        # Regression test runner
│   └── golden/*.txt        # Golden ASCII outputs (original Python tool)
└── examples/               # Sample images generated with this tool
```

## Testing

```sh
./tests/run_tests.sh        # or: ctest --test-dir build --output-on-failure
```

The suite:

1. Compares the ASCII output of this C++ implementation against golden files
   generated with the original Python tool — all 40 built-in protocols plus
   edge cases (custom bits-per-line, custom border characters, fields spanning
   multiple lines, hidden bit numbers, multiple protocols at once).
2. Sanity-checks SVG output (well-formed document) and PNG output (magic
   bytes).
3. Checks that invalid specifications fail with exit status 1.

## Third-party components

Both bundled in `third_party/`, both public domain (see
`third_party/README.md`):

* **stb_image_write.h** — PNG encoder by Sean Barrett
  (<https://github.com/nothings/stb>).
* **font8x8_basic.h** — 8x8 bitmap font by Daniel Hepper, based on IBM's
  public-domain VGA fonts (<https://github.com/dhepper/font8x8>).

## License

GNU GPL v3 (see `LICENSE.txt`), same as the original project. Original work
(c) 2014 Luis MartinGarcia; this C++ rewrite is distributed under the same
license.

---

<a id="中文文档"></a>

# 中文文档

这是原 Python 项目 [protocol](https://github.com/luismartingarcia/protocol)
（作者 Luis MartinGarcia）的 **C++ 重写版**，用于在命令行生成 RFC 风格的网络协议头图表。
ASCII 输出与原版**逐字节一致**（`tests/` 中有回归测试保证），并新增了：

* **PNG 图片导出** —— 像素风位图，零外部依赖
* **SVG 图片导出** —— 适合文档与幻灯片的矢量图
* **12 种颜色主题** —— 图片和终端输出（ANSI 真彩色）均支持
* 多个协议可堆叠到同一张图片中

## 目录

* [功能特性](#功能特性)
* [编译安装](#编译安装)
* [使用方法](#使用方法)
* [命令示例](#命令示例)
* [项目结构](#项目结构)
* [测试](#测试)
* [许可证](#许可证)

## 功能特性

* 内置 **40 种常用协议**（Ethernet、802.1q、TCP、UDP、IPv4、IPv6、ICMP、ICMPv6、
  DHCP、Modbus-TCP、Profinet-RT、DNP3、TSAP、COTP、S7 等），直接在终端打印 RFC 风格图表。
* 用一行规格描述即可生成**自定义协议**图表：`"Type:8,Code:8,Checksum:16,Payload:64"`。
* 字段可为任意比特长度；放不下的字段自动折行，边框按 RFC 风格合并。
* 可配置每行比特数、可隐藏比特序号行、可自定义边框字符。
* 三种输出格式：**文本**（纯 ASCII 或 ANSI 彩色）、**SVG**、**PNG**。
* **零外部依赖**：PNG 编码器（stb_image_write）与 8x8 点阵字体已内嵌（均为 public domain），
  只需 C++17 编译器和 CMake。

## 编译安装

### 环境要求

| 依赖 | 说明 |
|---|---|
| C++17 编译器 | GCC ≥ 7、Clang ≥ 5、Apple Clang、MSVC ≥ 2017 |
| CMake ≥ 3.16 | 标准构建方式；也可以[不用 CMake 直接编译](#不用-cmake-直接编译) |

**无需任何第三方库**（不需要 libpng、zlib 等），一切依赖均已随仓库附带。

### 快速构建（Unix 通用）

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

生成的二进制文件为 `build/protocol`，运行：

```sh
./build/protocol tcp
```

### macOS

```sh
xcode-select --install          # 安装 Apple Clang（如未安装）
brew install cmake              # 通过 Homebrew 安装 CMake

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Debian / Ubuntu

```sh
sudo apt update
sudo apt install build-essential cmake

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Fedora / RHEL / CentOS

```sh
sudo dnf install gcc-c++ cmake

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Arch Linux

```sh
sudo pacman -S base-devel cmake

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Windows

**MSVC（Visual Studio 2017 及以上，自带 CMake）：**

```bat
cmake -B build
cmake --build build --config Release
```

可执行文件为 `build\Release\protocol.exe`。

**MinGW-w64：**

```bat
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

注意：终端彩色输出（`--color`）需要支持 ANSI 转义的终端（如 Windows Terminal）。

### 构建类型

* `Release`（默认）：优化构建，日常使用推荐。
* `Debug`：无优化、带调试符号，供开发调试。

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### 安装到系统

```sh
sudo cmake --install build                 # 默认安装到 /usr/local/bin
```

自定义安装前缀：

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build
cmake --install build                      # 安装到 $HOME/.local/bin
```

### 不用 CMake 直接编译

项目源文件很少，一条编译命令即可：

```sh
c++ -std=c++17 -O2 -fno-strict-aliasing -Isrc -Ithird_party \
    src/main.cpp src/protocol.cpp src/specs.cpp src/theme.cpp \
    src/render_text.cpp src/render_svg.cpp src/render_png.cpp \
    -o protocol
```

（`-fno-strict-aliasing` 是 stb_image_write 文档建议的安全选项；CMake 构建只对该文件应用。）

### 验证构建

```sh
./tests/run_tests.sh
# 或通过 CTest：
ctest --test-dir build --output-on-failure
```

预期结果：`passed: 55, failed: 0`。

## 使用方法

### 命令语法

```
protocol {<协议名> 或 <协议规格>} [选项]
```

* `<协议名>` —— 内置协议名称（见 `protocol --list`）。
* `<协议规格>` —— 自定义协议的逐字段描述，例如 `"Type:8,Code:8,Checksum:16"`。
  任何包含 `:` 的参数都会被当作规格解析。

可以一次传入多个协议/规格；文本模式下依次输出，图片模式下堆叠到同一画布。

### 选项一览

| 选项 | 说明 |
|---|---|
| **输入** | |
| `-f, --file <文件>` | 从文本文件读取协议规格（每行一个） |
| **图表布局** | |
| `-b, --bits <n>` | 每行比特数（默认 32；规格内 `?bits=` 次之，此选项优先级最高） |
| `-n, --no-numbers` | 不打印图表顶部的比特序号行 |
| `--evenchar <字符>` | 水平边框偶数位字符（默认 `-`） |
| `--oddchar <字符>` | 水平边框奇数位字符（默认 `+`） |
| `--startchar <字符>` | 水平边框起始字符（默认 `+`） |
| `--endchar <字符>` | 水平边框结束字符（默认 `+`） |
| `--sepchar <字符>` | 字段分隔字符（默认 `\|`） |
| **输出** | |
| `-o, --output <文件>` | 输出到文件；格式由扩展名推断（`.txt`、`.svg`、`.png`），`--format` 优先 |
| `--format <格式>` | 输出格式：`text`、`svg` 或 `png`（默认 `text`） |
| **颜色** | |
| `-t, --theme <名称>` | 颜色主题（文本默认 `none`，图片默认 `light`）；见 `protocol --list-themes` |
| `--color <模式>` | 终端 ANSI 彩色：`auto`（仅当 stdout 是终端时启用）、`always`、`never`（默认 `auto`） |
| **PNG / SVG** | |
| `--scale <n>` | PNG 像素缩放，整数 1–8（默认 2；一个字符格 = 8x12 字体像素 x 缩放） |
| `--transparent` | PNG/SVG 使用透明画布背景 |
| **信息** | |
| `--list` | 列出全部内置协议 |
| `--list-themes` | 列出全部颜色主题 |
| `-h, --help` | 显示帮助 |
| `-V, --version` | 显示版本 |

### 显示内置协议

```sh
protocol tcp
protocol tcp udp          # 一次多个
protocol dot1q ethernet
```

协议名可以不写全，只要没有歧义——`protocol et` 等价于 `protocol ethernet`。
前缀有歧义时会列出候选：

```
$ protocol icmpv6-r
Ambiguous protocol specifier 'icmpv6-r'. Did you mean any of these?
  icmpv6-radv
  icmpv6-redirect
  icmpv6-rsol
```

### 内置协议列表

| 名称 | 说明 |
|---|---|
| `ethernet` | 以太网 |
| `8021q`、`dot1q` | IEEE 802.1q VLAN 标签 |
| `tcp` | 传输控制协议 |
| `udp` | 用户数据报协议 |
| `ip` | 互联网协议第 4 版 |
| `ipv6` | 互联网协议第 6 版 |
| `icmp` | 互联网控制报文协议（ICMP） |
| `icmp-destination` | ICMP 目标不可达 |
| `icmp-time` | ICMP 超时 |
| `icmp-parameter` | ICMP 参数问题 |
| `icmp-source` | ICMP 源站抑制 |
| `icmp-redirect` | ICMP 重定向 |
| `icmp-echo` | ICMP 回显请求/应答 |
| `icmp-timestamp` | ICMP 时间戳请求/应答 |
| `icmp-information` | ICMP 信息请求/应答 |
| `icmpv6` | IPv6 控制报文协议（ICMPv6） |
| `icmpv6-destination` | ICMPv6 目标不可达 |
| `icmpv6-big` | ICMPv6 数据包过大 |
| `icmpv6-time` | ICMPv6 超时 |
| `icmpv6-parameter` | ICMPv6 参数问题 |
| `icmpv6-echo` | ICMPv6 回显请求/应答 |
| `icmpv6-rsol` | ICMPv6 路由器请求 |
| `icmpv6-radv` | ICMPv6 路由器通告 |
| `icmpv6-nsol` | ICMPv6 邻居请求 |
| `icmpv6-nadv` | ICMPv6 邻居通告 |
| `icmpv6-redirect` | ICMPv6 重定向 |
| `dhcp` | 动态主机配置协议 |
| `modbus_tcp` | Modbus-TCP |
| `profinet_rt` | Profinet-RT |
| `dnp3` | DNP3 |
| `tsap` | TSAP |
| `cotp_cr` | COTP 连接请求 |
| `cotp_dt` | COTP 数据 |
| `cotp_dr` | COTP 断开请求 |
| `s7_header` | S7 通信头 |
| `s7_item` | S7 通信条目 |
| `s7_data` | S7 通信数据 |
| `example` | 自定义边框字符演示规格 |
| `test` | 压力测试规格（1..127 比特字段） |

### 自定义协议规格

规格格式：`"<字段列表>[?选项]"`：

* **字段列表** —— 逗号分隔的 `名称:长度` 元组。`名称` 是字段标签（可包含除 `?`、`,`、`:` 之外的任意字符；
  含空格时请用引号包住整个规格）。`长度` 为大于 0 的整数，表示字段占用的比特数。
  逗号后**不要**留空格。
* **选项** —— 可选，以 `?` 开头，逗号分隔的 `键=值` 列表。

字段列表示例：

```
Type:8,Code:8,Checksum:16,Message Body:64
Source Port:16,Destination Port:16,Length:16,Checksum:16
```

字段长度不需要与行宽对齐，过长的字段会自动折行：

```
$ protocol "Source:16,TTL:8,Reserved:40"
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             Source            |      TTL      |               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               +
|                            Reserved                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

规格内可用选项：

| 选项 | 取值 | 说明 |
|---|---|---|
| `bits=<n>` | 大于 0 的整数 | 每行比特数（默认 32，与 IETF 一致） |
| `numbers=<y/n>` | `1 y yes true` / `0 n no false` | 是否打印比特序号行 |
| `evenchar=<字符>` | 单个字符 | 水平边框偶数位字符（默认 `-`） |
| `oddchar=<字符>` | 单个字符 | 水平边框奇数位字符（默认 `+`） |
| `startchar=<字符>` | 单个字符 | 水平边框起始字符（默认 `+`） |
| `endchar=<字符>` | 单个字符 | 水平边框结束字符（默认 `+`） |
| `sepchar=<字符>` | 单个字符 | 字段分隔字符（默认 `\|`） |

各字符修饰符作用位置示意：

```
   比特序号                                                       sepchar(|)
     ^   ^                                                              |
     |   |                                                              |
     0   |               1                   2                   3      |
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   |
    |             Source            |      TTL      |    Reserved   | <-+
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ^              ^ ^ ^              ^ ^ ^                         ^
    |              | | |              | | |                         |
 startchar(+)     evenchar(-)        oddchar(+)                  endchar(+)

    |<------------------------------------------------------------->|
                               bits(32)
```

### 从文件读取规格

`-f/--file` 从文本文件读取规格：每行一个规格，空行忽略，`#` 开头为注释。

```
# my_protocols.txt —— 自定义协议集合
Type:8,Code:8,Checksum:16,Message Body:64
Length:16,Checksum:16?bits=16,numbers=n
```

```sh
protocol -f my_protocols.txt                # 依次输出两个 ASCII 图表
protocol -f my_protocols.txt -o all.svg     # 两个协议堆叠到同一张 SVG
```

### 输出格式

**文本（默认）**：经典的 RFC 风格 ASCII 图表。不加 `-o` 时输出到标准输出；
`-o file.txt` 时写入文件。选择非 `none` 主题且启用彩色时输出 ANSI 真彩色转义序列
（`--color auto` 仅在 stdout 为终端时启用）；写入文件时绝不包含转义序列。

**SVG**：矢量输出，适合文档与幻灯片。字段背景为矩形，边框为连续线条，标签为真实文字
（等宽字体栈）。通过 `-o file.svg` 写文件，或 `--format svg` 从 stdout 输出。
`--transparent` 省略背景矩形。

**PNG**：使用内嵌 8x8 像素字体栅格化的位图（像素风）。`--scale <n>`（1–8，默认 2）
控制像素大小：缩放为 2 时一个字符格为 16x24 像素。`--transparent` 生成透明画布
（字段填充与字形保持不透明）。编码由内嵌 stb_image_write 完成，无需 libpng。

格式选择优先级：`--format` 高于 `-o` 文件扩展名；两者都没有时为 `text`。

### 颜色主题

```
$ protocol --list-themes
```

| 主题 | 风格 |
|---|---|
| `none` | 黑白经典 RFC 风格，字段无填充色 |
| `rfc` | 白底，浅灰交替 |
| `light` | 浅色背景 + 柔和粉彩字段色（图片默认主题） |
| `dark` | 深色背景 + 鲜亮强调色 |
| `solarized` | Solarized 浅色调色板 |
| `solarized-dark` | Solarized 深色调色板 |
| `monokai` | Monokai 编辑器调色板 |
| `ocean` | 深海蓝 |
| `forest` | 森林绿 |
| `sunset` | 暖色日落，浅色背景 |
| `candy` | 明亮糖果色 |
| `grayscale` | 灰度交替，适合打印 |

字段颜色按主题调色板循环分配，相邻字段颜色一定不同；跨多行的字段保持同一颜色。
主题对三种输出格式都生效（文本通过 ANSI 真彩色转义）。

### 退出状态码

* `0` —— 成功
* `1` —— 任何错误（规格无效、协议/主题/选项不存在、文件不可读、输出不可写等）。
  错误信息输出到标准错误。

## 命令示例

```sh
# 1. 打印 TCP 头的经典 ASCII 图表
protocol tcp

# 2. 一次输出多个协议
protocol ip tcp udp

# 3. 一行规格描述自定义协议
protocol "Type:8,Code:8,Checksum:16,Payload:64"

# 4. 自定义每行比特数、隐藏比特序号
protocol "Length:16,Checksum:16?bits=16,numbers=n"
protocol tcp -b 16 -n

# 5. 自定义边框字符
protocol "Field4:4,Field4:4,Field8:8,Field16:16?bits=16,startchar=*,endchar=*"
protocol tcp --startchar '*' --endchar '*' --evenchar '=' --oddchar '-' --sepchar '!'

# 6. 终端彩色输出（ANSI 真彩色）
protocol ipv6 --theme ocean --color always

# 7. 生成 SVG 图片（默认 light 主题）
protocol tcp -o tcp.svg

# 8. 生成 PNG 图片，指定主题与 3 倍缩放
protocol tcp -o tcp.png --theme dark --scale 3

# 9. 多个协议堆叠到同一张图片
protocol ip tcp udp -o stack.svg --theme solarized
protocol ip tcp udp -o stack.png --theme dark

# 10. 透明背景 PNG（适合幻灯片）
protocol icmp -o icmp.png --theme sunset --transparent

# 11. SVG / PNG 输出到 stdout（便于管道）
protocol tcp --format svg > tcp.svg
protocol udp --format png > udp.png

# 12. 从文件批量生成
protocol -f my_protocols.txt -o all.svg --theme forest

# 13. 协议名可写唯一前缀
protocol et              # 等价于 protocol ethernet
protocol s7_h            # 等价于 protocol s7_header
```

## 许可证

GNU GPL v3（见 `LICENSE.txt`），与原项目一致。原始作品 (c) 2014 Luis MartinGarcia，
本 C++ 重写版按相同许可证发布。`third_party/` 内组件为 public domain，
详见 `third_party/README.md`。
