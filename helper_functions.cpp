#include "helper_functions.h"
//#include <ctime>   // Timestamp
#include <sstream> // Leading zeroes
#include <string>
#include <iomanip> // Leading zeroes & Timestamp

std::string lead_character(const std::string& string, const int& number_of_characters, const char& leading_character)
{
	std::stringstream stream;
	stream << std::setw(number_of_characters) << std::setfill(leading_character) << string;
	return stream.str();
}

std::string lead_character(const int& integer, const int& number_of_characters, const char& leading_character)
{
	return lead_character(std::to_string(integer), number_of_characters, leading_character);
}

std::string lead_blanks(const std::string& string, const int& number_of_characters)
{
	return lead_character(string, number_of_characters, ' ');
}

std::string lead_blanks(const int& integer, const int& number_of_characters)
{
	return lead_character(integer, number_of_characters, ' ');
}

std::string lead_zeroes(const std::string& string, const int& number_of_characters)
{
	return lead_character(string, number_of_characters, '0');
}

std::string lead_zeroes(const int& integer, const int& number_of_characters)
{
	return lead_character(integer, number_of_characters, '0');
}

std::string current_timestamp()
{
	time_t t = time(0); // Gets current time.
	tm now;
	localtime_s(&now, &t); // Converts to local time incl. attributes like year, month, etc.
	std::string stamp =
		lead_zeroes(now.tm_year + 1900, 4)
		+ '-' + lead_zeroes(now.tm_mon + 1, 2)
		+ '-' + lead_zeroes(now.tm_mday + 1, 2)
		+ '-' + lead_zeroes(now.tm_hour + 1, 2)
		+ '-' + lead_zeroes(now.tm_min + 1, 2)
		+ '-' + lead_zeroes(now.tm_sec + 1, 2);
	return stamp;
}

unsigned int mipIDForDimensions(unsigned int dimensions)
{
	unsigned int mipID = 0;
	while (dimensions > 1)
	{
		dimensions = dimensions >> 1;
		mipID++;
	}
	return mipID;
}

unsigned int mipLevelToMipID(unsigned int mipLevel, unsigned int textureDimensions) // Works for both mip levels and IDs expressed in tiles and in texels.
{
	unsigned int maxMipID = mipIDForDimensions(textureDimensions);
	return maxMipID - mipLevel;
}

unsigned int texelMipIDToTileMipID(unsigned int texelMipID, unsigned int tileDimensionsInTexels)
{
	unsigned int texelMipIDForTileMipIDZero = mipIDForDimensions(tileDimensionsInTexels);
	return texelMipID - texelMipIDForTileMipIDZero;
}

unsigned int numberOfMipLevelsForDimensions(unsigned int dimensions)
{
	return mipIDForDimensions(dimensions) + 1;
}
