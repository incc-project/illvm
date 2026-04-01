//===--- CheckCC1Driver.h - IClang cc1 driver for checking -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// IClang cc1 driver for checking.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_CHECKCC1DRIVER_H
#define ICLANG_CHECKCC1DRIVER_H

namespace iclang {

class SourceRangeCheckCC1Driver {
public:
  static void run();
};

class PCHCheckCC1Driver {
public:
  static void run() {}
};

class BasicFuncXCheckCC1Driver {
public:
  static void run();
};

class IncLineCheckCC1Driver {
public:
  static void run() {}
};

class DiffCheckCC1Driver {
public:
  static void run() {}
};

class FuncXCheckCC1Driver {
public:
  static void run();
};

class DumpCC1Driver {
public:
  static void run();
};

class ProfileCC1Driver {
public:
  static void run() {}
};

// PlaceHolder
class ClangCC1Driver {
public:
  static void run() {}
};

} // namespace iclang

#endif //ICLANG_CHECKCC1DRIVER_H
