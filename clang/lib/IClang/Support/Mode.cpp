// iClangMode:
// * "Inc": function-level incremental compilation.
// * "IncCheck": inc check mode for IClang developers.
// * "ShareMaster": master mode of shared compilation optimization.
// * "ShareClient": client mode of shared compilation optimization.
// * "ShareCheck": share check mode for IClang developers.
// * "SourceRangeCheck":
//    Record AST source range in compile.json, include:
//      * type(func, class, template)
//      * name
//      * source range(line, column)
//      * mangledName
//      * tags:
//        * special-linkage
//        * constexpr
//        * auto
//        * operator
//        * constructor, destructor
//        * virtual
//        * special-attr
//        * default, delete
//        * invalid-compound-stmt
//     Record the source range of the first Decl (offset, line, column).
//     Condition:
//       * In main file.
//       * Not implicit.
//       * Is not instantiation, specialization.
//       * Valid source range.
//       * Top-level-class, top-level-function/member definition (Note: filter
//         in-var func), top-level-template.
// * "ILexerCheck":
//    Run ILexer for the inputFile,
//    convert UTF-8 to whitespace,
//    eliminate cross line,
//    convert comment to whitespace,
//    match (#if*, #endif) directives,
//    save to .iclang/ilexer.cpp,
//    hack input buffer.
//    Calculate top include region.
//    Record the source range of the first Decl.
//    Check:
//    Only support R"()" (no delim).
//    '', "", /**/ pairwise matching.
//    (#if*, #endif) pairwise matching.
//    size(ilexer.cpp) == size(inputFile).
//    top include region <= the source range of the first Decl.
//    no compilation error.
// * "PCHCheck":
//    Normal compilation.
//    Calculate top include region by ILexer, save to *.iclang/tir.h.
//    Hack input buffer, delete all code except for the top include region,
//    make PCH, save to *.iclang/iclang.pch.
//    Hack input buffer, delete the top include region,
//    compile inputFile with PCH.
//    Record normal compilation time, PCH making time, PCH optimization time.
//    Check:
//    no pch, compilation error.
// * "IncLineCheck": Todo: merge IncLineCheck to LineMacroCheck.
// * "LineMacroCheck": Todo.
// * "BasicFuncXCheck": Todo.
// * "DiffCheck": Todo.
// * "FuncXCheck": Todo.
// * "Dump": AST dump mode.
// * "Profile": profile Clang.
// * "Clang": default, equivalent to Clang.

#include "iclang/Support/ILexer.h"