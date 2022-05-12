// Copyright (c) 2021-present, Trail of Bits, Inc.

#include "vast/Translation/HighLevelTypeConverter.hpp"

#include "vast/Util/Warnings.hpp"

VAST_RELAX_WARNINGS
#include "clang/AST/Type.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/LLVM.h"
#include "mlir/IR/AffineMap.h"
VAST_UNRELAX_WARNINGS

#include "vast/Dialect/HighLevel/HighLevelAttributes.hpp"
#include "vast/Dialect/HighLevel/HighLevelDialect.hpp"
#include "vast/Dialect/HighLevel/HighLevelTypes.hpp"
#include "vast/Translation/HighLevelBuilder.hpp"

#include <cassert>
#include <iostream>

namespace vast::hl
{
    using BuiltinType = clang::BuiltinType;

    constexpr IntegerKind get_integer_kind(const BuiltinType *ty)
    {
        switch (ty->getKind()) {
            case BuiltinType::Char_U:
            case BuiltinType::UChar:
            case BuiltinType::Char_S:
            case BuiltinType::SChar:
                return IntegerKind::Char;
            case BuiltinType::Short:
            case BuiltinType::UShort:
                return IntegerKind::Short;
            case BuiltinType::Int:
            case BuiltinType::UInt:
                return IntegerKind::Int;
            case BuiltinType::Long:
            case BuiltinType::ULong:
                return IntegerKind::Long;
            case BuiltinType::LongLong:
            case BuiltinType::ULongLong:
                return IntegerKind::LongLong;
            case BuiltinType::Int128:
            case BuiltinType::UInt128:
                return IntegerKind::Int128;
            default:
                VAST_UNREACHABLE("unknown integer kind");
        }
    }

    constexpr FloatingKind get_floating_kind(const BuiltinType *ty)
    {
        switch (ty->getKind()) {
            case BuiltinType::Half:
            case BuiltinType::Float16:
                return FloatingKind::Half;
            case BuiltinType::BFloat16:
                return FloatingKind::BFloat16;
            case BuiltinType::Float:
                return FloatingKind::Float;
            case BuiltinType::Double:
                return FloatingKind::Double;
            case BuiltinType::LongDouble:
                return FloatingKind::LongDouble;
            case BuiltinType::Float128:
                return FloatingKind::Float128;
            default:
                VAST_UNREACHABLE("unknown floating kind");
        }
    }

    mlir::Type HighLevelTypeConverter::convert(clang::QualType ty) {
        return convert(ty.getTypePtr(), ty.getQualifiers());
    }

    mlir::Type HighLevelTypeConverter::convert(const clang::Type *ty, Quals quals) {
        return dl_aware_convert(ty, quals);
    }

    mlir::Type HighLevelTypeConverter::lvalue_convert(clang::QualType ty) {
        return lvalue_convert(ty.getTypePtr(), ty.getQualifiers());
    }

    mlir::Type HighLevelTypeConverter::lvalue_convert(const clang::Type *ty, Quals quals) {
        return dl_aware_lvalue_convert(ty, quals);
    }

    mlir::Type HighLevelTypeConverter::dl_aware_lvalue_convert(const clang::Type *ty, Quals quals) {
        auto underlying = dl_aware_convert(ty, quals);
        VAST_ASSERT(!ty->isFunctionType());
        auto value = LValueType::get(&ctx.getMLIRContext(), underlying);
        ctx.data_layout().try_emplace(value, ty, ctx.getASTContext());
        return value;
    }

    bool is_forward_declared(const clang::Type *ty) {
        if (auto tag = ty->getAsTagDecl()) {
            return !tag->getDefinition();
        }
        return false;
    }

    mlir::Type HighLevelTypeConverter::dl_aware_convert(const clang::Type *ty, Quals quals) {
        auto out = do_convert(ty, quals);

        if (!ty->isFunctionType() && !is_forward_declared(ty)) {
            ctx.data_layout().try_emplace(out, ty, ctx.getASTContext());
        }
        return out;
    }

    std::string HighLevelTypeConverter::format_type(const clang::Type *type) const {
        std::string name;
        llvm::raw_string_ostream os(name);
        type->dump(os, ctx.getASTContext());
        return name;
    }

    mlir::Type HighLevelTypeConverter::do_convert(const clang::Type *ty, Quals quals) {
        if (auto td = llvm::dyn_cast< clang::TypedefType >(ty)) {
            return do_convert(td, quals);
        }

        ty = ty->getUnqualifiedDesugaredType();

        if (ty->isBuiltinType())
            return do_convert(clang::cast< BuiltinType >(ty), quals);

        if (ty->isPointerType())
            return do_convert(clang::cast< clang::PointerType >(ty), quals);

        if (ty->isRecordType())
            return do_convert(clang::cast< clang::RecordType >(ty), quals);

        if (ty->isEnumeralType())
            return do_convert(clang::cast< clang::EnumType >(ty), quals);

        if (ty->isArrayType())
            return do_convert(clang::cast< clang::ArrayType >(ty), quals);

        if (ty->isFunctionType())
            return convert(clang::cast< clang::FunctionType >(ty));

        VAST_UNREACHABLE( "unknown clang type: {0}", format_type(ty) );
    }

