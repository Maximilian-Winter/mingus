#pragma once

// ============================================================================
// DebugInfo.h — Full source range tracking for AST nodes and diagnostics
//
// Every AST node carries a DebugInfo with both a primary location (for quick
// reference) and a full range (for precise error underlines and debug info).
// ============================================================================

#include <memory>
#include <string>

namespace mingus {

class DebugInfo : public std::enable_shared_from_this<DebugInfo> {
public:
    DebugInfo() = default;

    DebugInfo(int line, int col)
        : lineNumber(line)
        , columnNumber(col)
        , lineNumberStart(line)
        , lineNumberEnd(line)
        , columnNumberStart(col)
        , columnNumberEnd(col) {}

    DebugInfo(int lineStart, int colStart, int lineEnd, int colEnd)
        : lineNumber(lineStart)
        , columnNumber(colStart)
        , lineNumberStart(lineStart)
        , lineNumberEnd(lineEnd)
        , columnNumberStart(colStart)
        , columnNumberEnd(colEnd) {}

    // Primary location (for quick reference in error messages)
    int lineNumber = 0;
    int columnNumber = 0;

    // Full source range (for underlines, hover info, debug info emission)
    int lineNumberStart = 0;
    int lineNumberEnd = 0;
    int columnNumberStart = 0;
    int columnNumberEnd = 0;

    // Source file (set once per compilation unit)
    std::string sourceFile;

    // Convenience: format as "file:line:col"
    std::string toString() const {
        std::string result;
        if (!sourceFile.empty()) {
            result += sourceFile + ":";
        }
        result += std::to_string(lineNumber) + ":" + std::to_string(columnNumber);
        return result;
    }

    // Convenience: format as "line:col-line:col" range
    std::string rangeString() const {
        return std::to_string(lineNumberStart) + ":" + std::to_string(columnNumberStart)
             + "-"
             + std::to_string(lineNumberEnd) + ":" + std::to_string(columnNumberEnd);
    }

    // Factory: create from ANTLR4 token
    static std::shared_ptr<DebugInfo> fromToken(int line, int col, int length) {
        auto info = std::make_shared<DebugInfo>();
        info->lineNumber = line;
        info->columnNumber = col;
        info->lineNumberStart = line;
        info->lineNumberEnd = line;
        info->columnNumberStart = col;
        info->columnNumberEnd = col + length;
        return info;
    }

    // Factory: merge two ranges (smallest start to largest end)
    static std::shared_ptr<DebugInfo> merge(
        const std::shared_ptr<DebugInfo>& a,
        const std::shared_ptr<DebugInfo>& b)
    {
        if (!a) return b;
        if (!b) return a;

        auto merged = std::make_shared<DebugInfo>();
        // Take the earlier start
        if (a->lineNumberStart < b->lineNumberStart ||
            (a->lineNumberStart == b->lineNumberStart &&
             a->columnNumberStart <= b->columnNumberStart)) {
            merged->lineNumberStart = a->lineNumberStart;
            merged->columnNumberStart = a->columnNumberStart;
        } else {
            merged->lineNumberStart = b->lineNumberStart;
            merged->columnNumberStart = b->columnNumberStart;
        }
        // Take the later end
        if (a->lineNumberEnd > b->lineNumberEnd ||
            (a->lineNumberEnd == b->lineNumberEnd &&
             a->columnNumberEnd >= b->columnNumberEnd)) {
            merged->lineNumberEnd = a->lineNumberEnd;
            merged->columnNumberEnd = a->columnNumberEnd;
        } else {
            merged->lineNumberEnd = b->lineNumberEnd;
            merged->columnNumberEnd = b->columnNumberEnd;
        }
        // Primary = start of range
        merged->lineNumber = merged->lineNumberStart;
        merged->columnNumber = merged->columnNumberStart;
        merged->sourceFile = a->sourceFile.empty() ? b->sourceFile : a->sourceFile;
        return merged;
    }
};

} // namespace mingus
