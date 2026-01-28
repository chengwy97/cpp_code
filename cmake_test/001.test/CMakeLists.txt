#[[
演示 cmake 的宏用法
在 cmake 的宏中，参数必须使用 ${变量名称} 的形式来获取
直接使用 ARGV 无法获取参数的值
使用 ${ARGV${i}} 的形式也无法获取参数的值
]]

macro(test_cmake p)
  message(STATUS "test_cmake: p")
  message(STATUS "test_cmake: ${p}")
  set(i 1)
  message(STATUS "test_cmake: i = ${ARGV${i}}")
endmacro()

test_cmake(123)
