include(FetchContent)

message(STATUS "get mjrrt ...")

FetchContent_Declare(mjrrt SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mjrrt")

FetchContent_GetProperties(mjrrt)
if(NOT mjrrt_POPULATED)
  # ==============================
  # Configure mjrrt build flags
  # ==============================
  # 静态库
  set(BUILD_SHARED_LIBS
      OFF
      CACHE BOOL "disable shared libs export support in mjrrt" FORCE)
  # 安装
  set(RT_INSTALL
      ON
      CACHE BOOL "enable install support in mjrrt" FORCE)
  # 启用日志组件支持
  set(RT_BUILD_WITH_LOG
      ON
      CACHE BOOL "enable log component support in mjrrt" FORCE)

  set(IS_ORIN_NX
      ON
      CACHE BOOL "enable orin nx support in mjrrt" FORCE)

  FetchContent_MakeAvailable(mjrrt)
endif()