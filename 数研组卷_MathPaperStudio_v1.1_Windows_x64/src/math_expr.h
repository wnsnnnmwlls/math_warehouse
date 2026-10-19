#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mathexpr {

// A compiled mathematical expression of one variable, x.
//
// Supported syntax:
//   decimal numbers, x, + - * / ^, unary +/-, parentheses,
//   implicit multiplication (2x, 3(x+1), 2sin(x), ...),
//   sin cos tan asin acos atan sqrt abs log (base 10) ln exp floor ceil,
//   constants pi, e, Unicode pi, and the Unicode square-root sign.
// Trigonometric functions use radians.
struct Expression {
    // Parses UTF-8 source. Returns true on success and clears *error.
    // On failure returns false and, when non-null, writes a diagnostic to *error.
    bool Parse(const std::string& utf8Text, std::string* error);

    // True if the last Parse call succeeded.
    bool ok() const { return ok_; }

    // Evaluates the expression at x. Undefined points (sqrt of a negative
    // number, log of a non-positive number, division by zero, ...) return NaN.
    double Evaluate(double x) const;

    // The original, unmodified UTF-8 source text.
    const std::string& Text() const { return text_; }

private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    bool ok_ = false;
    std::string text_;
};

struct RootInfo {
    double x = 0.0;
    double y = 0.0;
};

// Scans [xMin, xMax] for zeros of expression, refining sign changes with
// bisection and also accepting near-zero values at sample points.
// At most 100 roots are returned, in ascending x order, without duplicates.
std::vector<RootInfo> FindRoots(const Expression& expression, double xMin, double xMax,
                                int samples = 1200);

}  // namespace mathexpr
