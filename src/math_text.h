#pragma once

#include <string>

// Converts common LaTeX fragments into human-readable Unicode math text.
// Examples:
//   $\frac{\sqrt{2}}{2}$                  -> √2/2
//   \cos\left(\alpha+\frac{\pi}{4}\right) -> cos(α+π/4)
//   \frac{\cos 2\alpha}{\sin\alpha+\cos\alpha}
//                                          -> (cos 2α)/(sin α+cos α)
std::string NormalizeMathText(const std::string& text);
