#include <iostream>
#include <cstddef>
#include <cstdint>

// Mimic exactly what's in HitStruct
struct TestScalars {
    int RawTOTValue;  // int, not int32_t
    float TOTValue;
    float Amplitude;
    float Baseline;
    float Peak;
    float TimeIndex;
    double TimeInstant;
    float TimeAmplitude;
    double FirstCellTimeStamp;
};

int main() {
    std::cout << "sizeof(TestScalars) = " << sizeof(TestScalars) << std::endl;
    std::cout << "sizeof(int) = " << sizeof(int) << std::endl;
    std::cout << "sizeof(float) = " << sizeof(float) << std::endl;
    std::cout << "sizeof(double) = " << sizeof(double) << std::endl;

    // Calculate offsets
    TestScalars s;
    std::cout << "\nField offsets:" << std::endl;
    std::cout << "RawTOTValue: " << offsetof(TestScalars, RawTOTValue) << std::endl;
    std::cout << "TOTValue: " << offsetof(TestScalars, TOTValue) << std::endl;
    std::cout << "Amplitude: " << offsetof(TestScalars, Amplitude) << std::endl;
    std::cout << "Baseline: " << offsetof(TestScalars, Baseline) << std::endl;
    std::cout << "Peak: " << offsetof(TestScalars, Peak) << std::endl;
    std::cout << "TimeIndex: " << offsetof(TestScalars, TimeIndex) << std::endl;
    std::cout << "TimeInstant: " << offsetof(TestScalars, TimeInstant) << std::endl;
    std::cout << "TimeAmplitude: " << offsetof(TestScalars, TimeAmplitude) << std::endl;
    std::cout << "FirstCellTimeStamp: " << offsetof(TestScalars, FirstCellTimeStamp) << std::endl;

    return 0;
}
