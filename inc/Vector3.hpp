#ifndef VECTOR3_HPP
#define VECTOR3_HPP

class Vector3 {
public:
	Vector3();
	Vector3(float x, float y, float z);
	~Vector3() = default;

public:
	float X() const;
	float Y() const;
	float Z() const;

	float R() const;
	float G() const;
	float B() const;

	Vector3 operator+(Vector3 const& other) const;
	Vector3 operator-(Vector3 const& other) const;
	Vector3 operator*(float scalar) const;
	Vector3 operator/(float scalar) const;

	static float Dot(Vector3 const& a, Vector3 const& b);

private:
	float _x = 0.0f;
	float _y = 0.0f;
	float _z = 0.0f;
};

#endif // !VECTOR3_HPP
