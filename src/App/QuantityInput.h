// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <optional>
#include <memory>
#include <string_view>

#include <FCGlobal.h>

#include <Base/Quantity.h>

namespace App
{
class ObjectIdentifier;
class Expression;

/** Validation phase for user-entered quantity text. */
enum class InputPhase
{
    /// Preserve correctable partial input without committing a value.
    Editing,
    /// Require a complete value suitable for committing to the consumer.
    Commit
};

/** Structured outcome of quantity interpretation. */
enum class InputStatus
{
    Acceptable,
    Incomplete,
    Invalid
};

/** Canonical grammar accepted at this input boundary. */
enum class QuantityInputGrammar
{
    Quantity,
    Expression
};

enum class InputDiagnosticKind
{
    IncompleteNumber,
    MalformedGrouping,
    InvalidNumber,
    ExpressionSyntax,
    Evaluation,
    IncompatibleUnit,
    OutOfRange
};

struct AppExport InputDiagnostic
{
    InputDiagnosticKind kind {InputDiagnosticKind::ExpressionSyntax};
    /// Offset of the diagnostic range in the original UTF-8 input.
    std::size_t offsetBytes {};
    /// Length of the diagnostic range in UTF-8 bytes.
    std::size_t lengthBytes {1};
};

struct AppExport QuantityConstraints
{
    std::optional<Base::Unit> requiredUnit;
    std::optional<double> minimum;
    std::optional<double> maximum;
};

struct AppExport QuantityInputResult
{
    InputStatus status {InputStatus::Invalid};
    /// Interpreted quantity when status is Acceptable.
    std::optional<Base::Quantity> quantity;
    /// Parsed expression retained for expression-capable consumers.
    std::shared_ptr<Expression> expression;
    std::optional<InputDiagnostic> diagnostic;
};

/**
 * Interpret user-entered quantity text at the shared App input boundary.
 *
 * This operation owns localized numeric scanning, parsing, expression evaluation, default-unit
 * application, unit compatibility, range validation, and structured diagnostics. Editing accepts
 * correctable incomplete input; Commit requires a complete acceptable quantity. The caller must
 * explicitly select the accepted grammar and provide the locale used by its input surface.
 */
AppExport QuantityInputResult interpretQuantityInput(
    std::string_view input,
    QuantityInputGrammar grammar,
    const ObjectIdentifier& path,
    const Base::Unit& defaultUnit,
    const Base::NumericLocaleContext& locale,
    InputPhase phase,
    const QuantityConstraints& constraints
);

}  // namespace App
