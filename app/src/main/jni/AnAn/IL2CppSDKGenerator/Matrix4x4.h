#pragma once

#include <math.h>

struct Matrix4x4 {
    union {
        float m[16];  // Array format
        float m4x4[4][4];  // 2D array format
        struct {  // Named elements
            float m00, m10, m20, m30;
            float m01, m11, m21, m31;
            float m02, m12, m22, m32;
            float m03, m13, m23, m33;
        };
    };
    
    // Constructor
    Matrix4x4() {
        memset(m, 0, sizeof(m));
    }
    
    // Multiply point with 3x4 matrix (no perspective divide)
    Vector3 MultiplyPoint3x4(const Vector3& point) const {
        Vector3 result;
        result.x = m[0] * point.x + m[4] * point.y + m[8] * point.z + m[12];
        result.y = m[1] * point.x + m[5] * point.y + m[9] * point.z + m[13];
        result.z = m[2] * point.x + m[6] * point.y + m[10] * point.z + m[14];
        return result;
    }
    
    // Multiply point with full 4x4 matrix (with perspective divide)
    Vector3 MultiplyPoint(const Vector3& point) const {
        Vector3 result;
        float w;
        
        result.x = m[0] * point.x + m[4] * point.y + m[8] * point.z + m[12];
        result.y = m[1] * point.x + m[5] * point.y + m[9] * point.z + m[13];
        result.z = m[2] * point.x + m[6] * point.y + m[10] * point.z + m[14];
        w = m[3] * point.x + m[7] * point.y + m[11] * point.z + m[15];
        
        // Perspective divide
        if (w != 0.0f && w != 1.0f) {
            float invW = 1.0f / w;
            result.x *= invW;
            result.y *= invW;
            result.z *= invW;
        }
        
        return result;
    }
    
    // Get element at row, col
    float Get(int row, int col) const {
        return m4x4[col][row];  // Note: Unity stores column-major
    }
    
    // Set element at row, col
    void Set(int row, int col, float value) {
        m4x4[col][row] = value;
    }
    
    // Identity matrix
    static Matrix4x4 Identity() {
        Matrix4x4 result;
        result.m[0] = result.m[5] = result.m[10] = result.m[15] = 1.0f;
        return result;
    }
};