    mlir::Type HighLevelTypeConverter::do_convert(const BuiltinType *ty, Quals quals) {
        auto v = quals.hasVolatile();
        auto c = quals.hasConst();

        auto &mctx = ctx.getMLIRContext();

        if (ty->isVoidType()) {
            return VoidType::get(&mctx);
        }

        if (ty->isBooleanType()) {
            return BoolType::get(&mctx, c, v);
        }

        if (ty->isIntegerType()) {
            auto u = ty->isUnsignedIntegerType();

            switch (get_integer_kind(ty)) {
                case IntegerKind::Char:     return CharType::get(&mctx, u, c, v);
                case IntegerKind::Short:    return ShortType::get(&mctx, u, c, v);
                case IntegerKind::Int:      return IntType::get(&mctx, u, c, v);
                case IntegerKind::Long:     return LongType::get(&mctx, u, c, v);
                case IntegerKind::LongLong: return LongLongType::get(&mctx, u, c, v);
                case IntegerKind::Int128:   return Int128Type::get(&mctx, u, c, v);
            }
        }

        if (ty->isFloatingType()) {
            switch (get_floating_kind(ty)) {
                case FloatingKind::Half:       return HalfType::get(&mctx, c, v);
                case FloatingKind::BFloat16:   return BFloat16Type::get(&mctx, c, v);
                case FloatingKind::Float:      return FloatType::get(&mctx, c, v);
                case FloatingKind::Double:     return DoubleType::get(&mctx, c, v);
                case FloatingKind::LongDouble: return LongDoubleType::get(&mctx, c, v);
                case FloatingKind::Float128:   return Float128Type::get(&mctx, c, v);
            }
        }

        VAST_UNREACHABLE( "unknown builtin type: {0}", format_type(ty) );
    }

    mlir::Type HighLevelTypeConverter::do_convert(const clang::PointerType *ty, Quals quals) {
        return PointerType::get(
            & ctx.getMLIRContext()
            , convert(ty->getPointeeType())
            , quals.hasConst()
            , quals.hasVolatile()
            , quals.hasRestrict()
        );
    }

    mlir::Type HighLevelTypeConverter::do_convert(const clang::RecordType *ty, Quals quals) {
        auto decl = ty->getDecl();
        auto name = ctx.elaborated_name(decl);
        auto mctx = &ctx.getMLIRContext();
        return NamedType::get(mctx, mlir::StringAttr::get(mctx, name));
    }

    mlir::Type HighLevelTypeConverter::do_convert(const clang::EnumType *ty, Quals quals) {
        auto decl = ty->getDecl();
        auto name = ctx.elaborated_name(decl);
        auto mctx = &ctx.getMLIRContext();
        return NamedType::get(mctx, mlir::StringAttr::get(mctx, name));
    }

    namespace detail {
        SizeAttr get_size_attr(const clang::ConstantArrayType *arr, MContext &ctx) {
            return SizeAttr::get(&ctx);
        }

        SizeAttr get_size_attr(const clang::DependentSizedArrayType *arr, MContext &ctx) {
            return {};
        }

        SizeAttr get_size_attr(const clang::IncompleteArrayType *arr, MContext &ctx) {
            return {};
        }

        SizeAttr get_size_attr(const clang::VariableArrayType *arr, MContext &ctx) {
            return {};
        }
    } // namespace detail

    SizeAttr HighLevelTypeConverter::get_size_attr(const clang::ArrayType *ty) {
        return llvm::TypeSwitch< const clang::ArrayType *, SizeAttr >(ty)
            .Case< clang::ConstantArrayType, clang::DependentSizedArrayType
                 , clang::IncompleteArrayType, clang::VariableArrayType >
            ([&] (const auto *array_type) {
                return detail::get_size_attr(array_type, ctx.getMLIRContext());
            });
    }

    Type HighLevelTypeConverter::do_convert(const clang::ArrayType *ty, Quals quals) {
        auto element_type = convert(ty->getElementType());
        return ArrayType::get(&ctx.getMLIRContext(), element_type, get_size_attr(ty));
    }

    mlir::FunctionType HighLevelTypeConverter::convert(const clang::FunctionDecl *decl) {
        llvm::SmallVector< mlir::Type > args;
        for (auto param : decl-> parameters()) {
            args.push_back(lvalue_convert(param->getType()));
        }

        auto rty = convert(decl->getReturnType());
        return mlir::FunctionType::get(&ctx.getMLIRContext(), args, rty);
    }

    mlir::Type HighLevelTypeConverter::do_convert(const clang::TypedefType *ty, Quals quals) {
        auto decl = ty->getDecl();
        auto name = decl->getName();
        auto mctx = &ctx.getMLIRContext();
        return NamedType::get(mctx, mlir::StringAttr::get(mctx, name));
    }

} // namseapce vast::hl
