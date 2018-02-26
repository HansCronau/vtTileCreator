/*
 * Copyright 2017 Hans Cronau
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // std::reverse
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include "DevIL/devil_cpp_wrapper.h"
#include "pugixml/pugixml.hpp"
#include "RectangleBinPack/RectangleBinPack.h"

#include "config.h"
#include "helper_functions.h"

using namespace rbp;
namespace po = boost::program_options;

// Global configuration.
unsigned int vt_subtexture_border_texels_wide;
unsigned int vt_tile_texels_wide;
unsigned int vt_tile_border_texels_wide;
unsigned int vt_atlas_texels_wide;
std::string  vt_atlas_file_format;
std::string  vt_tile_file_format;
std::string  output_path;

// Global values.
ILubyte vt_atlas_bpp    = 3;                // Bytes (not bits) per pixel, number of channels.
ILenum  vt_atlas_format = IL_RGB;           // Channels are R, G, and B.
ILenum  vt_atlas_type   = IL_UNSIGNED_BYTE; // One byte per colour channel.

// TODO: Make subtexture a class with its own header file and private attributes.
struct subtexture
{
	size_t       m_index;
    std::string  m_original_file_name;
	ilImage      m_image;
	std::shared_ptr<RectangleBinPack::Node>
		         m_atlas_node;
	unsigned int m_texels_wide;
	unsigned int m_texels_high;
	unsigned int m_border_texels_wide = 0;

	unsigned int top_left_texel_within_atlas_x() { return m_atlas_node->x; }
	unsigned int top_left_texel_within_atlas_y() { return m_atlas_node->y; }
	unsigned int payload_top_left_texel_within_atlas_x() { return top_left_texel_within_atlas_x() + m_border_texels_wide; }
	unsigned int payload_top_left_texel_within_atlas_y() { return top_left_texel_within_atlas_y() + m_border_texels_wide; }
	unsigned int payload_texels_wide() { return m_texels_wide - 2 * m_border_texels_wide; }
	unsigned int payload_texels_high() { return m_texels_high - 2 * m_border_texels_wide; }

	subtexture(const size_t &index, const boost::filesystem::path file_path) :
        m_index(index),
        m_original_file_name(file_path.filename().string()),
		m_image(ilImage(file_path.string().c_str())),
		m_texels_wide(m_image.Width()),
		m_texels_high(m_image.Height())
	{}

	// Calling add_inset_border multiple times will yield unpredictable results.
	void add_inset_border(const unsigned int &border_texels_wide)
	{
		// Set border width.
		m_border_texels_wide = border_texels_wide;

		// Scale down current image (because border will be inset).
		iluImageParameter(ILU_FILTER, ILU_BILINEAR); // Sufficient quality filter for downscaling according to DevIL docs. Sidenote: Couldn't find this function in DevIL C++ Wrapper.
		m_image.Resize(m_texels_wide - 2*m_border_texels_wide, m_texels_high - 2*m_border_texels_wide, 1); // New size

		// Make temporal copy of scaled down image.
		ilState::Enable(IL_ORIGIN_SET);
		ilState::Origin(m_image.GetOrigin()); // Prevent image getting flipped vertically.
		ilImage bordered_image;
		//temp_image.Copy(m_image.GetId());
		bordered_image.TexImage(m_texels_wide, m_texels_high, 1, vt_atlas_bpp, vt_atlas_format, vt_atlas_type, NULL);
		bordered_image.Bind();
		ilClearImage(); // Just to get rid off garbage values. Nice for debugging.
		
		// Copy the scaled down original image over leaving borders uncoloured for now.
		ilOverlayImage(m_image.GetId(), m_border_texels_wide, m_border_texels_wide, 0);
		// Note: original m_image is now no longer required.

		// To add a border we will loop over all texels, 1 by 1, skipping the non-border texels, and copy byte values from the just copied image data.
		ILubyte *bordered_image_bytes = bordered_image.GetData();

		// Loop over texels left to right, top to bottom.
		for (unsigned int y = 0; y < m_texels_high; y++)
		{
			for (unsigned int x = 0; x < m_texels_wide; x++)
			{
				// Now we'll act as if the non-border texels are their own image with its own coordinate system starting at its top left texel.
				const int non_border_image_width  = m_texels_wide - 2 * m_border_texels_wide;
				const int non_border_image_height = m_texels_high - 2 * m_border_texels_wide;

				// Calculate corresponding non-border coordinates.
				int non_border_x = x - m_border_texels_wide;
				int non_border_y = y - m_border_texels_wide;

				// If non_border_x and non_border_y are within the range of the non-border image, this texel was copied already.
				if (non_border_x >= 0 && non_border_x < non_border_image_width && non_border_y >= 0 && non_border_y < non_border_image_height)
				{
					continue;
				}
				// If we've got this far, we've got a non-border coordinate outside of the non-border image, in other words: a border texel.

				// Wrap non_border_x and non_border_y and copy texel bytes and adjust back to border_image coordinates by adding border width.
				const int wrapped_x = positive_modulo(non_border_x, non_border_image_width ) + m_border_texels_wide;
				const int wrapped_y = positive_modulo(non_border_y, non_border_image_height) + m_border_texels_wide;

				// Translate the x and y coordinates into a one dimensional byte offset for bordered_image.
				int byte_offset = (y * m_texels_wide + x) * vt_atlas_bpp;

				// Now do the same for the imaginary non-border image.
				int wrapped_byte_offset = (wrapped_y * m_texels_wide + wrapped_x) * vt_atlas_bpp;
				
				// Last is to loop over all texel bytes, copying from wrapped to non-wrapped.
				for (int b = 0; b < vt_atlas_bpp; b++)
				{
					*(bordered_image_bytes + byte_offset + b) = *(bordered_image_bytes + wrapped_byte_offset + b);
				}
			}
		}

		// The bordered image is now finished and ready to become the new m_image.
		m_image = bordered_image;
	}

	// Add subtexture to atlas using RectangleBinPack to find a spot and ilImage to copy subtexture data to.
	void add_to_atlas(RectangleBinPack &atlas_bin, ilImage &atlas_image)
	{
		m_atlas_node = atlas_bin.Insert(m_texels_wide, m_texels_high);
		if (!m_atlas_node)
		{
			std::cout << "Something went terribly wrong inserting subtexture " << m_original_file_name << " into atlas." << std::endl;
			exit(EXIT_FAILURE);
		}
		else
		{
			atlas_image.Bind();
			ilOverlayImage(m_image.GetId(), top_left_texel_within_atlas_x(), top_left_texel_within_atlas_y(), 0);
		}
	}
};

std::string regex_escape(const std::string& string_to_escape) {
	static const boost::regex re_boostRegexEscape("[.^$|()\\[\\]{}*+?\\\\]");
	const std::string rep("\\\\&");
	std::string result = regex_replace(string_to_escape, re_boostRegexEscape, rep, boost::match_default | boost::format_sed);
	return result;
}

int main(int argc, char *argv[])
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Parse command line options.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// Options allowed only on command line
	po::options_description generic("Generic options");
	generic.add_options()
		("help", "produce help message")
		;

	// Options allowed both on command line and config file
	po::options_description config("Configuration");
	config.add_options()
		("subtexture-files,i", po::value< std::vector<std::string> >(), "specifies texture files to virtualise")
		("output-path,o", po::value< std::string >(&output_path)->default_value(boost::filesystem::current_path().string() + "\\" + current_timestamp()), "path to write files to")

		("wrap-border-width", po::value<unsigned int>(&vt_subtexture_border_texels_wide)->default_value(std::atoi(VT_SUBTEXTURE_BORDER_TEXELS_WIDE)), "subtexture wrapping border width in texels")
		("atlas-width", po::value<unsigned int>(&vt_atlas_texels_wide)->default_value(std::atoi(VT_ATLAS_TEXELS_WIDE)), "atlas width (and height) in texels")
		("atlas-format", po::value< std::string >(&vt_atlas_file_format)->default_value(VT_ATLAS_FORMAT), "extension to use for atlas image file")

		("tile-width", po::value<unsigned int>(&vt_tile_texels_wide)->default_value(std::atoi(VT_TILE_TEXELS_WIDE)), "tile width (and height) in texels")
		("tile-border-width", po::value<unsigned int>(&vt_tile_border_texels_wide)->default_value(std::atoi(VT_TILE_BORDER_TEXELS_WIDE)), "tile border width in texels")
		("tile-format", po::value< std::string >(&vt_tile_file_format)->default_value(VT_TILE_FORMAT), "extension to use for tile image files")
		;

	// Options allowed in both, but hidden from help.
	po::options_description hidden("Hidden options");
	hidden.add_options()
		;

	po::options_description po_commandline;
	po_commandline.add(generic).add(config).add(hidden);

	po::positional_options_description po_positionals;
	po_positionals.add("subtexture-files", -1);

	po::options_description po_config;
	po_config.add(config).add(hidden);

	po::options_description po_visible("Allowed options");
	po_visible.add(generic).add(config);

	po::variables_map po_variables;
	po::store(
		po::command_line_parser(argc, argv)
		.options(po_commandline)
		.positional(po_positionals)
		.run(),
		po_variables
	);
	// Commented lines below were intended for a config. Sadly something in Boost appears to be broken. :( Documentation is incorrect.
	//const char* filename = "config.txt";
	//po::basic_parsed_options<char> parsed = po::parse_config_file<char>(filename, po_config); // Added <char>
	//po::store(parsed, po_variables);
	po::notify(po_variables);

	if (po_variables.count("help")) {
		std::cout << "vtTileCreator creates tiles from textures for use in virtual textures.\n"
			<< po_visible << std::endl;
		return 1;
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Step 0: Prepare running the full program.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Running full program. Initial output.
	std::cout << "Running vtTileCreator. Copyright by Hans Cronau." << std::endl
		<< "This software is licenced under the MIT licence." << std::endl
		<< "Use at own risk of imploding the cosmos." << std::endl
		<< std::endl;

	// Create a vector of all/provided subtextures.
	std::cout << "Loading subtextures..." << std::endl;

	std::vector<std::string> subtexture_paths;
	std::vector<subtexture> subtextures;
	
	// Check if filenames were given.
	if (po_variables.count("subtexture-files") < 1)
	{
		std::cout << "No filenames were given to create atlas and tiles from. Exiting..." << std::endl;
		return 1;
	}
	
	// Go over command line input, adding existing paths to subtexture_paths vector.
	for (std::string subtexture_path : po_variables["subtexture-files"].as<std::vector<std::string>>())
	{
		boost::filesystem::path file_path(subtexture_path);
		if (file_path.is_relative())
		{
			file_path = boost::filesystem::current_path().string() + "\\" + file_path.string();
		}
			
		// Support for wildcards is very convenient when working with many files as input. I use regex for this.

		// Build a regex filter from the path input.
		const boost::regex dot(regex_escape("."));
		const std::string escaped_dot = regex_escape("\\.");

		const boost::regex wildcard(regex_escape("*"));
		const std::string regex_wildcard = regex_escape(".*");

		const std::string filename_escaped_dot = boost::regex_replace(file_path.filename().string(), dot,      escaped_dot);
		const std::string filename_regex       = boost::regex_replace(filename_escaped_dot,          wildcard, regex_wildcard);

		const boost::regex filter(filename_regex);

		// Go over files in path parent directory using filter.
		boost::filesystem::directory_iterator end_iterator; // Default constructor yields a past-the-end iterator.
		for (boost::filesystem::directory_iterator i(file_path.parent_path()); i != end_iterator; ++i)
		{
			// Skip if not a file.
			if (!boost::filesystem::is_regular_file(i->status()))
			{
				continue;
			}

			// Skip if no match.
			boost::smatch what;
			if (!boost::regex_match(i->path().filename().string(), what, filter))
			{
				continue;
			}

			// File matches. Add to vector.
			subtexture_paths.push_back(i->path().string());
		}
	}

	// Check if files were found.
	if (subtexture_paths.size() < 1)
	{
		std::cout << "No files matching to input were found create atlas and tiles from. Exiting..." << std::endl;
		return 1;
	}

	// For clean output, determine the length of the longest file name.
	unsigned int length_longest_filename = 0;
	for (boost::filesystem::path subtexture_path : subtexture_paths)
	{
		const unsigned int length_filename = (unsigned int)subtexture_path.filename().string().size();
		if (length_filename > length_longest_filename)
		{
			length_longest_filename = length_filename;
		}
	}

	// Go over subtexture_paths vector, creating a subtexture for each.
	for (boost::filesystem::path subtexture_path : subtexture_paths)
	{
		// Assumed here is that all arguments are correct paths to textures.
		subtexture texture = subtexture(subtextures.size(), subtexture_path);
		subtextures.push_back(texture);
		std::cout << " - Loaded subtexture " << lead_blanks(subtexture_path.filename().string(), length_longest_filename) << ", " << lead_blanks(texture.m_texels_wide, 4) << " * " << lead_blanks(texture.m_texels_high, 4) << " texels, " << (int)texture.m_image.Bpp() << " bpp, format: " << texture.m_image.Format() << ", type: " << texture.m_image.Type() << "." << std::endl;
	}
	std::cout << std::endl;

	// Prepare root directory.
	boost::filesystem::path output_dir(output_path);
	if (!boost::filesystem::exists(output_dir))
	{
		std::cout << "Creating output directory " << output_dir << "..." << std::endl;
		bool success = boost::filesystem::create_directories(output_dir);
		if (!success)
		{
			std::cout << "Couldn't create output directory. Exiting..." << std::endl;
			return 1;
		}
	}
	{
		std::cout << "Output directory " << output_dir << " exists." << std::endl;
	}
	std::cout << std::endl;


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Step 1a: Add an inset wrapping borders to all subtextures.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	boost::filesystem::path wrapping_border_folder_path( output_dir.string() + "\\1a_wrappingborders");
	boost::filesystem::create_directory(wrapping_border_folder_path);
	std::cout << "Creating subtextures with wrapping borders in " << wrapping_border_folder_path.string() << "..." << std::endl;

	for (subtexture &subtexture : subtextures)
	{
		subtexture.add_inset_border(vt_subtexture_border_texels_wide);
		std::string file_path = wrapping_border_folder_path.string() + "\\" + subtexture.m_original_file_name;
		std::cout << " - Saving subtexture " << subtexture.m_original_file_name << "." << std::endl;
		subtexture.m_image.Save(file_path.c_str());
	}
	std::cout << std::endl;


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Step 1b: Add all subtextures to a texture atlas.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	boost::filesystem::path atlas_folder_path(output_dir.string() + "\\1b_atlas");
	boost::filesystem::create_directory(atlas_folder_path);
	std::cout << "Creating atlas in " << atlas_folder_path.string() << "..." << std::endl;

	// Prepare atlas bin and image.
	RectangleBinPack atlas_rectangle_bin_pack;
	atlas_rectangle_bin_pack.Init((int)vt_atlas_texels_wide, (int)vt_atlas_texels_wide);
	ilState::Enable(IL_ORIGIN_SET);
	ilState::Origin(IL_ORIGIN_UPPER_LEFT); // Just to be sure. Just how we like it by convention.
	ilImage atlas_image;
	atlas_image.TexImage(vt_atlas_texels_wide, vt_atlas_texels_wide, 1, vt_atlas_bpp, vt_atlas_format, vt_atlas_type, NULL);
	atlas_image.Bind();
	ilClearImage(); // Just to get rid off garbage values. Nice for debugging.

	// Add each subtexture to atlas bin and image.
	const unsigned int nr_characters_texel_coordinates = (unsigned int)std::to_string(vt_atlas_texels_wide).size();
	for (subtexture &subtexture : subtextures)
	{
		subtexture.add_to_atlas(atlas_rectangle_bin_pack, atlas_image);
		std::cout
			<< " - Assigned subtexture " << lead_blanks(subtexture.m_original_file_name, length_longest_filename)
			<< " to coordinates " << lead_blanks(subtexture.top_left_texel_within_atlas_x(), nr_characters_texel_coordinates)
			<< ", " << lead_blanks(subtexture.top_left_texel_within_atlas_y(), nr_characters_texel_coordinates) << "." << std::endl;
	}

	// Save atlas image.
	const std::string file_path = atlas_folder_path.string() + "\\atlas" + vt_atlas_file_format;
	std::cout << "Saving atlas " << file_path << "." << std::endl;
	atlas_image.Save(file_path.c_str());
	std::cout << std::endl;


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Step 1c: Generate XML output about subtexture positions in atlas
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	boost::filesystem::path atlas_xml_folder_path(output_dir.string() + "\\1c_atlas_xml");
	boost::filesystem::create_directory(atlas_xml_folder_path);
	std::cout << "Creating atlas subtexture info xml in " << atlas_xml_folder_path.string() << "..." << std::endl;

	// Setup xml document. Document layout as exported from TexturePacker by CodeAndWeb GmbH.
	pugi::xml_document atlas_xml_document;
	pugi::xml_node atlas_xml_declaration = atlas_xml_document.append_child(pugi::node_declaration);
	atlas_xml_declaration.append_attribute("version") = "1.0";
	atlas_xml_declaration.append_attribute("encoding") = "UTF-8";
	pugi::xml_node atlas_xml_header_comment1 = atlas_xml_document.append_child(pugi::node_comment);
	atlas_xml_header_comment1.set_value("Created by vtTileCreator, Copyright 2017 Hans Cronau\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :\nThe above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\nTHE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.");
	pugi::xml_node atlas_xml_header_comment2 = atlas_xml_document.append_child(pugi::node_comment);
	atlas_xml_header_comment2.set_value("Format in compliance with TexturePacker by CodeAndWeb GmbH:\nn  => name of the sprite\nx  => sprite x pos in texture\ny  => sprite y pos in texture\nw  => sprite width (may be trimmed)\nh  => sprite height (may be trimmed)\npX => x pos of the pivot point (relative to sprite width)\npY => y pos of the pivot point (relative to sprite height)\noX => sprite's x-corner offset (only available if trimmed)\noY => sprite's y-corner offset (only available if trimmed)\noW => sprite's original width (only available if trimmed)\noH => sprite's original height (only available if trimmed)\nr => 'y' only set if sprite is rotated\nwith polygon mode enabled:\nvertices   => points in sprite coordinate system (x0,y0,x1,y1,x2,y2, ...)\nverticesUV => points in sheet coordinate system (x0,y0,x1,y1,x2,y2, ...)\ntriangles  => sprite triangulation, 3 vertex indices per triangle\n");

	pugi::xml_node xml_atlas = atlas_xml_document.append_child("TextureAtlas");
	xml_atlas.append_attribute("imagePath");
	xml_atlas.append_attribute("width");
	xml_atlas.append_attribute("height");
	xml_atlas.attribute("imagePath").set_value(("atlas" + vt_atlas_file_format).c_str());
	xml_atlas.attribute("width").set_value(vt_atlas_texels_wide);
	xml_atlas.attribute("height").set_value(vt_atlas_texels_wide);

	// Add each subtexture's details to xml document.
	for (subtexture &subtexture : subtextures)
	{
		pugi::xml_node xml_subtexture = xml_atlas.append_child("sprite");
		xml_subtexture.append_attribute("n");
		xml_subtexture.append_attribute("x");
		xml_subtexture.append_attribute("y");
		xml_subtexture.append_attribute("w");
		xml_subtexture.append_attribute("h");
		xml_subtexture.append_attribute("pX"); // Only because present in TexturePacker xml files.
		xml_subtexture.append_attribute("pY"); // Only because present in TexturePacker xml files.

		xml_subtexture.attribute("n").set_value(subtexture.m_original_file_name.c_str());
		xml_subtexture.attribute("x").set_value(subtexture.payload_top_left_texel_within_atlas_x());
		xml_subtexture.attribute("y").set_value(subtexture.payload_top_left_texel_within_atlas_y());
		xml_subtexture.attribute("w").set_value(subtexture.payload_texels_wide());
		xml_subtexture.attribute("h").set_value(subtexture.payload_texels_high());
		xml_subtexture.attribute("pX").set_value("0.5");
		xml_subtexture.attribute("pY").set_value("0.5");
	}

	// Save file.
	const std::string atlas_xml_file_path = atlas_xml_folder_path.string() + "\\atlas.xml";
	std::cout << "Saving xml document " << atlas_xml_file_path << "." << std::endl;
	atlas_xml_document.save_file(atlas_xml_file_path.c_str(), PUGIXML_TEXT("    "), pugi::format_default, pugi::encoding_utf8);
	std::cout << std::endl;


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Step 2: Create mipmaps of the texture atlas.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	boost::filesystem::path mipmapped_atlas_folder_path(output_dir.string() + "\\2_mipmapped_atlas");
	boost::filesystem::create_directory(mipmapped_atlas_folder_path);
	std::cout << "Creating atlas mipmaps in " << mipmapped_atlas_folder_path.string();

	// At this point I really wanna make sure that we've got power-of-two stuff going on here.
	assert(is_power_of_two(vt_atlas_texels_wide));
	assert(is_power_of_two(vt_tile_texels_wide));
	// Some explanation about dimensions: I actually don't care that much that the atlas is an exact power of two.
	// In fact the only technical requirement for software virtual textures, the type I'm doing here with the page table being a texture in itself,
	// is that the page table is a power of two. Stating that the atlas should be a power of two, AND that the tiles should be a power of two,
	// AND that there should be tile borders actually goes against the page table also being a power of two. That's why, in the next and last step,
	// you'll see me scale down the atlas, to make room for tile borders.
	// At this point however, I mostly want to ensure a minimum of bleeding between subtextures, which is assured if both
	// the atlas and subtextures are power-of-two textures, the latter of which I know, in this case, to be true.

	// What's gonna happen now.
	// This program will not create mipmaps all the way down to a 1x1 texel mipmap. Instead it will stop at the dimensions of 1 tile.
	// This will give the same number of mipmap levels as the corresponding page table mipmap level will eventually have.
	// I'll express mip levels not by starting at 0 for full res and going up each time it is scaled down (as DevIL does).
	// Instead I'll use a "mipID", a number that is unique based on mip level dimensions: 2^mipID = texels_wide
	// Since we were talking about tile pool texels there, texels_wide actually corresponds to tiles_wide at atlas level.
	// Note that the tile (pool) mip level of 1x1 tile has a tile mipID 0 and that every next power of two has a 1 higher mipID.
	// Ready?

	// Prepare image vector for mipmap levels.
	std::vector<ilImage*> atlas_mipmaps;
    ILuint current_mipmap_texels_wide = atlas_image.Width();
    iluImageParameter(ILU_FILTER, ILU_BILINEAR);

	// Generate mipmap levels down to tile size and add to vector.
    while (current_mipmap_texels_wide >= vt_tile_texels_wide)
	{
		// Give some output.
		std::cout << ".";

        // Downscale mipmap slightly more to accomodate for tile borders while retaining power-of-two page table.
        const unsigned int current_mipmap_tiles_wide = current_mipmap_texels_wide / vt_tile_texels_wide;
        const unsigned int current_mipmap_texels_wide_scaled = current_mipmap_texels_wide - current_mipmap_tiles_wide * 2 * vt_tile_border_texels_wide;

        // Create new mipmap
        ilImage* current_mipmap_level = new ilImage();
        current_mipmap_level->Copy(atlas_image.GetId());
        current_mipmap_level->Resize(current_mipmap_texels_wide_scaled, current_mipmap_texels_wide_scaled, 1);

        // Add to vector.
        atlas_mipmaps.push_back(current_mipmap_level);

        // Calculate what would be the size of next (smaller) mipmap level.
        current_mipmap_texels_wide /= 2;
	}
    std::cout << std::endl;
	
	// Reverse mipmap levels so that index corresponds to tile mipID.
	std::reverse(atlas_mipmaps.begin(), atlas_mipmaps.end());

	// Save all mipmap levels to file.
	for (size_t atlas_tile_mipID = 0; atlas_tile_mipID < atlas_mipmaps.size(); atlas_tile_mipID++)
	{
		const std::string mipmap_level_file_path = mipmapped_atlas_folder_path.string() + "\\atlas_" + std::to_string(atlas_tile_mipID) + vt_atlas_file_format;
		std::cout << " - Saving atlas tile mipID " << atlas_tile_mipID << " to atlas_" + std::to_string(atlas_tile_mipID) + vt_atlas_file_format + "." << std::endl;
		atlas_mipmaps[atlas_tile_mipID]->Save(mipmap_level_file_path.c_str());
	}
	std::cout << std::endl;


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Step 3a: Cut all atlas mipmaps into bordered tiles
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	boost::filesystem::path tiles_folder_path(output_dir.string() + "\\3a_tiles");
	boost::filesystem::create_directory(tiles_folder_path);
	std::cout << "Creating tiles in " << tiles_folder_path.string() << "." << std::endl;

	// Values used down the line.
	const unsigned int payload_texels_wide = vt_tile_texels_wide - 2 * vt_tile_border_texels_wide;

	// Values used down the line for saving tiles to file.
	const unsigned int atlas_tiles_wide        = vt_atlas_texels_wide / vt_tile_texels_wide;
	const unsigned int nr_characters_for_coord = (unsigned int)std::to_string(atlas_tiles_wide - 1).size();
	const unsigned int nr_characters_for_mipID = (unsigned int)std::to_string(atlas_mipmaps.size() - 1).size();

	// Downscale all mipmap levels to make room for tile borders and cut up in bordered tiles.
	for (size_t atlas_tile_mipID = 0; atlas_tile_mipID < atlas_mipmaps.size(); atlas_tile_mipID++)
	{
		// Give some output.
		std::cout << " - Processing mipmap level with tile mipID " << atlas_tile_mipID;

        // Calculate mipmap dimension in tiles.
        const unsigned int mipmap_level_tiles_wide = 1 << atlas_tile_mipID;

		// Loop over all tiles in mipmap level and create and save.
		for (unsigned int tile_y = 0; tile_y < mipmap_level_tiles_wide; ++tile_y)
		{
			for (unsigned int tile_x = 0; tile_x < mipmap_level_tiles_wide; ++tile_x)
			{
				// Give some output.
				std::cout << ".";

				// Coordinates in atlas mipmap. (Minus border width is to give tiles a border with data from neighbouring tiles.)
				const int tile_top_left_atlas_texel_x = tile_x                                 * payload_texels_wide - vt_tile_border_texels_wide;
				const int tile_top_left_atlas_texel_y = (mipmap_level_tiles_wide - 1 - tile_y) * payload_texels_wide - vt_tile_border_texels_wide;
				// Note: Tile-coordinates, like UV-coordinates, use a lower left origin (in my implementation at least).
				//       At texel level this program uses an upper left origin for image manipulation. Unlike the x-axis, the tile-y-axis is therefore flipped above.

				// Used for check whether sampling inside atlas mipmap. (See below.)
				const int tile_lower_right_atlas_texel_x = tile_top_left_atlas_texel_x + vt_tile_texels_wide;
				const int tile_lower_right_atlas_texel_y = tile_top_left_atlas_texel_y + vt_tile_texels_wide;
				
				// We need to take care of some "edge" conditions, specifically for borders sampling outside of the texture atlas.
				const unsigned int texels_past_left_border  = tile_top_left_atlas_texel_x     <  0                                             ? (unsigned int)( -tile_top_left_atlas_texel_x                                               ) : 0;
				const unsigned int texels_past_upper_border = tile_top_left_atlas_texel_y     <  0                                             ? (unsigned int)( -tile_top_left_atlas_texel_y                                               ) : 0;
				const unsigned int texels_past_right_border = tile_lower_right_atlas_texel_x >= (int)atlas_mipmaps[atlas_tile_mipID]->Width()  ? (unsigned int)( tile_lower_right_atlas_texel_x - atlas_mipmaps[atlas_tile_mipID]->Width()  ) : 0;
				const unsigned int texels_past_lower_border = tile_lower_right_atlas_texel_y >= (int)atlas_mipmaps[atlas_tile_mipID]->Height() ? (unsigned int)( tile_lower_right_atlas_texel_y - atlas_mipmaps[atlas_tile_mipID]->Height() ) : 0;
				
				// Create tile image.
				ilImage tile_image;
				tile_image.TexImage(vt_tile_texels_wide, vt_tile_texels_wide, 1, vt_atlas_bpp, vt_atlas_format, vt_atlas_type, NULL);
				tile_image.Bind();
				ilClearImage(); // Just to get rid off garbage values. Nice for debugging.

				// Copy corresponding texels from atlas mipmap level.
				ilBlit(
					atlas_mipmaps[atlas_tile_mipID]->GetId(),
					0 + texels_past_left_border,                                               // destination/tile top left x
					0 + texels_past_upper_border,                                              // destination/tile top left y
					0,                                                                         // destination/tile top left z
					tile_top_left_atlas_texel_x + texels_past_left_border,                     // source/atlas top left x
					tile_top_left_atlas_texel_y + texels_past_upper_border,                    // source/atlas top left y
					0,                                                                         // source/atlas top left z
					vt_tile_texels_wide - texels_past_left_border  - texels_past_right_border, // nr texels to copy x
					vt_tile_texels_wide - texels_past_upper_border - texels_past_lower_border, // nr texels to copy y
					1                                                                          // nr texels to copy z
				);

				// Save tile to file.
				const std::string tile_file_path = tiles_folder_path.string() + "\\tile_mipid_" + std::to_string(atlas_tile_mipID) + "_x_" + std::to_string(tile_x) + "_y_" + std::to_string(tile_y) + vt_atlas_file_format;
				tile_image.Save(tile_file_path.c_str());
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Step 3b: Generate XML with tile info
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	boost::filesystem::path tile_xml_folder_path(output_dir.string() + "\\3b_tiles_xml");
	boost::filesystem::create_directory(tile_xml_folder_path);
	std::cout << "Creating xml with tile info in " << tile_xml_folder_path.string() << "..." << std::endl;

	// Setup xml document.
	pugi::xml_document tile_xml_document;
	pugi::xml_node tile_xml_declaration = tile_xml_document.append_child(pugi::node_declaration);
	tile_xml_declaration.append_attribute("version") = "1.0";
	tile_xml_declaration.append_attribute("encoding") = "UTF-8";
	pugi::xml_node tile_xml_header_comment = tile_xml_document.append_child(pugi::node_comment);
	tile_xml_header_comment.set_value("Created by vtTileCreator, Copyright 2017 Hans Cronau\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :\nThe above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\nTHE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.");
	
	pugi::xml_node xml_tile_info = tile_xml_document.append_child("tile_info");
	xml_tile_info.append_attribute("dimensions_in_texels");
	xml_tile_info.append_attribute("border_in_texels");
	xml_tile_info.append_attribute("file_extension");
	xml_tile_info.attribute("dimensions_in_texels").set_value(vt_tile_texels_wide);
	xml_tile_info.attribute("border_in_texels").set_value(vt_tile_border_texels_wide);
	xml_tile_info.attribute("file_extension").set_value(vt_tile_file_format.c_str());

	// Save file.
	const std::string tile_xml_file_path = tile_xml_folder_path.string() + "\\tile_info.xml";
	std::cout << "Saving xml document " << tile_xml_file_path << "." << std::endl;
	tile_xml_document.save_file(tile_xml_file_path.c_str(), PUGIXML_TEXT("    "), pugi::format_default, pugi::encoding_utf8);
    std::cout << std::endl;


    /////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Closing
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	for (ilImage* mipmap_level : atlas_mipmaps)
	{
		delete mipmap_level;
	}
	std::cout << "Done!\nBye bye." << std::endl;

	return 0;
}
