/*
 * FileTag.cpp
 *
 *  Created on: Dec 18, 2012
 *      Author: akavo
 */

#include "FileTagger.h"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <cstring>

tstring FieldTypeToString(FieldType type)
{
	tstring str_type;
	switch(type)
	{
		case Title:
			str_type = _T("Title";)
			break;
		case Artist:
			str_type = _T("Artist");
			break;
		case Album:
			str_type = _T("Album");
			break;
		case Genre:
			str_type = _T("Genre");
			break;
		case Comment:
			str_type = _T("Comment");
			break;
		case TrackNo:
			str_type = _T("Track#");
			break;
		case Year:
			str_type = _T("Year");
			break;
		case Delimiter:
			str_type = _T("Delimiter");
			break;
		case Ignore:
			str_type = _T("Ignore");
			break;
		default:
			str_type = _T("_Unknown");
			break;
	}
	return str_type;
}

//////////////////////////////////////////////////////////////////////////////////
std::string Field::content_narrow;

Field::Field(tstring content, FieldType type)
: _content(content)
, _type(type)
{
}

Field::~Field()
{
}

const char* Field::ToCharArr(std::string &src)
{
	return src.c_str();
}

const char* Field::ToCharArr(std::wstring &src)
{
	content_narrow.reserve(src.size());
	// wide to UTF-8
	content_narrow.assign(src.begin(), src.end());
	return content_narrow.c_str();
}

const char* Field::ToCharArr()
{
	return ToCharArr(_content);
}
//////////////////////////////////////////////////////////////////////////////////

Pattern::Pattern(tstring format, bool trim)
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
//separated by any delimiter string

bool Pattern::parse()
{
	_nNamedFields = 0;
	_nDelFields = 0;
	_nPathSeparators = 0;
	_structure.clear();
	_delimiters.clear();

	//Find all occurrences of the allowed fields
	Field title(_T("<Title>"), Title);
	_nNamedFields += find_in_pattern(title);

	Field artist(_T("<Artist>"), Artist);
	_nNamedFields += find_in_pattern(artist);

	Field album(_T("<Album>"), Album);
	_nNamedFields += find_in_pattern(album);

	Field genre(_T("<Genre>"), Genre);
	_nNamedFields += find_in_pattern(genre);

	Field comment(_T("<Comment>"), Comment);
	_nNamedFields += find_in_pattern(comment);

	Field trackno(_T("<Track#>"), TrackNo);
	_nNamedFields += find_in_pattern(trackno);

	Field year(_T("<Year>"), Year);
	_nNamedFields += find_in_pattern(year);

	Field ignore(_T("<Ignore>"), Ignore);
	_nNamedFields += find_in_pattern(ignore);

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

int Pattern::find_insert_field(Field needle, tstring &haystack, position_map &out)
{
	int count = 0;

	size_t pos = haystack.find(needle._content, 0);
	while(pos != tstring::npos)
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
	tstring delimiter = _pattern.substr(del_start, del_end-del_start);
	if(_trim) {
		boost::algorithm::trim_if(delimiter, boost::is_any_of(_trim_chars));
		if(delimiter.size() < 1)
			return false;
	}
	Field del(delimiter, Delimiter);
	_delimiters.insert(std::pair<size_t,Field>(del_start, del));
	++_nDelFields;
	if(delimiter==_T("/") || delimiter==_T("\\"))
		++_nPathSeparators;
	return true;
}

bool Pattern::match(tstring &file_str, position_map &out)
{
	typedef std::vector<std::pair<size_t,size_t> > intervals;
	intervals field_intervals;
	position_map delimiters;
	size_t pos = 0;
	//Ensure sequence of delimiters exists
	for(position_map::iterator it = _delimiters.begin(); it != _delimiters.end(); ++it)
	{
		Field &field = it->second;
		pos = file_str.find(field._content, pos);
		if(pos == tstring::npos) {
			tcout << "Rejected: Delimiter `" << field._content << "` not found\n\n";
			return false;
		}
		delimiters.insert(std::pair<size_t,Field>(pos, field));	//insert delimiter
		++pos;
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
		}
		idx_start = idx_end+field.size();
	}
	//check tail
	if(idx_start != file_str.size())
		field_intervals.push_back(std::pair<size_t,size_t>(idx_start, file_str.size()));

	if(_nNamedFields != field_intervals.size()) {
		tcout << "Rejected: Field count mismatch\n\n";
		return false;
	}
	//Extract fields
	int n = 0;
	for(position_map::iterator it = _structure.begin(); it != _structure.end(); ++it)
	{
		Field field = it->second;
		if(field._type == Delimiter)
			continue;
		std::pair<size_t,size_t> interval = field_intervals[n]; ++n;
		field._content = file_str.substr(interval.first, interval.second-interval.first);
		if(_trim)
			boost::algorithm::trim_if(field._content, boost::is_any_of(_trim_chars));

		out.insert(std::pair<size_t,Field>(interval.first, field));
	}
	return true;
}

