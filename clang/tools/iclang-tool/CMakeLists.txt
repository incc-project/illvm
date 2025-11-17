set(LLVM_LINK_COMPONENTS
  Support
  ILLVMSupport
  )

add_clang_tool(iclang-tool
  main.cpp
  )

clang_target_link_libraries(iclang-tool
  PRIVATE
  clangBasic
  clangIClangSupport
  clangIClangASTSupport
  )