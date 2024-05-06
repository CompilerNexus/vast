// Copyright (c) 2022-present, Trail of Bits, Inc.

#pragma once

#include "vast/Util/Warnings.hpp"

VAST_RELAX_WARNINGS
#include <clang/AST/DeclVisitor.h>
VAST_UNRELAX_WARNINGS

#include "vast/CodeGen/CodeGenVisitorBase.hpp"
#include "vast/CodeGen/CodeGenMembers.hpp"

namespace vast::cg {

    struct default_decl_visitor : decl_visitor_base< default_decl_visitor >
    {
        using base = decl_visitor_base< default_decl_visitor >;
        using base::base;

        using decl_visitor_base< default_decl_visitor >::Visit;

        operation visit(const clang_decl *decl) { return Visit(decl); }
        operation visit_prototype(const clang_function *decl);
        mlir_attr_list visit_attrs(const clang_function *decl);

        void fill_init(const clang_expr *init, hl::VarDeclOp var);

        operation VisitVarDecl(const clang_var_decl *decl);
        operation VisitParmVarDecl(const clang::ParmVarDecl *decl);
        operation VisitImplicitParamDecl(const clang::ImplicitParamDecl *decl);
        operation VisitLinkageSpecDecl(const clang::LinkageSpecDecl *decl);
        operation VisitFunctionDecl(const clang::FunctionDecl *decl);
        operation VisitTranslationUnitDecl(const clang::TranslationUnitDecl *decl);
        operation VisitTypedefNameDecl(const clang::TypedefNameDecl *decl);
        operation VisitTypedefDecl(const clang::TypedefDecl *decl);
        operation VisitTypeAliasDecl(const clang::TypeAliasDecl *decl);
        operation VisitLabelDecl(const clang::LabelDecl *decl);
        operation VisitEmptyDecl(const clang::EmptyDecl *decl);
        operation VisitEnumDecl(const clang::EnumDecl *decl);
        operation VisitEnumConstantDecl(const clang::EnumConstantDecl *decl);
        operation VisitRecordDecl(const clang::RecordDecl *decl);
        operation VisitCXXRecordDecl(const clang::CXXRecordDecl *decl);
        operation VisitAccessSpecDecl(const clang::AccessSpecDecl *decl);
        operation VisitFieldDecl(const clang::FieldDecl *decl);
        operation VisitIndirectFieldDecl(const clang::IndirectFieldDecl *decl);

        void fill_enum_constants(const clang::EnumDecl *decl);

        operation mk_incomplete_decl(const clang::RecordDecl *decl);

        template< typename RecordDeclOp >
        operation mk_record_decl(const clang::RecordDecl *decl) {
            auto field_builder = [&] (auto &/* bld */, auto /* loc */) {
                auto gen = mk_scoped_generator< members_generator >(
                    self.scope, bld, self
                );

                gen.emit(decl);
            };

            return maybe_declare([&] {
                return bld.compose< RecordDeclOp >()
                    .bind(self.location(decl))
                    .bind(self.symbol(decl))
                    .bind(field_builder)
                    .freeze();
            });
        }
    };

} // namespace vast::cg