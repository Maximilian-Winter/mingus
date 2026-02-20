//================================================================================
// MINGUS v1 - Error Reporter
// Collects diagnostics (errors and warnings) during semantic analysis
//================================================================================

#pragma once

#include "mingus/ast/ASTNode.h"

#include <string>
#include <vector>

namespace mingus {
namespace sema {

using namespace mingus::ast;

//================================================================================
// Severity — error or warning
//================================================================================
enum class Severity {
    Error,
    Warning
};

//================================================================================
// Diagnostic — a single error or warning message
//================================================================================
struct Diagnostic {
    Severity severity;
    SourceLocation location;
    std::string message;

    Diagnostic(Severity sev, const SourceLocation& loc, const std::string& msg)
        : severity(sev), location(loc), message(msg) {}

    std::string toString() const {
        std::string prefix = (severity == Severity::Error) ? "error" : "warning";
        return location.toString() + ": " + prefix + ": " + message;
    }
};

//================================================================================
// ErrorReporter — collects and reports diagnostics
//================================================================================
class ErrorReporter {
public:
    void error(const SourceLocation& loc, const std::string& message) {
        diagnostics_.emplace_back(Severity::Error, loc, message);
        ++errorCount_;
    }

    void warning(const SourceLocation& loc, const std::string& message) {
        diagnostics_.emplace_back(Severity::Warning, loc, message);
        ++warningCount_;
    }

    bool hasErrors() const { return errorCount_ > 0; }
    int getErrorCount() const { return errorCount_; }
    int getWarningCount() const { return warningCount_; }

    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics_; }

    void clear() {
        diagnostics_.clear();
        errorCount_ = 0;
        warningCount_ = 0;
    }

private:
    std::vector<Diagnostic> diagnostics_;
    int errorCount_ = 0;
    int warningCount_ = 0;
};

} // namespace sema
} // namespace mingus
