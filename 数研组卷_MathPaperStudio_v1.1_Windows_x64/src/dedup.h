#pragma once

#include "model.h"

#include <cstddef>
#include <string>
#include <vector>

struct DuplicateGroup {
    std::string canonicalKey;
    std::vector<size_t> questionIndices;
};

// Canonical form used for duplicate detection. It intentionally ignores spacing,
// sentence punctuation, multiplication signs, full-width punctuation and common
// notation differences (for example "cos7.5°cos52.5°" and
// "cos7.5°*cos52.5°" compare equal).
std::string CanonicalQuestionKey(const std::string& stem);

// Returns groups containing two or more canonically identical questions.
std::vector<DuplicateGroup> FindDuplicateGroups(const std::vector<Question>& questions);

// Adds an explicit middle dot between obvious implicit multiplications such as
// "7.5°cos 52.5°", "2sin x" and ")(x+1)". It is conservative and only inserts
// in places where the expression would otherwise be ambiguous.
std::string InsertImplicitMultiplication(const std::string& text);
