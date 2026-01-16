#include "css_transform.h"
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace litehtml
{

namespace {
    // Trim whitespace
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }

    // Convert to lowercase
    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    // Parse comma or space separated values
    std::vector<std::string> parseArgs(const std::string& args) {
        std::vector<std::string> result;
        std::string current;
        int parenDepth = 0;

        for (char c : args) {
            if (c == '(') {
                parenDepth++;
                current += c;
            } else if (c == ')') {
                parenDepth--;
                current += c;
            } else if ((c == ',' || c == ' ') && parenDepth == 0) {
                std::string trimmed = trim(current);
                if (!trimmed.empty()) {
                    result.push_back(trimmed);
                }
                current.clear();
            } else {
                current += c;
            }
        }

        std::string trimmed = trim(current);
        if (!trimmed.empty()) {
            result.push_back(trimmed);
        }

        return result;
    }

    constexpr float PI = 3.14159265358979323846f;
}

float CSSTransform::parseAngle(const std::string& value) {
    std::string v = toLower(trim(value));

    float num = std::strtof(v.c_str(), nullptr);

    if (v.find("deg") != std::string::npos) {
        return num * PI / 180.0f;
    } else if (v.find("rad") != std::string::npos) {
        return num;
    } else if (v.find("turn") != std::string::npos) {
        return num * 2.0f * PI;
    } else if (v.find("grad") != std::string::npos) {
        return num * PI / 200.0f;
    }

    // Default: degrees
    return num * PI / 180.0f;
}

float CSSTransform::parseLength(const std::string& value, float context) {
    std::string v = toLower(trim(value));

    float num = std::strtof(v.c_str(), nullptr);

    if (v.find('%') != std::string::npos) {
        return num * context / 100.0f;
    } else if (v.find("px") != std::string::npos) {
        return num;
    } else if (v.find("em") != std::string::npos) {
        return num * 16.0f;  // Assume 16px base font
    } else if (v.find("rem") != std::string::npos) {
        return num * 16.0f;
    } else if (v.find("vw") != std::string::npos) {
        return num * context / 100.0f;
    } else if (v.find("vh") != std::string::npos) {
        return num * context / 100.0f;
    }

    // Default: pixels
    return num;
}

float CSSTransform::parseNumber(const std::string& value) {
    return std::strtof(trim(value).c_str(), nullptr);
}

bool CSSTransform::parseFunction(const std::string& func, TransformMatrix& result) {
    size_t parenStart = func.find('(');
    size_t parenEnd = func.rfind(')');

    if (parenStart == std::string::npos || parenEnd == std::string::npos) {
        return false;
    }

    std::string name = toLower(trim(func.substr(0, parenStart)));
    std::string argsStr = func.substr(parenStart + 1, parenEnd - parenStart - 1);
    std::vector<std::string> args = parseArgs(argsStr);

    if (name == "translate") {
        float tx = args.size() > 0 ? parseLength(args[0]) : 0;
        float ty = args.size() > 1 ? parseLength(args[1]) : 0;
        result = TransformMatrix::translate(tx, ty);
        return true;
    } else if (name == "translatex") {
        float tx = args.size() > 0 ? parseLength(args[0]) : 0;
        result = TransformMatrix::translate(tx, 0);
        return true;
    } else if (name == "translatey") {
        float ty = args.size() > 0 ? parseLength(args[0]) : 0;
        result = TransformMatrix::translate(0, ty);
        return true;
    } else if (name == "scale") {
        float sx = args.size() > 0 ? parseNumber(args[0]) : 1;
        float sy = args.size() > 1 ? parseNumber(args[1]) : sx;
        result = TransformMatrix::scale(sx, sy);
        return true;
    } else if (name == "scalex") {
        float sx = args.size() > 0 ? parseNumber(args[0]) : 1;
        result = TransformMatrix::scale(sx, 1);
        return true;
    } else if (name == "scaley") {
        float sy = args.size() > 0 ? parseNumber(args[0]) : 1;
        result = TransformMatrix::scale(1, sy);
        return true;
    } else if (name == "rotate") {
        float angle = args.size() > 0 ? parseAngle(args[0]) : 0;
        result = TransformMatrix::rotate(angle);
        return true;
    } else if (name == "skew") {
        float ax = args.size() > 0 ? parseAngle(args[0]) : 0;
        float ay = args.size() > 1 ? parseAngle(args[1]) : 0;
        result = TransformMatrix::skewX(ax).multiply(TransformMatrix::skewY(ay));
        return true;
    } else if (name == "skewx") {
        float ax = args.size() > 0 ? parseAngle(args[0]) : 0;
        result = TransformMatrix::skewX(ax);
        return true;
    } else if (name == "skewy") {
        float ay = args.size() > 0 ? parseAngle(args[0]) : 0;
        result = TransformMatrix::skewY(ay);
        return true;
    } else if (name == "matrix") {
        if (args.size() >= 6) {
            result.a = parseNumber(args[0]);
            result.b = parseNumber(args[1]);
            result.c = parseNumber(args[2]);
            result.d = parseNumber(args[3]);
            result.e = parseNumber(args[4]);
            result.f = parseNumber(args[5]);
            return true;
        }
    }

    return false;
}

TransformMatrix CSSTransform::parse(const std::string& transformStr) {
    TransformMatrix result = TransformMatrix::identity();

    if (transformStr.empty() || toLower(trim(transformStr)) == "none") {
        return result;
    }

    // Parse transform functions one by one
    // Example: "rotate(45deg) scale(2) translateX(10px)"
    std::string remaining = transformStr;

    while (!remaining.empty()) {
        remaining = trim(remaining);
        if (remaining.empty()) break;

        // Find the function name and opening paren
        size_t parenStart = remaining.find('(');
        if (parenStart == std::string::npos) break;

        // Find matching closing paren
        int depth = 1;
        size_t parenEnd = parenStart + 1;
        while (parenEnd < remaining.size() && depth > 0) {
            if (remaining[parenEnd] == '(') depth++;
            else if (remaining[parenEnd] == ')') depth--;
            parenEnd++;
        }
        parenEnd--;  // Point to the closing paren

        if (depth != 0) break;  // Mismatched parens

        // Extract the function
        std::string func = remaining.substr(0, parenEnd + 1);

        // Parse and apply the transform
        TransformMatrix funcMatrix;
        if (parseFunction(func, funcMatrix)) {
            result = result.multiply(funcMatrix);
        }

        // Move to next function
        remaining = remaining.substr(parenEnd + 1);
    }

    return result;
}

} // namespace litehtml
