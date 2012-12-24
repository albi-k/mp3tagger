/*
 * FileTag.cpp
 *
 *  Created on: Dec 18, 2012
 *      Author: akavo
 */

#include "FileTagger.h"
#include <boost/algorithm/string.hpp>
#include <iostream>

#include "common.h"

void list_files(std::string dir) {
	fs::path files_dir(dir);

	try {
		if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) {
			std::cout << "Invalid directory: " << files_dir.string()
					<< std::endl;
			return;
		}

		for (fs::recursive_directory_iterator end, dir(files_dir); dir != end;
				++dir) {
			fs::path p = dir->path();

			if (fs::is_regular_file(p)) {

				/*if (p.extension().string() != ".jpg"
						&& p.extension().string() != ".tif"
						&& p.extension().string() != ".png")
					continue;*/

				std::string file = p.string();
				std::cout << "File: " << p.filename().string() << "\n";

			} else if (fs::is_directory(p)) {
				std::cout << "Directory: " << p.string() << std::endl;

			} else {
				std::cout << "Error reading: " << p.string() << std::endl;
			}
		}
	} catch (const fs::filesystem_error& ex) {
		std::cout << ex.what() << std::endl;
	}

}

void test() {
	TagLib::FileRef f("Fracx - For The World (D&B Edit).mp3");
	TagLib::String artist = f.tag()->artist();
	std::cout << artist << std::endl;
	//f.tag()->setArtist("Fracx");
	//f.save();

}

std::string FieldTypeToString(FieldType type)
{
	std::string str_type;
	switch(type)
	{
		case Title:
			str_type = "Title";
			break;
		case Artist:
			str_type = "Artist";
			break;
		case Album:
			str_type = "Album";
			break;
		case Delimiter:
			str_type = "Delimiter";
			break;
		default:
			str_type = "Unknown";
			break;
	}
	return str_type;
}

//////////////////////////////////////////////////////////////////////////////////

Field::Field(std::string content, FieldType type)
: _content(content)
, _type(type)
{
}

Field::~Field()
{
}

//////////////////////////////////////////////////////////////////////////////////

Pattern::Pattern(std::string format, bool trim)
: _pattern(format)
, _trim(trim)
{
	_valid = parse();
}

Pattern::~Pattern()
{
}

//A valid pattern is:
//A sequence of one or more known fields (<Artist> ...)
//separated by any non-space delimiter string

bool Pattern::parse()
{
	_nNamedFields = 0;
	_nDelFields = 0;
	_structure.clear();
	_delimiters.clear();

	//Find all occurrences of the allowed fields
	Field title("<Title>", Title);
	_nNamedFields += find_in_pattern(title);

	Field artist("<Artist>", Artist);
	_nNamedFields += find_in_pattern(artist);

	Field album("<Album>", Album);
	_nNamedFields += find_in_pattern(album);

	//Everything else must be delimiters
	size_t prev_pos = 0;
	size_t prev_size= 0;
	size_t pos = 0;
	size_t size = 0;
	position_map::iterator it = _structure.begin();
	for(; it != _structure.end(); ++it)
	{
		pos = it->first;
		size = it->second.size();

		if(!parse_helper(pos, size, prev_pos, prev_size)) {
			throw Exc("Invalid pattern: Adjacent fields without a delimiter.");
			return false;
		}

		//Update previous
		prev_pos = pos;
		prev_size = size;
	}

	bool tail = _pattern.size() > (pos + size);
	if(tail) parse_helper(_pattern.size(), 0, pos, size);


	_structure.insert(_delimiters.begin(), _delimiters.end());
	return true;
}

int Pattern::find_in_pattern(Field needle)
{
	return find_insert_field(needle, _pattern, _structure);
}

int Pattern::find_insert_field(Field needle, std::string &haystack, position_map &out)
{
	int count = 0;

	size_t pos = haystack.find(needle._content, 0);
	while(pos != std::string::npos)
	{
		++count;
		out.insert(std::pair<size_t,Field>(pos, needle));
	    pos = haystack.find(needle._content,pos+1);
	}
	return count;
}

bool Pattern::parse_helper(size_t pos, size_t size, size_t prev_pos, size_t prev_size)
{
	size_t del_start = prev_pos + prev_size;
	size_t del_end = pos;
	if(del_end <= del_start) {
		if(del_start ==0) {
			//Update previous
			prev_pos = pos;
			prev_size = size;
			return true;
		}
		return false;
	}
	std::string delimiter = _pattern.substr(del_start, del_end-del_start);
	if(_trim) {
		boost::algorithm::trim(delimiter);
		if(delimiter.size() < 1)
			return false;
	}
	Field del(delimiter, Delimiter);
	_delimiters.insert(std::pair<size_t,Field>(del_start, del));
	++_nDelFields;
	return true;
}

