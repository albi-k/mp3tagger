/*
 * FileTag.h
 *
 *  Created on: Dec 18, 2012
 *      Author: akavo
 */

#ifndef FILETAG_H_
#define FILETAG_H_

#define TAGLIB_STATIC 

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <list>
#include <map>
#include <vector>
#include <time.h>
#include "common.h"


//////////////////////////////////////////////////////////////////////////////////

enum FieldType {Title = 0, Artist, Album, TrackNo, Genre,
				Year, Comment, Ignore, Delimiter, _Unknown};

tstring FieldTypeToString(FieldType type);

//////////////////////////////////////////////////////////////////////////////////

class Field {
private:
	Field(): _type(_Unknown) {}
	static std::string content_narrow;
	const char* ToCharArr(std::string &src);
	const char* ToCharArr(std::wstring &src);
public:
	Field(tstring content, FieldType type);
	virtual ~Field();
public:
	tstring _content;
	FieldType _type;
	size_t size() { return _content.size(); }

	const char* ToCharArr();
};

//////////////////////////////////////////////////////////////////////////////////

class Pattern {
public:
	typedef std::map<size_t, Field> position_map;

	Pattern(tstring format, bool trim);
	~Pattern();
protected:
	position_map _structure;
	position_map _delimiters_generic;

	tstring _pattern;
	bool _valid;
	bool _trim;
	tstring _trim_chars;

	size_t _nNamedFields;
	size_t _nDelFields;
	size_t _nPathSeparators;

	bool parse();
	bool parse_helper(size_t pos, size_t size, size_t prev_pos, size_t prev_size);
	int find_in_pattern(Field needle);

public:
	bool match(tstring &file_stem, position_map &out) const;
	void print() const;
	static int find_insert_field(Field needle, tstring &haystack, position_map &out);

	size_t get_separator_count() { return _nPathSeparators; }
	bool begins_with_separator();
	void SetTrimChars(tstring chars) { _trim_chars = chars; }
};

//////////////////////////////////////////////////////////////////////////////////

class FileTagger {
public:
	FileTagger(Pattern &p, bool replace_non_empty=true);
	virtual ~FileTagger();

	void SetEmptyFieldConstraint(std::vector<tstring> &empty_fields);
	void SetSafeMode(bool safe_mode);
	void SetThreadCount(unsigned int count) { _threads_max = count;}
	void Tag(tstring path, bool recursive);

protected:
	void UpdateTags(TagLib::FileRef &file, Pattern::position_map &fieldmap) const;
	bool CheckEmptyFields(TagLib::FileRef &file) const;
	void TagDirectory(fs::path dir);
	void TagDirectoryRecursive(fs::path dir);
	void TagFile(fs::path file) const;
	void TagFileOnThread(fs::path file);
	void _thread_func(time_t *last_alive);
	void NewThread();
	bool ExtractRelevantFileName(fs::path file_path, tstring &out) const;

protected:
	Pattern &_pattern;
	std::vector<tstring> _empty_fields;
	bool _safe;						//safe mode: don't write changes
	bool _replace;					//replace if tag exists?
	//Threads
	typedef std::pair<boost::thread*, time_t>	thread_info_type;
	typedef std::list<thread_info_type> threadlist;
	typedef std::list<fs::path> worklist;
	//
	unsigned int _threads_max;		//# of workers
	threadlist _threads;
	worklist _work_queue;
	boost::mutex _mtx;
	bool _done;
};

#endif /* FILETAG_H_ */
