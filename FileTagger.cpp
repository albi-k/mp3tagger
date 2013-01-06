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
	_delimiters_generic.clear();

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


	_structure.insert(_delimiters_generic.begin(), _delimiters_generic.end());
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
	_delimiters_generic.insert(std::pair<size_t,Field>(del_start, del));
	++_nDelFields;
	if(delimiter==_T("/") || delimiter==_T("\\"))
		++_nPathSeparators;
	return true;
}

bool Pattern::match(tstring &file_str, position_map &out) const
{
	typedef std::vector<std::pair<size_t,size_t> > intervals;
	intervals field_intervals;
	position_map delimiters;
	size_t pos = 0;
	//Ensure sequence of delimiters exists
	for(position_map::const_iterator it = _delimiters_generic.cbegin(); it != _delimiters_generic.cend(); ++it)
	{
		const Field &field = it->second;
		pos = file_str.find(field._content, pos);
		if(pos == tstring::npos) {
			log << _T("Rejected: Delimiter `") << field._content << _T("` not found\n\n");
			return false;
		}
		delimiters.insert(std::pair<size_t,Field>(pos, field));	//insert delimiter
		++pos;
	}

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
		log << _T("Rejected: Field count mismatch\n\n");
		return false;
	}
	//Extract fields
	int n = 0;
	for(position_map::const_iterator it = _structure.cbegin(); it != _structure.cend(); ++it)
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

void Pattern::print() const
{
	if(!_valid) {
		log << _T("Pattern::print(): Invalid pattern") << std::endl;
		return;
	}
	log << _T("Trim=") << _trim << std::endl;
	
	for(position_map::const_iterator it = _structure.cbegin(); it != _structure.cend(); ++it)
	{
		log_type mylog = log;
		size_t pos = it->first;
		const Field &field = it->second;
		mylog << _T("Type ") << FieldTypeToString(field._type);
		if(field._type == Delimiter)
			mylog << _T("`") << field._content << _T("`");
		mylog << _T(" at pos= ") << pos << std::endl;
	}

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
, _threads_max(1)
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


void FileTagger::Tag(tstring path, bool recursive)
{
	_done = false;
	fs::path path_to_dir_or_file = fs::path(path);
	try
	{
		if (!fs::exists(path_to_dir_or_file)) {
			log << _T("Invalid path: ") << path_to_dir_or_file.string<tstring>() << std::endl;
			return;
		}

		if(fs::is_directory(path_to_dir_or_file))
		{
			recursive ? TagDirectoryRecursive(path) : TagDirectory(path);
		}
		else if (fs::is_regular_file(path_to_dir_or_file))
		{
				TagFileOnThread(path_to_dir_or_file);
		}
		else
			log << _T("Invalid path: ") << path_to_dir_or_file.string<tstring>() << std::endl;

		_done = true;

		while(!_threads.empty()) {
			for(threadlist::const_iterator it = _threads.cbegin(); it !=_threads.cend();)
			{
				boost::thread* thread = it->first;
				time_t last_alive = it->second;
				threadlist::const_iterator it_prev = it;
				++it;
				if(last_alive < time(NULL) - 5) {	//consider dead
					//terminate
					HardKill(thread);
					_threads.remove(*it_prev);
					NewThread();
				}
			}
			boost::this_thread::sleep(boost::posix_time::milliseconds(100));
		}

	} catch (const fs::filesystem_error& ex) {
		log << ex.what() << std::endl;
	}
}

void FileTagger::TagDirectory(fs::path files_dir)
{

	try {
		if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) {
			log << _T("Invalid directory: ") << files_dir.string<tstring>() << std::endl;
			return;
		}

		for (fs::directory_iterator end, dir(files_dir); dir != end;
				++dir) {
			fs::path p = dir->path();

			if (fs::is_regular_file(p)) {
				tstring extension = p.extension().string<tstring>();
				boost::algorithm::to_lower(extension);
				if (extension != _T(".mp3")) {
					log << _T("Skipping ") <<  p.string<tstring>() << std::endl;
					continue;
				}
				
				TagFileOnThread(p);

			} else if (fs::is_directory(p)) {
				log << _T("Directory: ") << p.string<tstring>() << std::endl;

			} else {
				log << _T("Error reading: ") << p.string<tstring>() << std::endl;
			}
		}
	} catch (const fs::filesystem_error& ex) {
		log << ex.what() << std::endl;
	}
}