void Pattern::print()
{
	if(!_valid) {
		tcout << "Pattern::print(): Invalid pattern" << std::endl;
		return;
	}
	tcout << "Trim=" << _trim << std::endl;
	for(position_map::iterator it = _structure.begin(); it != _structure.end(); ++it)
	{
			size_t pos = it->first;
			Field &field = it->second;
			tcout << "Type " << FieldTypeToString(field._type);
			if(field._type == Delimiter)
				tcout << "`" << field._content << "`";
			tcout << " at pos= " << pos << std::endl;
	}
	tcout << std::endl;
}

bool Pattern::begins_with_separator()
{
	if(!_structure.empty() &&
			_structure.begin()->second._type == Delimiter &&
			(_structure.begin()->second._content == _T("/") ||
					_structure.begin()->second._content == _T("\\")))
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////

FileTagger::FileTagger(Pattern &p, bool replace_non_empty)
: _pattern(p)
, _safe(false)
, _replace(replace_non_empty)
{

}

FileTagger::~FileTagger()
{

}

void FileTagger::SetEmptyFieldConstraint(std::vector<tstring> &empty_fields)
{
	_empty_fields = empty_fields;
}

void FileTagger::SetSafeMode(bool safe_mode)
{
	_safe = safe_mode;
}
//TODO: DUPLICATE FIELDS
void FileTagger::Tag(tstring path, bool recursive)
{
	fs::path path_to_dir_or_file = fs::path(path);
	try
	{
		if (!fs::exists(path_to_dir_or_file)) {
			tcout << _T("Invalid path: ") << path_to_dir_or_file.string<tstring>() << std::endl;
			return;
		}

		if(fs::is_directory(path_to_dir_or_file))
		{
			TagDirectory(path, recursive);
		}
		else if (fs::is_regular_file(path_to_dir_or_file))
		{
			TagFile(path_to_dir_or_file);
		}
		else
			tcout << _T("Invalid path: ") << path_to_dir_or_file.string<tstring>() << std::endl;

	} catch (const fs::filesystem_error& ex) {
		tcout << ex.what() << std::endl;
	}
}

void FileTagger::TagDirectory(tstring dir, bool recursive)
{
	fs::path files_dir = fs::absolute(fs::path(dir));
	if(recursive)
		TagDirectoryRecursive(files_dir);
	else
		TagDirectory(files_dir);
}

void FileTagger::TagDirectory(fs::path files_dir)
{

	try {
		if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) {
			tcout << _T("Invalid directory: ") << files_dir.string<tstring>() << std::endl;
			return;
		}

		for (fs::directory_iterator end, dir(files_dir); dir != end;
				++dir) {
			fs::path p = dir->path();

			if (fs::is_regular_file(p)) {
				tstring extension = p.extension().string<tstring>();
				boost::algorithm::to_lower(extension);
				if (extension != _T(".mp3")) {
					tcout << _T("Skipping ") <<  p.string<tstring>() << std::endl;
					continue;
				}
				TagFile(p);

			} else if (fs::is_directory(p)) {
				tcout << _T("Directory: ") << p.string<tstring>() << std::endl;

			} else {
				tcout << _T("Error reading: ") << p.string<tstring>() << std::endl;
			}
		}
	} catch (const fs::filesystem_error& ex) {
		tcout << ex.what() << std::endl;
	}
}

void FileTagger::TagDirectoryRecursive(fs::path files_dir)
{

	try {
		if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) {
			tcout << _T("Invalid directory: ") << files_dir.string<tstring>() << std::endl;
			return;
		}

		for (fs::recursive_directory_iterator end, dir(files_dir); dir != end;
				++dir) {
			fs::path p = dir->path();

			if (fs::is_regular_file(p)) {

				if (p.extension().string<tstring>() != _T(".mp3"))
					continue;

				TagFile(p);

			} else if (fs::is_directory(p)) {
				tcout << _T("Directory: ") << p.string<tstring>() << std::endl;

			} else {
				tcout << _T("Error reading: ") << p.string<tstring>() << std::endl;
			}
		}
	} catch (const fs::filesystem_error& ex) {
		tcout << ex.what() << std::endl;
	}
}

