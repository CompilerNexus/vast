// Copyright (c) 2022-present, Trail of Bits, Inc.

#include "vast/CodeGen/CodeGenDriver.hpp"

VAST_RELAX_WARNINGS
#include <clang/AST/GlobalDecl.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <mlir/IR/Verifier.h>
VAST_UNRELAX_WARNINGS

#include "vast/Frontend/Options.hpp"

#include "vast/CodeGen/CodeGenVisitor.hpp"
#include "vast/CodeGen/DataLayout.hpp"
#include "vast/CodeGen/DefaultVisitor.hpp"
#include "vast/CodeGen/UnreachableVisitor.hpp"
#include "vast/CodeGen/UnsupportedVisitor.hpp"

#include "vast/CodeGen/CodeGenModule.hpp"

namespace vast::cg {

    void driver::emit(clang::DeclGroupRef decls) { generator.emit(decls); }

    void driver::emit(clang::Decl *decl) { generator.emit(decl); }

    owning_module_ref driver::freeze() { return std::move(mod); }

    // TODO this should not be needed the data layout should be emitted from cached types directly
    dl::DataLayoutBlueprint emit_data_layout_blueprint(
        const acontext_t &actx, const default_visitor &visitor
    ) {
        dl::DataLayoutBlueprint dl;

        auto store_layout = [&] (const clang_type *orig, mlir_type vast_type) {
            if (orig->isFunctionType()) {
                return;
            }

            // skip forward declared types
            if (auto tag = orig->getAsTagDecl()) {
                if (!tag->isThisDeclarationADefinition()) {
                    return;
                }
            }

            dl.try_emplace(vast_type, orig, actx);
        };

        for (auto [orig, vast_type] : visitor.cache) {
            store_layout(orig, vast_type);
        }

        for (auto [qual_type, vast_type] : visitor.qual_cache) {
            auto [orig, quals] = qual_type.split();
            store_layout(orig, vast_type);
        }

        return dl;
    }

    void driver::finalize(const cc::vast_args &vargs) {
        generator.finalize();

        if (auto type_visitor = visitor->find< default_visitor >()) {
            auto dl = emit_data_layout_blueprint(actx, type_visitor.value());
            emit_data_layout(*mctx, mod, dl);
        }

        if (!vargs.has_option(cc::opt::disable_vast_verifier)) {
            if (!verify()) {
                VAST_FATAL("codegen: module verification error before running vast passes");
            }
        }
    }

    bool driver::verify() {
        return mlir::verify(mod.get()).succeeded();
    }

    std::unique_ptr< codegen_builder > mk_codegen_builder(mcontext_t *mctx) {
        return std::make_unique< codegen_builder >(mctx);
    }

    std::unique_ptr< meta_generator > mk_meta_generator(
        acontext_t *actx, mcontext_t *mctx, const cc::vast_args &vargs
    ) {
        if (vargs.has_option(cc::opt::locs_as_meta_ids))
            return std::make_unique< id_meta_gen >(actx, mctx);
        return std::make_unique< default_meta_gen >(actx, mctx);
    }

    std::unique_ptr< mcontext_t > mk_mcontext() {
        auto mctx = std::make_unique< mcontext_t >();
        mlir::registerAllDialects(*mctx);
        vast::registerAllDialects(*mctx);
        mctx->loadAllAvailableDialects();
        return mctx;
    }

    std::unique_ptr< symbol_generator > mk_symbol_generator(acontext_t &actx) {
        return std::make_unique< default_symbol_mangler >(actx.createMangleContext());
    }

    std::unique_ptr< driver > mk_driver(cc::action_options &opts, const cc::vast_args &vargs, acontext_t &actx) {
        auto mctx = mk_mcontext();
        auto bld  = mk_codegen_builder(mctx.get());
        auto mg   = mk_meta_generator(&actx, mctx.get(), vargs);
        auto sg   = mk_symbol_generator(actx);

        options_t copts = {
            .lang = cc::get_source_language(actx.getLangOpts()),
            .optimization_level = opts.codegen.OptimizationLevel,
            // function emition options
            .has_strict_return   = opts.codegen.StrictReturn,
            // visitor options
            .disable_unsupported = vargs.has_option(cc::opt::disable_unsupported),
        };

        return std::make_unique< driver >(
            actx,
            std::move(mctx),
            std::move(copts),
            std::move(bld),
            std::move(mg),
            std::move(sg)
        );
    }

    std::unique_ptr< codegen_visitor > driver::mk_visitor(const options_t &opts) {
        auto top = std::make_unique< codegen_visitor >(*mctx, *mg, *sg, opts);

        top->visitors.push_back(
            std::make_unique< default_visitor >(
                *mctx, *bld, *mg, *sg, visitor_view(*top)
            )
        );

        if (!opts.disable_unsupported) {
            top->visitors.push_back(
                std::make_unique< unsup_visitor >(
                    *mctx, *bld, *mg, *sg, visitor_view(*top)
                )
            );
        }

        top->visitors.push_back(
            std::make_unique< unreach_visitor >(*mctx, *mg, *sg, opts)
        );

        return top;
    }

    void set_target_triple(owning_module_ref &mod, std::string triple) {
        mlir::OpBuilder bld(mod.get());
        auto attr = bld.getAttr< mlir::StringAttr >(triple);
        mod.get()->setAttr(core::CoreDialect::getTargetTripleAttrName(), attr);
    }

    void set_source_language(owning_module_ref &mod, source_language lang) {
        mlir::OpBuilder bld(mod.get());
        auto attr = bld.getAttr< core::SourceLanguageAttr >(lang);
        mod.get()->setAttr(core::CoreDialect::getLanguageAttrName(), attr);
    }

    string_ref get_path_to_source(acontext_t &actx) {
        // Set the module name to be the name of the main file. TranslationUnitDecl
        // often contains invalid source locations and isn't a reliable source for the
        // module location.
        auto main_file_id = actx.getSourceManager().getMainFileID();
        const auto &main_file = *actx.getSourceManager().getFileEntryForID(main_file_id);
        return main_file.tryGetRealPathName();
    }

    namespace detail
    {
        std::pair< loc_t, std::string > module_loc_name(mcontext_t &mctx, acontext_t &actx) {
            // TODO use meta generator
            if (auto path = get_path_to_source(actx); !path.empty()) {
                return { mlir::FileLineColLoc::get(&mctx, path, 0, 0), path.str() };
            }
            return { mlir::UnknownLoc::get(&mctx), "unknown" };
        }
    } // namespace detail

    owning_module_ref mk_module(acontext_t &actx, mcontext_t &mctx) {
        auto [loc, name] = detail::module_loc_name(mctx, actx);
        auto mod = owning_module_ref(vast_module::create(loc));
        // TODO use symbol generator
        mod->setSymName(name);
        return mod;
    }

    owning_module_ref mk_module_with_attrs(acontext_t &actx, mcontext_t &mctx, source_language lang) {

        auto mod = mk_module(actx, mctx);

        set_target_triple(mod, actx.getTargetInfo().getTriple().str());
        set_source_language(mod, lang);

        return mod;
    }

} // namespace vast::cg