void FileTagger::TagDirectoryRecursive(fs::path files_dir)
{

	try {
		if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) {
			log << _T("Invalid directory: ") << files_dir.string<tstring>() << std::endl;
			return;
		}

		for (fs::recursive_directory_iterator end, dir(files_dir); dir != end;
				++dir) {
			fs::path p = dir->path();

			if (fs::is_regular_file(p)) {

				if (p.extension().string<tstring>() != _T(".mp3"))
					continue;

				TagFileOnThread(p);

			} else if (fs::is_directory(p)) {
				log << _T("Directory: ") << p.string<tstring>() << std::endl;

			} else {
				log << _T("Error reading: ") << p.string<tstring>() << std::endl;
			}
		}
	} catch (const fs::filesystem_error& ex) {
		log << ex.what() << std::endl;
	}
}

void FileTagger::NewThread()
{
	if(_threads.size() < _threads_max) {
		thread_info_type new_thread(NULL, time(NULL));
		_threads.push_back(new_thread);
		std::pair<boost::thread*, time_t> &thread_info = _threads.back();
		thread_info.second = time(NULL);

		thread_info.first = new boost::thread(boost::bind(&FileTagger::_thread_func, this, &thread_info.second));
	}
}
void FileTagger::TagFileOnThread(fs::path file)
{
	NewThread();
	boost::lock_guard<boost::mutex> lock(_mtx);
	_work_queue.push_back(file);
}

void FileTagger::_thread_func(time_t *last_alive)
{
	while(!_done || !_work_queue.empty()) {
		time(last_alive);
		fs::path file;
		{
			boost::lock_guard<boost::mutex> lock(_mtx);
			if(_work_queue.empty())
				continue;
			file = _work_queue.front();
			_work_queue.pop_front();
		}
		TagFile(file);
	}
}
void FileTagger::TagFile(fs::path file) const
{
	fs::path filec = fs::canonical(file).make_preferred().native();

	log << filec << std::endl;

	TagLib::FileRef f(filec.string<tstring>().c_str());

	if(!CheckEmptyFields(f)) {
		log << "Rejected: Non-Empty field(s)\n\n";
		return;
	}
	Pattern::position_map fields;

	tstring file_name;
	if(!ExtractRelevantFileName(filec, file_name)) {
		log << "Rejected: Filename path separator mismatch\n\n";
		return;
	}
	log << _T("RelevantFileName: ") << file_name << std::endl;

	if(_pattern.match(file_name, fields))
	{
		UpdateTags(f, fields);
		log << "Done\n\n";
		return;
	}

}

void FileTagger::UpdateTags(TagLib::FileRef &file, Pattern::position_map &fieldmap) const
{
	for (Pattern::position_map::iterator it = fieldmap.begin();	it!=fieldmap.end(); ++it) {
		Field &field = it->second;

		switch(field._type)
		{
		case Artist:
			if(!_safe) file.tag()->setArtist(field._content);
			log << _T("Artist = `") << field._content << _T("`") << std::endl;
			break;
		case Title:
			if(!_safe) file.tag()->setTitle(field._content);
			log << _T("Title = `") << field._content << _T("`") << std::endl;
			break;
		case Album:
			if(!_safe) file.tag()->setAlbum(field._content);
			log << _T("Album = `") << field._content << _T("`") << std::endl;
			break;
		case Genre:
			if(!_safe) file.tag()->setGenre(field._content);
			log << _T("Genre = `") << field._content << _T("`") << std::endl;
			break;
		case Comment:
			if(!_safe) file.tag()->setComment(field._content);
			log << _T("Comment = `") << field._content << _T("`") << std::endl;
			break;
		case TrackNo:
			if(!_safe) file.tag()->setTrack(atoi(field.ToCharArr()));
			log << _T("Track# = `") << field._content << _T("`") << std::endl;
			break;
		case Year:
			if(!_safe) file.tag()->setYear(atoi(field.ToCharArr()));
			log << _T("Year = `") << field._content << _T("`") << std::endl;
			break;

		default:
			break;
		}
	}
	if(!_safe) file.save();
}

bool FileTagger::CheckEmptyFields(TagLib::FileRef &file) const
{
	for (std::vector<tstring>::const_iterator it = _empty_fields.cbegin();
			it!=_empty_fields.cend(); ++it) {
		const tstring &field = *it;
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


bool FileTagger::ExtractRelevantFileName(fs::path file_path, tstring &out) const
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