void FileTagger::TagFile(tstring file)
{
	fs::path filepath(file);
	TagFile(filepath);
}


void FileTagger::TagFile(fs::path file)
{
	fs::path filec = fs::canonical(file).make_preferred().native();

	tcout << "Canonical Path: " << filec << std::endl;

	//TagLib::FileRef f(filec.string().c_str());
	TagLib::FileRef f(filec.string<tstring>().c_str());

	if(!CheckEmptyFields(f)) {
		tcout << "Rejected: Non-Empty field(s)\n\n";
		return;
	}
	Pattern::position_map fields;

	tstring file_name;
	if(!ExtractRelevantFileName(filec, file_name)) {
		tcout << "Rejected: Filename path separator mismatch\n\n";
		return;
	}
	tcout << _T("RelevantFileName: ") << file_name << std::endl;

	if(_pattern.match(file_name, fields))
	{
		UpdateTags(f, fields);
		tcout << "Done\n\n";
		return;
	}

}

void FileTagger::UpdateTags(TagLib::FileRef &file, Pattern::position_map &fieldmap)
{
	for (Pattern::position_map::iterator it = fieldmap.begin();	it!=fieldmap.end(); ++it) {
		Field &field = it->second;

		switch(field._type)
		{
		case Artist:
			if(!_safe) file.tag()->setArtist(field._content);
			tcout << _T("Artist<-`") << field._content << _T("` | ");
			break;
		case Title:
			if(!_safe) file.tag()->setTitle(field._content);
			tcout << _T("Title<-`") << field._content << _T("` | ");
			break;
		case Album:
			if(!_safe) file.tag()->setAlbum(field._content);
			tcout << _T("Album<-`") << field._content << _T("` | ");
			break;
		case Genre:
			if(!_safe) file.tag()->setGenre(field._content);
			tcout << _T("Genre<-`") << field._content << _T("` | ");
			break;
		case Comment:
			if(!_safe) file.tag()->setComment(field._content);
			tcout << _T("Comment<-`") << field._content << _T("` | ");
			break;
		case TrackNo:
			if(!_safe) file.tag()->setTrack(atoi(field.ToCharArr()));
			tcout << _T("Track#<-`") << field._content << _T("` | ");
			break;
		case Year:
			if(!_safe) file.tag()->setYear(atoi(field.ToCharArr()));
			tcout << _T("Year<-`") << field._content << _T("` | ");
			break;

		default:
			break;
		}
	}
	if(!_safe) file.save();
}

bool FileTagger::CheckEmptyFields(TagLib::FileRef &file)
{
	for (std::vector<tstring>::iterator it = _empty_fields.begin();
			it!=_empty_fields.end(); ++it) {
		tstring &field = *it;
		if(		(field == _T("<Artist>") && !file.tag()->artist().isEmpty())
			|| 	(field == _T("<Title>") && !file.tag()->title().isEmpty())
			|| 	(field == _T("<Album>") && !file.tag()->album().isEmpty())
			|| 	(field == _T("<Genre>") && !file.tag()->genre().isEmpty())
			|| 	(field == _T("<Comment>") && !file.tag()->comment().isEmpty())
			|| 	(field == _T("<Year>") && !file.tag()->year())
			|| 	(field == _T("<Track#>") && !file.tag()->track()) )
			return false;
	}
	return true;
}


bool FileTagger::ExtractRelevantFileName(fs::path file_path, tstring &out)
{
	out = _T("");
	size_t nSeparators = _pattern.get_separator_count();
	if(nSeparators)
	{
		size_t nTokens = _pattern.begins_with_separator() ? nSeparators-1 : nSeparators;
		//Explode path
		tstring parentpath = file_path.parent_path().string<tstring>();
		std::vector<tstring> tokens;
		boost::split(tokens, parentpath, boost::is_any_of(_T("/\\")));
		if(nTokens > tokens.size())
			return false;
		boost::filesystem::path slash("/");
		fs::path::string_type platform_slash = slash.make_preferred().native();

		if(_pattern.begins_with_separator())
			out.append(platform_slash);
		while(!tokens.empty() && nTokens)
		{
			--nTokens;
			out += tokens.back();
			out += platform_slash;
			tokens.pop_back();
		}

	}
	 out += file_path.stem().string<tstring>();
	 return true;
}
