/*
 * Copyright (c) 2021 Trail of Bits, Inc.
 */

#pragma once

#define VAST_COMMON_RELAX_WARNINGS \
  _Pragma( "GCC diagnostic ignored \"-Wsign-conversion\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Wconversion\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Wold-style-cast\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Wunused-parameter\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Wcast-align\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Woverloaded-virtual\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Wctad-maybe-unsupported\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Wdouble-promotion\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Wshadow\"")

#define VAST_CLANG_RELAX_WARNINGS \
  _Pragma( "GCC diagnostic ignored \"-Wambiguous-reversed-operator\"" )

#define VAST_GCC_RELAX_WARNINGS \
  _Pragma( "GCC diagnostic ignored \"-Wuseless-cast\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Wnull-dereference\"" ) \
  _Pragma( "GCC diagnostic ignored \"-Wmaybe-uninitialized\"" )

#ifdef __clang__
#define VAST_RELAX_WARNINGS \
  _Pragma( "GCC diagnostic push" ) \
  VAST_COMMON_RELAX_WARNINGS \
  VAST_CLANG_RELAX_WARNINGS
#elif __GNUC__
#define VAST_RELAX_WARNINGS \
  _Pragma( "GCC diagnostic push" ) \
  VAST_COMMON_RELAX_WARNINGS \
  VAST_GCC_RELAX_WARNINGS
#else
#define VAST_RELAX_WARNINGS \
  _Pragma( "GCC diagnostic push" ) \
  VAST_COMMON_RELAX_WARNINGS
#endif

#define VAST_UNRELAX_WARNINGS \
  _Pragma( "GCC diagnostic pop" )

VAST_RELAX_WARNINGS
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/Debug.h>
VAST_UNRELAX_WARNINGS

#ifdef ENABLE_VAST_EXCEPTION
#include <stdexcept>
#include <sstream>

namespace vast
{
    struct error_stream {
      private:
        std::stringstream buff;

       public:
        error_stream(std::stringstream ss = std::stringstream())
              : buff(std::move(ss)) {}
        ~error_stream() noexcept(false) {
            throw std::runtime_error(buff.str());
        }

        template <typename V>
        error_stream& operator<<(V &&s) {
            // explicit cast to std::string to avoid compiler error
            // during implicit type conversion
            buff << static_cast<std::string>(std::forward<V>(s));
            return *this;
        }
    };
} // namespace vast

  #define vast_error vast::error_stream
#else
  #define vast_error llvm::dbgs
#endif

#define vast_debug llvm::dbgs

#define DEBUG_TYPE "vast"

namespace vast
{

    #define VAST_ERROR(...) do { \
      vast_error() << "[VAST Error] " << llvm::formatv(__VA_ARGS__) << "\n"; \
    } while(0)

    #define VAST_REPORT(...) do { \
      vast_debug() << "[VAST Debug] " << llvm::formatv(__VA_ARGS__) << "\n"; \
    } while(0)

    #define VAST_UNREACHABLE(...) do { \
      VAST_ERROR(__VA_ARGS__); \
      llvm_unreachable(nullptr); \
    } while (0)

    #define VAST_UNIMPLEMENTED VAST_UNREACHABLE("not implemented: {0}", __PRETTY_FUNCTION__)

    #define VAST_UNIMPLEMENTED_MSG(msg) \
      VAST_UNREACHABLE("not implemented: {0} because {1}", __PRETTY_FUNCTION__, msg)

    #define VAST_UNIMPLEMENTED_IF(cond) \
      if (cond) { VAST_UNREACHABLE("not implemented: {0}", __PRETTY_FUNCTION__); }

    #define VAST_DEBUG(fmt, ...) LLVM_DEBUG(VAST_REPORT(__VA_ARGS__))

    #define VAST_CHECK(cond, fmt, ...) if (!(cond)) { VAST_UNREACHABLE(fmt __VA_OPT__(,) __VA_ARGS__); }

    #define VAST_ASSERT(cond) if (!(cond)) { VAST_UNREACHABLE("assertion: " #cond " failed"); }

    #define VAST_TODO(fmt, ... ) VAST_UNREACHABLE("[vast-todo]: " # fmt __VA_OPT__(,) __VA_ARGS__ )

}
