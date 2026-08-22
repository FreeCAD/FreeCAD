// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <optional>
#include <memory>
#include <string>
#include <string_view>

#include <FCGlobal.h>

#include <Base/Quantity.h>

namespace App
{
class ObjectIdentifier;
class Expression;

enum class InputPhase
{
    Editing,
    Commit
};

enum class InputStatus
{
    Acceptable,
    Incomplete,
    Invalid
};

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
    std::size_t offset {};
    std::size_t length {1};
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
    std::optional<Base::Quantity> quantity;
    std::shared_ptr<Expression> expression;
    std::string normalizedText;
    std::optional<InputDiagnostic> diagnostic;
};

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
