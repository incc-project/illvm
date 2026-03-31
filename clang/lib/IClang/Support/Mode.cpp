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
// * "PCHCheck":
//    Normal compilation.
//    Save top include region (provided by user) to *.iclang/tir.h.
//    Hack input buffer, delete all code except for the top include region,
//    make PCH, save to *.iclang/iclang.pch.
//    Hack input buffer, delete the top include region,
//    compile inputFile with PCH.
//    Record normal compilation time, PCH making time, PCH optimization time.
//    Check:
//    no pch, compilation error.
// * "BasicFuncXCheck": Todo.
// * "IncLineCheck": Todo: merge IncLineCheck to LineMacroCheck.
// * "LineMacroCheck": Todo.
// * "DiffCheck": Todo.
// * "FuncXCheck": Todo.
// * "Dump": AST dump mode.
// * "Profile": profile Clang.
// * "Clang": default, equivalent to Clang.