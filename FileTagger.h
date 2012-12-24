/*
 * FileTag.h
 *
 *  Created on: Dec 18, 2012
 *      Author: akavo
 */

#ifndef FILETAG_H_
#define FILETAG_H_

#include <boost/filesystem.hpp>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <map>
#include <vector>

namespace fs = boost::filesystem;
void list_files(std::string dir);

void test();

enum FieldType {Title = 0, Artist, Album, TrackNo, Delimiter, Unknown};

std::string FieldTypeToString(FieldType type);

//////////////////////////////////////////////////////////////////////////////////

class Field {
private:
	Field(): _type(Unknown) {}
public:
	Field(std::string content, FieldType type);
	virtual ~Field();
public:
	std::string _content;
	FieldType _type;
	size_t size() { return _content.size(); }
};

//////////////////////////////////////////////////////////////////////////////////

class Pattern {
public:
	typedef std::map<size_t, Field> position_map;

	Pattern(std::string format, bool trim);
	~Pattern();
protected:
	position_map _structure;
	position_map _delimiters;

	std::string _pattern;
	bool _valid;
	bool _trim;
	size_t _nNamedFields;
	size_t _nDelFields;

	bool parse();
	bool parse_helper(size_t pos, size_t size, size_t prev_pos, size_t prev_size);
	int find_in_pattern(Field needle);

public:
	bool match_string(std::string &file_stem, position_map &out);
	void print();
	static int find_insert_field(Field needle, std::string &haystack, position_map &out);
};

//////////////////////////////////////////////////////////////////////////////////

class FileTagger {
public:
	FileTagger(Pattern &p);
	virtual ~FileTagger();

	void SetEmptyFieldConstraint(std::vector<std::string> empty_fields);
	void SetSafeMode(bool safe_mode);
	void Tag(std::string path, bool recursive);
	void TagDirectory(std::string dir, bool recursive);
	void TagFile(std::string file);

protected:
	void UpdateTags(TagLib::FileRef &file, Pattern::position_map &fieldmap);
	bool CheckEmptyFields(TagLib::FileRef &file);
	void TagDirectory(fs::path dir);
	void TagDirectoryRecursive(fs::path dir);
	void TagFile(fs::path file);

protected:
	Pattern &_pattern;
	std::vector<std::string> _empty_fields;
	bool _safe;
};

#endif /* FILETAG_H_ */
