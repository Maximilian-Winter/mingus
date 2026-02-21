#pragma once

// ============================================================================
// ErrorReporter.h — Diagnostic collection for sema passes
// ============================================================================

#include "mingus/DebugInfo.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace mingus {

enum class DiagnosticLevel {
    Error,
    Warning,
    Note
};

struct Diagnostic {
    DiagnosticLevel level;
    std::string message;
    std::shared_ptr<DebugInfo> location;
};

class ErrorReporter {
public:
    void error(const std::string& message, const std::shared_ptr<DebugInfo>& loc = nullptr) {
        diagnostics_.push_back({DiagnosticLevel::Error, message, loc});
        errorCount_++;
    }

    void warning(const std::string& message, const std::shared_ptr<DebugInfo>& loc = nullptr) {
        diagnostics_.push_back({DiagnosticLevel::Warning, message, loc});
    }

    void note(const std::string& message, const std::shared_ptr<DebugInfo>& loc = nullptr) {
        diagnostics_.push_back({DiagnosticLevel::Note, message, loc});
    }

    bool hasErrors() const { return errorCount_ > 0; }
    int errorCount() const { return errorCount_; }
    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }

    void dump(std::ostream& out = std::cerr) const {
        for (const auto& d : diagnostics_) {
            switch (d.level) {
                case DiagnosticLevel::Error:   out << "error: "; break;
                case DiagnosticLevel::Warning: out << "warning: "; break;
                case DiagnosticLevel::Note:    out << "note: "; break;
            }
            if (d.location) {
                out << d.location->toString() << ": ";
            }
            out << d.message << "\n";
        }
    }

    void clear() {
        diagnostics_.clear();
        errorCount_ = 0;
    }

private:
    std::vector<Diagnostic> diagnostics_;
    int errorCount_ = 0;
};

} // namespace mingus
