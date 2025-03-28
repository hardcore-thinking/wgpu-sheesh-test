#include <Matrix4x4.hpp>

Matrix4x4::Matrix4x4() {
	_elements.fill(0.0f);
}

Matrix4x4::Matrix4x4(float e00, float e01, float e02, float e03,
					 float e10, float e11, float e12, float e13,
					 float e20, float e21, float e22, float e23,
					 float e30, float e31, float e32, float e33) {
	_elements = {
		e00, e01, e02, e03,
		e10, e11, e12, e13,
		e20, e21, e22, e23,
		e30, e31, e32, e33,
	};
}

Matrix4x4 Matrix4x4::Identity() {
	Matrix4x4 result;
	result._elements = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
	
	return result;
}

Matrix4x4 Matrix4x4::Transpose(Matrix4x4 const& mat) {
	Matrix4x4 result;
	std::array<float, 16> const& e = mat._elements;
	
	result._elements = {
		e[0], e[4],  e[8], e[12],
		e[1], e[5],  e[9], e[13],
		e[2], e[6], e[10], e[14],
		e[3], e[7], e[11], e[15],
	};

	return result;
}

float Matrix4x4::Element(int n, int m) const {
	return _elements[n * 4 + m];
}

std::array<float, 16> Matrix4x4::Elements() const {
	return _elements;
}

Vec4 Matrix4x4::Line(int n) const {
	return Vec4(_elements[n * 4 + 0], _elements[n * 4 + 1], _elements[n * 4 + 2], _elements[n * 4 + 3]);
}

Vec4 Matrix4x4::Column(int m) const {
	return Vec4(_elements[0 * 4 + m], _elements[1 * 4 + m], _elements[2 * 4 + m], _elements[3 * 4 + m]);
}

float const* Matrix4x4::Data() const {
	return _elements.data();
}

Matrix4x4 Matrix4x4::operator+(Matrix4x4 const& other) const {
	Matrix4x4 result;
	for (int i = 0; i < 16; ++i) {
		result._elements[i] = _elements[i] + other._elements[i];
	}
	return result;
}

Matrix4x4 Matrix4x4::operator-() const {
	Matrix4x4 result;
	for (int i = 0; i < 16; ++i) {
		result._elements[i] = -_elements[i];
	}
	return result;
}

Matrix4x4 Matrix4x4::operator-(Matrix4x4 const& other) const {
	return *this + -other;
}

Matrix4x4 Matrix4x4::operator*(Matrix4x4 const& other) const {
	Matrix4x4 result;
	
	result._elements[0]  = Element(0, 0) * other.Element(0, 0) + Element(0, 1) * other.Element(1, 0) + Element(0, 2) * other.Element(2, 0) + Element(0, 3) * other.Element(3, 0);
	result._elements[1]  = Element(0, 0) * other.Element(0, 1) + Element(0, 1) * other.Element(1, 1) + Element(0, 2) * other.Element(2, 1) + Element(0, 3) * other.Element(3, 1);
	result._elements[2]  = Element(0, 0) * other.Element(0, 2) + Element(0, 1) * other.Element(1, 2) + Element(0, 2) * other.Element(2, 2) + Element(0, 3) * other.Element(3, 2);
	result._elements[3]  = Element(0, 0) * other.Element(0, 3) + Element(0, 1) * other.Element(1, 3) + Element(0, 2) * other.Element(2, 3) + Element(0, 3) * other.Element(3, 3);

	result._elements[4]  = Element(1, 0) * other.Element(0, 0) + Element(1, 1) * other.Element(1, 0) + Element(1, 2) * other.Element(2, 0) + Element(1, 3) * other.Element(3, 0);
	result._elements[5]  = Element(1, 0) * other.Element(0, 1) + Element(1, 1) * other.Element(1, 1) + Element(1, 2) * other.Element(2, 1) + Element(1, 3) * other.Element(3, 1);
	result._elements[6]  = Element(1, 0) * other.Element(0, 2) + Element(1, 1) * other.Element(1, 2) + Element(1, 2) * other.Element(2, 2) + Element(1, 3) * other.Element(3, 2);
	result._elements[7]  = Element(1, 0) * other.Element(0, 3) + Element(1, 1) * other.Element(1, 3) + Element(1, 2) * other.Element(2, 3) + Element(1, 3) * other.Element(3, 3);

	result._elements[8]  = Element(2, 0) * other.Element(0, 0) + Element(2, 1) * other.Element(1, 0) + Element(2, 2) * other.Element(2, 0) + Element(2, 3) * other.Element(3, 0);
	result._elements[9]  = Element(2, 0) * other.Element(0, 1) + Element(2, 1) * other.Element(1, 1) + Element(2, 2) * other.Element(2, 1) + Element(2, 3) * other.Element(3, 1);
	result._elements[10] = Element(2, 0) * other.Element(0, 2) + Element(2, 1) * other.Element(1, 2) + Element(2, 2) * other.Element(2, 2) + Element(2, 3) * other.Element(3, 2);
	result._elements[11] = Element(2, 0) * other.Element(0, 3) + Element(2, 1) * other.Element(1, 3) + Element(2, 2) * other.Element(2, 3) + Element(2, 3) * other.Element(3, 3);

	result._elements[12] = Element(3, 0) * other.Element(0, 0) + Element(3, 1) * other.Element(1, 0) + Element(3, 2) * other.Element(2, 0) + Element(3, 3) * other.Element(3, 0);
	result._elements[13] = Element(3, 0) * other.Element(0, 1) + Element(3, 1) * other.Element(1, 1) + Element(3, 2) * other.Element(2, 1) + Element(3, 3) * other.Element(3, 1);
	result._elements[14] = Element(3, 0) * other.Element(0, 2) + Element(3, 1) * other.Element(1, 2) + Element(3, 2) * other.Element(2, 2) + Element(3, 3) * other.Element(3, 2);
	result._elements[15] = Element(3, 0) * other.Element(0, 3) + Element(3, 1) * other.Element(1, 3) + Element(3, 2) * other.Element(2, 3) + Element(3, 3) * other.Element(3, 3);

	return result;
}

std::ostream& operator<<(std::ostream& os, Matrix4x4 const& mat) {
	os << "[";
	for (int i = 0; i < 4; ++i) {
		os << "[";
		for (int j = 0; j < 4; ++j) {
			os << mat.Element(i, j);
			if (j < 3) {
				os << ", ";
			}
		}
		os << "]";
		if (i < 3) {
			os << ", ";
		}
		std::cout << std::endl;
	}
	os << "]";
	return os;
}