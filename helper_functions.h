#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <string>

std::string lead_character(const std::string& string, const int& number_of_characters, const char& leading_character);
std::string lead_character(const int& integer, const int& number_of_characters, const char& leading_character);
std::string lead_blanks(const std::string& string, const int& number_of_characters);
std::string lead_blanks(const int& string, const int& number_of_characters);
std::string lead_zeroes(const std::string& string, const int& number_of_characters);
std::string lead_zeroes(const int& integer, const int& number_of_characters);

std::string current_timestamp();

inline int positive_modulo(int i, int n) {
	return (i % n + n) % n;
}

inline float positive_modulo(float i, float n) {
	return fmod(fmod(i, n) + n, n);
}

inline bool is_power_of_two(unsigned int number)
{
	return !(number == 0) && !(number & (number - 1));
}

unsigned int mipIDForDimensions(unsigned int dimensions);

unsigned int mipLevelToMipID(unsigned int mipLevel, unsigned int textureDimensions);

unsigned int texelMipIDToTileMipID(unsigned int texelMipID, unsigned int tileDimensionsInTexels);

unsigned int numberOfMipLevelsForDimensions(unsigned int dimensions);

#endif // HELPER_FUNCTIONS_H
