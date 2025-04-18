# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/trading_bot_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/trading_bot_autogen.dir/ParseCache.txt"
  "trading_bot_autogen"
  )
endif()
