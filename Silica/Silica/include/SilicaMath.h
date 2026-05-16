#pragma once

namespace Silica {

	struct Vec2 {
		float x = 0.0f;
		float y = 0.0f;

		Vec2() = default;
		Vec2(float x, float y) : x(x), y(y) {}
		Vec2(int x, int y) : x((float)x), y((float)y) {}

		static Vec2 zero() { return Vec2(0.0f, 0.0f); }
		static Vec2 one() { return Vec2(1.0f, 1.0f); }

		Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
		Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
		Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
		Vec2 operator/(float scalar) const { return Vec2(x / scalar, y / scalar); }
		Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
		Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
		Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
		Vec2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }
		bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
		bool operator!=(const Vec2& other) const { return x != other.x || y != other.y; }
	};

	struct Rect {
		float left = 0.0f;
		float right = 0.0f;
		float top = 0.0f;
		float bottom = 0.0f;

		Rect() = default;
		Rect(float left, float right, float top, float bottom) : left(left), right(right), top(top), bottom(bottom) {}

		inline float getWidth() const { return right - left; }
		inline float getHeight() const { return bottom - top; }
		inline bool contains(const Vec2& point) const {
			return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
		}


	};

}
