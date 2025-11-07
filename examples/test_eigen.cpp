/**
 * Eigen Test - Matrix operations and performance
 */
#include <eigen3/Eigen/Dense>
#include <iostream>
#include <chrono>

using namespace Eigen;

int main() {
    std::cout << "=== Eigen Test ===" << std::endl;
    std::cout << "Eigen version: " << EIGEN_WORLD_VERSION << "."
              << EIGEN_MAJOR_VERSION << "." << EIGEN_MINOR_VERSION << std::endl;

    // Test basic matrix operations
    std::cout << "\n=== Basic Matrix Operations ===" << std::endl;

    Matrix3d m = Matrix3d::Random();
    std::cout << "Random 3x3 matrix:\n" << m << std::endl;

    std::cout << "\nTranspose:\n" << m.transpose() << std::endl;
    std::cout << "\nDeterminant: " << m.determinant() << std::endl;

    // Test vector operations
    std::cout << "\n=== Vector Operations ===" << std::endl;

    VectorXd v1(5);
    v1 << 1, 2, 3, 4, 5;

    VectorXd v2(5);
    v2 << 5, 4, 3, 2, 1;

    std::cout << "v1: " << v1.transpose() << std::endl;
    std::cout << "v2: " << v2.transpose() << std::endl;
    std::cout << "Dot product: " << v1.dot(v2) << std::endl;

    // Cross product requires fixed-size vectors
    Vector3d v1_3d = v1.head(3);
    Vector3d v2_3d = v2.head(3);
    std::cout << "Cross product (first 3 elements): "
              << v1_3d.cross(v2_3d).transpose() << std::endl;

    // Performance test
    std::cout << "\n=== Performance Test ===" << std::endl;

    const int size = 1000;
    const int iterations = 100;

    MatrixXd A = MatrixXd::Random(size, size);
    MatrixXd B = MatrixXd::Random(size, size);

    auto start = std::chrono::high_resolution_clock::now();

    MatrixXd C;
    for (int i = 0; i < iterations; ++i) {
        C = A * B;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Matrix multiplication (" << size << "x" << size << "): "
              << (duration.count() / double(iterations)) << " ms average" << std::endl;

    std::cout << "\n✓ Eigen test PASSED" << std::endl;
    return 0;
}
