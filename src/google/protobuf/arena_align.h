// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file provides alignment utilities for use in arenas.
//
// `ArenaAlign` contains a single `align` data member and provides
// the below functions which operate on the given alignment.
//
//   Ceil(size_t n)      - rounds `n` up to the nearest `align` boundary.
//   Floor(size_t n)     - rounds `n` down to the nearest `align` boundary.
//   Padded(size_t n)    - returns the unaligned size to align 'n' bytes. (1)

//   Ceil(T* P)          - rounds `p` up to the nearest `align` boundary. (2)
//   IsAligned(size_t n) - returns true if `n` is aligned to `align`
//   IsAligned(T* p)     - returns true if `p` is aligned to `align`
//   CheckAligned(T* p)  - returns `p`. Checks alignment of `p` in debug.
//
// 1) `Padded(n)` returns the minimum size needed to align an object of size 'n'
//    into a memory area that is default aligned. For example, allocating 'n'
//    bytes aligned at 32 bytes requires a size of 'n + 32 - 8' to align at 32
//    bytes for any 8 byte boundary.
//
// 2) There is an optimized `CeilDefaultAligned(T*)` method which is equivalent
//    to `Ceil(ArenaAlignDefault::CheckAlign(p))` but more efficiently
//    implemented as a 'check only' for ArenaAlignDefault.
//
// These classes allow for generic arena logic using 'alignment policies'.
//
// For example:
//
//  template <Align>
//  void* NaiveAlloc(size_t n, Align align) {
//    ABSL_ASSERT(align.IsAligned(n));
//    const size_t required = align.Padded(n);
//    if (required <= static_cast<size_t>(ptr_ - limit_)) {
//      uint8_t* ptr = align.CeilDefaultAligned(ptr_);
//      ptr_ = ptr + n;
//      return ptr;
//    }
//    return nullptr;
//  }
//
//  void CallSites() {
//    void *p1 = NaiveAlloc(n, ArenaAlignDefault());
//    void *p2 = NaiveAlloc(n, ArenaAlignAs(32));
//  }
//
#ifndef GOOGLE_PROTOBUF_ARENA_ALIGN_H__
#define GOOGLE_PROTOBUF_ARENA_ALIGN_H__

#include <cstddef>
#include <cstdint>

#include "google/protobuf/stubs/logging.h"
#include "absl/numeric/bits.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

struct ArenaAlignDefault {
  PROTOBUF_EXPORT static constexpr size_t align = 8;  // NOLINT

  static constexpr bool IsAligned(size_t n) { return (n & (align - 1)) == 0; }

  template <typename T>
  static inline PROTOBUF_ALWAYS_INLINE bool IsAligned(T* ptr) {
    return (reinterpret_cast<uintptr_t>(ptr) & (align - 1)) == 0;
  }

  static inline PROTOBUF_ALWAYS_INLINE constexpr size_t Ceil(size_t n) {
    return (n + align - 1) & -align;
  }
  static inline PROTOBUF_ALWAYS_INLINE constexpr size_t Floor(size_t n) {
    return (n & ~(align - 1));
  }

  static inline PROTOBUF_ALWAYS_INLINE constexpr size_t Padded(size_t n) {
    ABSL_ASSERT(IsAligned(n));
    return n;
  }

  template <typename T>
  static inline PROTOBUF_ALWAYS_INLINE T* Ceil(T* ptr) {
    uintptr_t intptr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<T*>((intptr + align - 1) & -align);
  }

  template <typename T>
  static inline PROTOBUF_ALWAYS_INLINE T* CeilDefaultAligned(T* ptr) {
    ABSL_ASSERT(IsAligned(ptr));
    return ptr;
  }

  // Address sanitizer enabled alignment check
  template <typename T>
  static inline PROTOBUF_ALWAYS_INLINE T* CheckAligned(T* ptr) {
    ABSL_ASSERT(IsAligned(ptr));
    return ptr;
  }
};

struct ArenaAlign {
  static constexpr bool IsDefault() { return false; };

  size_t align;

  constexpr bool IsAligned(size_t n) const { return (n & (align - 1)) == 0; }

  template <typename T>
  bool IsAligned(T* ptr) const {
    return (reinterpret_cast<uintptr_t>(ptr) & (align - 1)) == 0;
  }

  constexpr size_t Ceil(size_t n) const { return (n + align - 1) & -align; }
  constexpr size_t Floor(size_t n) const { return (n & ~(align - 1)); }

  constexpr size_t Padded(size_t n) const {
    ABSL_ASSERT(IsAligned(n));
    ABSL_ASSERT(ArenaAlignDefault::IsAligned(align));
    return n + align - ArenaAlignDefault::align;
  }

  template <typename T>
  T* Ceil(T* ptr) const {
    uintptr_t intptr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<T*>((intptr + align - 1) & -align);
  }

  template <typename T>
  T* CeilDefaultAligned(T* ptr) const {
    ABSL_ASSERT(ArenaAlignDefault::IsAligned(ptr));
    return Ceil(ptr);
  }

  // Address sanitizer enabled alignment check
  template <typename T>
  T* CheckAligned(T* ptr) const {
    ABSL_ASSERT(IsAligned(ptr));
    return ptr;
  }
};

inline ArenaAlign ArenaAlignAs(size_t align) {
  // align must be a non zero power of 2 >= 8
  GOOGLE_DCHECK_NE(align, 0);
  GOOGLE_DCHECK(absl::has_single_bit(align)) << "Invalid alignment " << align;
  return ArenaAlign{align};
}

template <bool, size_t align>
struct AlignFactory {
  static_assert(align > ArenaAlignDefault::align, "Not over-aligned");
  static_assert((align & (align - 1)) == 0, "Not power of 2");
  static constexpr ArenaAlign Create() { return ArenaAlign{align}; }
};

template <size_t align>
struct AlignFactory<true, align> {
  static_assert(align <= ArenaAlignDefault::align, "Over-aligned");
  static_assert((align & (align - 1)) == 0, "Not power of 2");
  static constexpr ArenaAlignDefault Create() { return ArenaAlignDefault{}; }
};

// Returns an `ArenaAlignDefault` instance for `align` less than or equal to the
// default alignment, and `AlignAs(align)` for over-aligned values of `align`.
// The purpose is to take advantage of invoking functions accepting a template
// overloaded 'Align align` argument reducing the alignment operations on
// `ArenaAlignDefault` implementations to no-ops.
template <size_t align>
inline constexpr auto ArenaAlignAs() {
  return AlignFactory<align <= ArenaAlignDefault::align, align>::Create();
}

// Returns ArenaAlignAs<alignof(T)>
template <typename T>
inline constexpr auto ArenaAlignOf() {
  return ArenaAlignAs<alignof(T)>();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_ALIGN_H__
