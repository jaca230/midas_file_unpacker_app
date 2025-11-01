#include <iostream>
#include <cstddef>
#include <cstdint>

// Copy the structure definitions from SAMPIC library
#define MAX_NB_OF_SAMPLES 64

typedef int Boolean;

struct AdvancedHitStruct {
    // Placeholder - we don't care about the contents
    char dummy[100];
};

struct HitStruct {
    int FeBoardIndex;
    int Channel;
    int HitNumber;
    int SampicIndex;
    int ChannelIndex;
    int DataSize;
    Boolean INLCorrected;
    Boolean ADCCorrected;
    Boolean ResidualPedestalCorrected;
    int CellInfo;
    int FirstCellPhysicalIndex;
    unsigned short RawDataSamples[MAX_NB_OF_SAMPLES];
    unsigned short OrderedRawDataSamples[MAX_NB_OF_SAMPLES];
    float CorrectedDataSamples[MAX_NB_OF_SAMPLES];
    int RawTOTValue;
    float TOTValue;
    float Amplitude;
    float Baseline;
    float Peak;
    float TimeIndex;
    double TimeInstant;
    float TimeAmplitude;
    double FirstCellTimeStamp;
    AdvancedHitStruct AdvancedParams;
};

int main() {
    std::cout << "sizeof(HitStruct) = " << sizeof(HitStruct) << std::endl;
    std::cout << "offsetof(HitStruct, RawDataSamples) = " << offsetof(HitStruct, RawDataSamples) << std::endl;
    std::cout << "offsetof(HitStruct, CorrectedDataSamples) = " << offsetof(HitStruct, CorrectedDataSamples) << std::endl;
    std::cout << "offsetof(HitStruct, RawTOTValue) = " << offsetof(HitStruct, RawTOTValue) << std::endl;
    std::cout << "offsetof(HitStruct, AdvancedParams) = " << offsetof(HitStruct, AdvancedParams) << std::endl;

    size_t header_size = offsetof(HitStruct, RawDataSamples);
    size_t corrected_section_start = offsetof(HitStruct, CorrectedDataSamples);
    size_t corrected_section_end = offsetof(HitStruct, AdvancedParams);
    size_t corrected_section_size = corrected_section_end - corrected_section_start;

    std::cout << "\nHeader size (up to RawDataSamples): " << header_size << std::endl;
    std::cout << "Corrected section size (CorrectedDataSamples to AdvancedParams): " << corrected_section_size << std::endl;

    size_t waveform_size = MAX_NB_OF_SAMPLES * sizeof(float);
    size_t scalars_size = corrected_section_size - waveform_size;
    std::cout << "Waveform size (64 floats): " << waveform_size << std::endl;
    std::cout << "Scalars size (after waveform): " << scalars_size << std::endl;

    return 0;
}
