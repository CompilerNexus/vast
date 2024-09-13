// Copyright (c) 2021-present, Trail of Bits, Inc.

#include "vast/Conversion/Passes.hpp"

VAST_RELAX_WARNINGS
#include <mlir/IR/PatternMatch.h>
#include <mlir/Transforms/DialectConversion.h>
#include <mlir/Transforms/GreedyPatternRewriteDriver.h>
VAST_UNRELAX_WARNINGS

#include "../PassesDetails.hpp"

#include "vast/Dialect/LowLevel/LowLevelOps.hpp"

#include "vast/Conversion/Common/Mixins.hpp"
#include "vast/Conversion/Common/Patterns.hpp"

#include "vast/Util/Common.hpp"


namespace vast::conv {

    namespace pattern {
        struct cell_to_alloca : replace_pattern< ll::Cell, ll::Alloca > {
            using base = replace_pattern< ll::Cell, ll::Alloca >;
            using base::base;

            operation replacement(ll::Cell op, adaptor_t, conversion_rewriter &) const override {
                return rewriter.create< ll::Alloca >(op.getLoc(), tc.convert_type_to_type(op.getType()));
            }
        };
    } // namespace patterns


    struct CellsToAllocas : ModuleConversionPassMixin< CellsToAllocas, CellsToAllocasBase >
    {
        using base = ModuleConversionPassMixin< CellsToAllocas, CellsToAllocasBase >;

        static void populate_conversions(auto &cfg) {
            // // base::populate_conversions< pattern::cell_to_alloca >(cfg);
        }
    };

} // namespace vast::conv

std::unique_ptr< mlir::Pass > vast::createCellsToAllocasPass() {
    return std::make_unique< vast::conv::CellsToAllocas >();
}
