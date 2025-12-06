set(LLVM_LINK_COMPONENTS
  Option
  ProfileData
  Support
  ILLVMSupport
  )

add_clang_library(clangIClangSupport
  Global.cpp
  ILexer.cpp
  MetaData.cpp
  Mode.cpp

  DEPENDS
  ClangDriverOptions

  LINK_LIBS
  clangBasic
  ${system_libs}
)