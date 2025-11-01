#include <iostream>
#include <cstddef>
#include <cstdint>

struct SampicHitScalars {
    int32_t raw_tot_value;
    float tot_value;
    float amplitude;
    float baseline;
    float peak;
    float time_index;
    double time_instant;
    float time_amplitude;
    double first_cell_timestamp;
};

#pragma pack(push, 1)
struct SampicHitScalarsPacked {
    int32_t raw_tot_value;
    float tot_value;
    float amplitude;
    float baseline;
    float peak;
    float time_index;
    double time_instant;
    float time_amplitude;
    double first_cell_timestamp;
};
#pragma pack(pop)

int main() {
    std::cout << "sizeof(SampicHitScalars) normal = " << sizeof(SampicHitScalars) << std::endl;
    std::cout << "sizeof(SampicHitScalarsPacked) packed = " << sizeof(SampicHitScalarsPacked) << std::endl;

    std::cout << "\nField sizes:" << std::endl;
    std::cout << "int32_t: " << sizeof(int32_t) << std::endl;
    std::cout << "float: " << sizeof(float) << std::endl;
    std::cout << "double: " << sizeof(double) << std::endl;

    std::cout << "\nExpected manual calculation:" << std::endl;
    std::cout << "4 + 4 + 4 + 4 + 4 + 4 + 8 + 4 + 8 = " << (4+4+4+4+4+4+8+4+8) << std::endl;

    return 0;
}
