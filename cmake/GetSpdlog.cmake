include(FetchContent)

message(STATUS "get spdlog ...")

# 1. 定义 FetchContent，指定 SOURCE_DIR 为本地路径
FetchContent_Declare(
    spdlog
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog"
)

# 2. 获取属性并检查是否已填充
FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
  # 运行 spdlog 目录下的 CMakeLists.txt，创建 'spdlog' (或 'spdlog::spdlog') 目标
  FetchContent_MakeAvailable(spdlog)
endif()

# --- 3. 创建别名 (核心步骤) ---
# spdlog 的 CMake 配置通常会创建一个名为 'spdlog' 的目标。
# 某些新版本或特定配置可能会创建 'spdlog::spdlog'。
# 最安全的方法是检查并链接到它。

# 假设 spdlog 库创建了名为 'spdlog' 的目标（最常见）
# 我们为它创建一个别名
if(TARGET spdlog)
    add_library(third_party::spdlog ALIAS spdlog)
    message(STATUS "Created alias third_party::spdlog for spdlog target.")
elseif(TARGET spdlog::spdlog)
    # 某些库使用命名空间目标。如果是这种情况，直接链接即可，别名可以省略，
    # 但如果要严格遵循您的命名规范，可以为它创建一个别名。
    # ⚠️ 注意：给命名空间目标创建别名通常不是必需的，但可以这样做：
    add_library(third_party::spdlog ALIAS spdlog::spdlog)
    message(STATUS "Created alias third_party::spdlog for spdlog::spdlog target.")
else()
    message(FATAL_ERROR "spdlog target not found after FetchContent_MakeAvailable. Check spdlog's CMakeLists.txt.")
endif()