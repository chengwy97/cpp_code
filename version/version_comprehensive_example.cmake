# ========== 项目版本信息 ==========
project(MyProject VERSION 1.2.3)

# 从 PROJECT_VERSION 提取主版本号、次版本号、补丁号
string(REGEX MATCH "^([0-9]+)" VERSION_MAJOR "${PROJECT_VERSION}")
string(REGEX MATCH "^[0-9]+\\.([0-9]+)" VERSION_MINOR "${PROJECT_VERSION}")
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.([0-9]+)" VERSION_PATCH "${PROJECT_VERSION}")

# ========== Git 信息 ==========
find_package(Git QUIET)

if(GIT_FOUND)
    # 完整提交 ID
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # 短提交 ID（前 7 位）
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # 当前分支名
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # 最近的标签
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # 检查是否有未提交的更改
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_DIRTY_RESULT
    )
    if(GIT_DIRTY_RESULT EQUAL 0)
        set(GIT_DIRTY "false")
    else()
        set(GIT_DIRTY "true")
    endif()
else()
    set(GIT_COMMIT_ID "unknown")
    set(GIT_COMMIT_SHORT "unknown")
    set(GIT_BRANCH "unknown")
    set(GIT_TAG "unknown")
    set(GIT_DIRTY "false")
endif()

# ========== 构建时间信息 ==========
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME_ISO "%Y-%m-%dT%H:%M:%S")

# ========== 主机信息 ==========
# 获取主机名
if(UNIX)
    execute_process(
        COMMAND hostname
        OUTPUT_VARIABLE HOSTNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # 获取用户名
    execute_process(
        COMMAND whoami
        OUTPUT_VARIABLE USERNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
elseif(WIN32)
    set(HOSTNAME $ENV{COMPUTERNAME})
    set(USERNAME $ENV{USERNAME})
else()
    set(HOSTNAME "unknown")
    set(USERNAME "unknown")
endif()

# ========== 构建配置 ==========
# 如果没有设置 BUILD_TYPE，设置默认值
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# 构建机器标识（主机名 + 用户名）
set(BUILD_MACHINE "${HOSTNAME} (${USERNAME})")

# 构建编号（可以从环境变量或 CI/CD 系统获取）
if(DEFINED ENV{BUILD_NUMBER})
    set(BUILD_NUMBER $ENV{BUILD_NUMBER})
else()
    set(BUILD_NUMBER "local")
endif()

# ========== 输出信息（调试用）==========
message(STATUS "=== Build Information ===")
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Git commit: ${GIT_COMMIT_SHORT} (${GIT_BRANCH})")
message(STATUS "Build time: ${BUILD_TIME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Build machine: ${BUILD_MACHINE}")

# ========== 生成头文件 ==========
configure_file(
    ${CMAKE_SOURCE_DIR}/version/version_comprehensive.hpp.in
    ${CMAKE_BINARY_DIR}/generated/version/version.hpp
    @ONLY
)

include_directories(${CMAKE_BINARY_DIR}/generated)

