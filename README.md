# Score2dx

- C++ Library for analyzing and processing Beatmania IIDX CSV score.
- Example application: [score2dx-gui](https://github.com/blazar0112/score2dx-gui)

## Features

- Load Konami CSV files.
- Identify timeline of score.
- Import/Export to Json format.
- Utility to scrap IST data.
- Analyze for statistics and activity.

## Build

- Dependencies:

    | Library | Version | Site |
    | - | - | - |
    | Google Test | 1.10.0 | `https://github.com/google/googletest` |
    | FMT | 6.2.0 | `https://github.com/fmtlib/fmt` |
    | Nlohmann Json | 3.9.1 | `https://github.com/nlohmann/json` |
    | IES | 3.0.0 | `https://github.com/blazar0112/ies` |

- Only MINGW flow is supported currently.
- Set environment variable `INSTALL_ROOT`, `FMT_ROOT`, `JSON_ROOT` point to installed paths.

    | Library | Type | CMake usage |
    | - | - | - |
    | Google Test | MSVC | managed by vcpkg |
    | Google Test | MINGW | `$ENV{INSTALL_ROOT}/gtest/1.10.0/mingw64-10.2.0` |
    | FMT | Header only | `$ENV{FMT_ROOT}/include` |
    | Nlohmann Json | Header only | `$ENV{JSON_ROOT}/include` |
    | IES | MSVC | Todo. |
    | IES | MINGW | `$ENV{INSTALL_ROOT}/ies/latest/mingw64-11.2.0` |

- Build as any cmake project

    ```
    cd /build/score2dx/build-mingw64-11.2.0
    cmake /projects/score2dx -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PATH=/install/score2dx/latest/mingw64-11.2.0
    make -j8
    make install
    ```