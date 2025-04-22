# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/CryptoBacktesterGUI_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/CryptoBacktesterGUI_autogen.dir/ParseCache.txt"
  "CryptoBacktesterGUI_autogen"
  )
endif()
