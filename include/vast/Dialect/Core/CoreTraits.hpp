// Copyright (c) 2022-present, Trail of Bits, Inc.

#pragma once

#include "vast/Util/Warnings.hpp"

VAST_RELAX_WARNINGS
#include <mlir/IR/OpDefinition.h>
VAST_UNRELAX_WARNINGS

#include "vast/Util/Common.hpp"

namespace vast::core
{
    template< typename ConcreteType, template< typename > class Derived >
    using op_trait_base = mlir::OpTrait::TraitBase< ConcreteType, Derived >;

    template< typename ConcreteType, template< typename > class Derived >
    using attr_trait_base = mlir::AttributeTrait::TraitBase< ConcreteType, Derived >;

    //
    // SoftTerminator
    //
    template< typename ConcreteType >
    struct SoftTerminator : op_trait_base< ConcreteType, SoftTerminator > {};

    static inline bool is_soft_terminator(operation op) {
        return op->hasTrait< SoftTerminator >();
    }

    //
    // ReturnLikeOpTrait
    //
    template< typename ConcreteType >
    struct ReturnLikeOpTrait : op_trait_base< ConcreteType, ReturnLikeOpTrait > {};

    static inline bool is_return(mlir::Operation *op) {
        return op->hasTrait< ReturnLikeOpTrait >();
    }
} // namespace vast::core

