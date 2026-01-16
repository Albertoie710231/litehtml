#ifndef LITEHTML_CSS_TRANSFORM_H
#define LITEHTML_CSS_TRANSFORM_H

#include <string>
#include <cmath>
#include <algorithm>

namespace litehtml
{

// 2D Transformation Matrix
// [ a  c  e ]   [ x ]   [ a*x + c*y + e ]
// [ b  d  f ] * [ y ] = [ b*x + d*y + f ]
// [ 0  0  1 ]   [ 1 ]   [       1       ]
struct TransformMatrix {
    float a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;

    // Identity matrix
    static TransformMatrix identity() {
        return {1, 0, 0, 1, 0, 0};
    }

    // Translation
    static TransformMatrix translate(float tx, float ty) {
        return {1, 0, 0, 1, tx, ty};
    }

    // Scale
    static TransformMatrix scale(float sx, float sy) {
        return {sx, 0, 0, sy, 0, 0};
    }

    // Rotation (angle in radians)
    static TransformMatrix rotate(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        return {c, s, -s, c, 0, 0};
    }

    // Skew X (angle in radians)
    static TransformMatrix skewX(float angle) {
        return {1, 0, std::tan(angle), 1, 0, 0};
    }

    // Skew Y (angle in radians)
    static TransformMatrix skewY(float angle) {
        return {1, std::tan(angle), 0, 1, 0, 0};
    }

    // Matrix multiplication: this * other
    TransformMatrix multiply(const TransformMatrix& other) const {
        return {
            a * other.a + c * other.b,
            b * other.a + d * other.b,
            a * other.c + c * other.d,
            b * other.c + d * other.d,
            a * other.e + c * other.f + e,
            b * other.e + d * other.f + f
        };
    }

    // Transform a point
    void apply(float& x, float& y) const {
        float newX = a * x + c * y + e;
        float newY = b * x + d * y + f;
        x = newX;
        y = newY;
    }

    // Transform a rectangle (returns bounding box of transformed corners)
    void applyToRect(float& x, float& y, float& width, float& height) const {
        // Get four corners
        float x1 = x, y1 = y;
        float x2 = x + width, y2 = y;
        float x3 = x + width, y3 = y + height;
        float x4 = x, y4 = y + height;

        // Transform corners
        apply(x1, y1);
        apply(x2, y2);
        apply(x3, y3);
        apply(x4, y4);

        // Find bounding box
        float minX = std::min(std::min(x1, x2), std::min(x3, x4));
        float maxX = std::max(std::max(x1, x2), std::max(x3, x4));
        float minY = std::min(std::min(y1, y2), std::min(y3, y4));
        float maxY = std::max(std::max(y1, y2), std::max(y3, y4));

        x = minX;
        y = minY;
        width = maxX - minX;
        height = maxY - minY;
    }

    bool isIdentity() const {
        return a == 1 && b == 0 && c == 0 && d == 1 && e == 0 && f == 0;
    }
};

// CSS Transform parser
class CSSTransform {
public:
    // Parse CSS transform string like "rotate(45deg) scale(2) translateX(10px)"
    static TransformMatrix parse(const std::string& transformStr);

    // Parse a single transform function
    static bool parseFunction(const std::string& func, TransformMatrix& result);

private:
    // Parse angle value (returns radians)
    static float parseAngle(const std::string& value);

    // Parse length value (returns pixels, percentage needs context)
    static float parseLength(const std::string& value, float context = 0);

    // Parse number
    static float parseNumber(const std::string& value);
};

} // namespace litehtml

#endif // LITEHTML_CSS_TRANSFORM_H