bool Pattern::match_string(std::string &file_stem, position_map &out)
{
	typedef std::vector<std::pair<size_t,size_t> > intervals;
	intervals field_intervals;
	position_map delimiters;
	long int pos = -1;
	//Ensure sequence of delimiters exists
	for(position_map::iterator it = _delimiters.begin(); it != _delimiters.end(); ++it)
	{
		Field &field = it->second;
		pos = file_stem.find(field._content, pos+1);
		if(pos == std::string::npos) {
			std::cout << "Rejected: Delimiter `" << field._content << "` not found(s)";
			return false;
		}
		delimiters.insert(std::pair<size_t,Field>(pos, field));	//insert delimiter
	}
	//TODO:check for existence of additional delimiters

	//Extract non-delimiter field ranges
	size_t idx_start = 0;
	size_t idx_end = 0;
	for(position_map::iterator it = delimiters.begin(); it != delimiters.end(); ++it)
	{
		idx_end = it->first;
		Field &field = it->second;
		if(idx_end-idx_start > 0) {
			field_intervals.push_back(std::pair<size_t,size_t>(idx_start, idx_end));
			idx_start = idx_end+field.size();
		}
	}
	//check tail
	if(idx_start != file_stem.size())
		field_intervals.push_back(std::pair<size_t,size_t>(idx_start, file_stem.size()));

	if(_nNamedFields != field_intervals.size())
		std::cout << "Rejected: Field count mismatch";

	//Extract fields
	int n = 0;
	for(position_map::iterator it = _structure.begin(); it != _structure.end(); ++it)
	{
		Field field = it->second;
		if(field._type == Delimiter)
			continue;
		std::pair<size_t,size_t> interval = field_intervals[n]; ++n;
		field._content = file_stem.substr(interval.first, interval.second-interval.first);
		if(_trim)
			boost::algorithm::trim(field._content);

		out.insert(std::pair<size_t,Field>(interval.first, field));
	}
	return true;
}

void Pattern::print()
{
	if(!_valid) {
		std::cout << "Pattern::print(): Invalid pattern" << std::endl;
		return;
	}
	std::cout << "Trim=" << _trim << std::endl;
	for(position_map::iterator it = _structure.begin(); it != _structure.end(); ++it)
	{
			size_t pos = it->first;
			Field &field = it->second;
			std::cout << "Type " << FieldTypeToString(field._type);
			if(field._type == Delimiter)
				std::cout << "`" << field._content << "`";
			std::cout << " at pos= " << pos << std::endl;
	}
	std::cout << std::endl;
}

//////////////////////////////////////////////////////////////////////////////////

FileTagger::FileTagger(Pattern &p)
: _pattern(p)
{

}

FileTagger::~FileTagger()
{

}

void FileTagger::SetEmptyFieldConstraint(std::vector<std::string> empty_fields)
{
	_empty_fields = empty_fields;
}

void FileTagger::TagDirectory(std::string dir)
{
	fs::path files_dir(dir);

	try {
		if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) {
			std::cout << "Invalid directory: " << files_dir.string() << std::endl;
			return;
		}

		for (fs::recursive_directory_iterator end, dir(files_dir); dir != end;
				++dir) {
			fs::path p = dir->path();

			if (fs::is_regular_file(p)) {

				if (p.extension().string() != ".mp3")
					continue;

				TagFile(p);

			} else if (fs::is_directory(p)) {
				std::cout << "Directory: " << p.string() << std::endl;

			} else {
				std::cout << "Error reading: " << p.string() << std::endl;
			}
		}
	} catch (const fs::filesystem_error& ex) {
		std::cout << ex.what() << std::endl;
	}
}

void FileTagger::TagFile(std::string file)
{
	fs::path filepath(file);
	TagFile(filepath);
}

void FileTagger::TagFile(fs::path file)
{
	std::cout << "File: " << file.filename().string() << " | ";

	TagLib::FileRef f(file.string().c_str());
	if(!CheckEmptyFields(f)) {
		std::cout << "Rejected: Non-Empty field(s)\n";
		return;
	}
	Pattern::position_map fields;
	std::string stem = file.stem().string();
	if(_pattern.match_string(stem, fields))
	{
		UpdateTags(f, fields);
		std::cout << "Done\n";
		return;
	}

}

void FileTagger::UpdateTags(TagLib::FileRef &file, Pattern::position_map &fieldmap)
{
	for (Pattern::position_map::iterator it = fieldmap.begin();	it!=fieldmap.end(); ++it) {
		Field &field = it->second;
		if(field._type == Artist) {
			file.tag()->setArtist(field._content);
			std::cout << "Artist<-`" << field._content << "` | ";
		}else if(field._type == Title) {
			file.tag()->setTitle(field._content);
			std::cout << "Title<-`" << field._content << "` | ";
		}else if(field._type == Album) {
			file.tag()->setAlbum(field._content);
			std::cout << "Album<-`" << field._content << "` | ";
		}
	}
	file.save();
}

bool FileTagger::CheckEmptyFields(TagLib::FileRef &file)
{
	for (std::vector<std::string>::iterator it = _empty_fields.begin();
			it!=_empty_fields.end(); ++it) {
		std::string &field = *it;
		if(field == "<Artist>" && !file.tag()->artist().isEmpty())
			return false;
		if(field == "<Title>" && !file.tag()->title().isEmpty())
			return false;
		if(field == "<Album>" && !file.tag()->album().isEmpty())
			return false;
	}
	return true;
}
