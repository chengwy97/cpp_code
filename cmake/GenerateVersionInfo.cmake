message(STATUS "Generating version info from ${from_template_file} to ${to_file}...")
# Extract version components from PROJECT_VERSION
string(REGEX MATCH "^([0-9]+)" VERSION_MAJOR "${PROJECT_VERSION}")
string(REGEX MATCH "^[0-9]+\\.([0-9]+)" VERSION_MINOR "${PROJECT_VERSION}")
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.([0-9]+)" VERSION_PATCH "${PROJECT_VERSION}")

# ========== Git Information ==========
find_package(Git QUIET)

if(GIT_FOUND)
    # Full commit id
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Short commit id (first 7 chars)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Current branch
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Latest tag
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Check for dirty status
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

# ========== Build Time Information ==========
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME_ISO "%Y-%m-%dT%H:%M:%S")

# ========== Host Info ==========
if(UNIX)
    execute_process(
        COMMAND hostname
        OUTPUT_VARIABLE HOSTNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
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

# ========== Build Config ==========
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
set(BUILD_MACHINE "${HOSTNAME} (${USERNAME})")

if(DEFINED ENV{BUILD_NUMBER})
    set(BUILD_NUMBER $ENV{BUILD_NUMBER})
else()
    set(BUILD_NUMBER "local")
endif()

# ========== Print Build Info ==========
message(STATUS "=== Build Information (generate_version_info) ===")
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Git commit: ${GIT_COMMIT_SHORT} (${GIT_BRANCH})")
message(STATUS "Build time: ${BUILD_TIME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Build machine: ${BUILD_MACHINE}")

# ========== Generate version header ==========
configure_file(
    ${FROM_TEMPLATE}
    ${TO_FILE}
    @ONLY
)
