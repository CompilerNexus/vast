// Copyright (c) 2023-present, Trail of Bits, Inc.

#include "vast/Util/Warnings.hpp"

VAST_RELAX_WARNINGS
#include <mlir/Pass/Pass.h>
#include <mlir/Pass/PassManager.h>
VAST_UNRELAX_WARNINGS

#include "vast/Conversion/Passes.hpp"

namespace vast::conv::pipeline {

    pipeline_step_ptr to_hlbi() {
        return pass(createHLToHLBI);
    }

    pipeline_step_ptr hl_to_ll_cf() {
        // TODO add dependencies
        return pass(createHLToLLCFPass);
    }

    pipeline_step_ptr hl_to_ll_geps() {
        // TODO add dependencies
        return pass(createHLToLLGEPsPass);
    }

    pipeline_step_ptr hl_to_ll_vars() {
        // TODO add dependencies
        return pass(createHLToLLVarsPass);
    }

    pipeline_step_ptr lazy_regions() {
        // TODO add dependencies
        return pass(createHLEmitLazyRegionsPass);
    }

    pipeline_step_ptr hl_to_ll_func() {
        // TODO add dependencies
        return pass(createHLToLLFuncPass);
    }

    pipeline_step_ptr fn_args_to_alloca() {
        return pass(createFnArgsToAllocaPass);
    }

    pipeline_step_ptr lower_value_categories() {
        return pass(createLowerValueCategoriesPass);
    }

    pipeline_step_ptr vars_to_allocas() {
        return pass(createCellsToAllocasPass);
    }

    pipeline_step_ptr to_ll() {
        return compose( "to-ll",
            hl_to_ll_func,
            hl_to_ll_vars,
            hl_to_ll_cf,
            hl_to_ll_geps,
            lazy_regions
        );
    }

    pipeline_step_ptr to_mem() {
        return compose( "to-mem",
            vars_to_allocas,
            fn_args_to_alloca,
            lower_value_categories
        ).depends_on(to_ll);
    }

} // namespace vast::conv::pipeline
