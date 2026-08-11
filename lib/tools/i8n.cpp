#include <tools/i8n.h>

#include <tools/string_utils.h>
#include <tools/file_utils.h>
#include <tools/algorithm.h>
#include <tools/platform.h>

#include <algorithm>
#include <ctype.h>
#include <iostream>			//For the debug methods.
#include <iterator>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
// Exceptions

tools::i8n_exception::i8n_exception(
	const std::string& _err
)
	:std::runtime_error(_err) {

}

tools::i8n_exception_no_path::i8n_exception_no_path()
	:i8n_exception("cannot load i8n : no path specified") {

}

tools::i8n_exception_no_language::i8n_exception_no_language()
	:i8n_exception("cannot load i8n : no language specified") {

}

tools::i8n_exception_invalid_fail_entry::i8n_exception_invalid_fail_entry(const std::string& _str)
	:i8n_exception("Invalid fail entry string '"+_str+"'") {

}

tools::i8n_exception_file_error::i8n_exception_file_error(const std::string& _path)
	:i8n_exception("could not open file "+_path) {

}

tools::i8n_repeated_path::i8n_repeated_path(const std::string& _path)
	:i8n_exception("repeated path '"+_path+"' cannot be added twice") {

}

tools::i8n_lexer_error_with_file::i8n_lexer_error_with_file(const std::string& _err, const std::string& _file)
	:i8n_exception("lexer error: "
		+_err
		+" in file "
		+_file) {
}

tools::i8n_lexer_generic_error::i8n_lexer_generic_error(const std::string& _err)
	:std::runtime_error(_err) {

}

tools::i8n_delimiter_exception::i8n_delimiter_exception()
	:i8n_exception("invalid delimiters size: delimiters must be 2 chars long") {

}


tools::i8n_parser_token_error::i8n_parser_token_error(const std::string _err, int _line, int _char)
	:i8n_exception(_err
		+" in line "
		+std::to_string(_line)
		+" ending at char "
		+std::to_string(_char)) {

}

tools::i8n_parser_error::i8n_parser_error(const std::string _err)
	:i8n_exception("parser error in "+_err) {

}

////////////////////////////////////////////////////////////////////////////////
// Main

//!Class constructor with path and default language.
tools::i8n::i8n(
	const std::string& _path, 
	const std::string& _lan, 
	const std::vector<std::string>& _input
)
:file_path(_path), language(_lan) {

	create_default_error_entry();

	if(_input.size()) {

		for(const auto& _i : _input) {
			paths.push_back({_i});
		}

		reload_codex();
	}
}

//!Class constructor with path and default language.
tools::i8n::i8n(
	const std::string& _path, 
	const std::string& _lan
)
	:file_path(_path), language(_lan) {

	create_default_error_entry();
}

void tools::i8n::create_default_error_entry() {

	set_fail_entry(
		delimiter_set.open_value
		+"ERROR : could not locate i8n key "
		+delimiter_set.open_var
		+"__key__"
		+delimiter_set.close_var
		+delimiter_set.close_value);
}

void tools::i8n::add_file(
	const std::string& _path,
	bool _rebuild
) {

	if(!_path.size()) {
		throw i8n_exception_no_path{};
	}

	if(!language.size()) {
		throw i8n_exception_no_language{};
	}

	for(const auto& path : paths) {

		if(path.path==_path) {

			throw i8n_repeated_path{_path};
		}
	}

	//This is added in the public interface: the private interface may do its own
	//calls that might interfere with this.
	paths.push_back({_path});

	if(_rebuild) {

		reload_codex();
	}
}

void tools::i8n::add_user_file(
	const std::string& _path,
	const std::string& _root,
	bool _rebuild
) {

	if(!_path.size()) {
		throw i8n_exception_no_path{};
	}

	if(!language.size()) {
		throw i8n_exception_no_language{};
	}

	for(const auto& path : paths) {

		if(path.path==_path) {

			throw i8n_repeated_path{_path};
		}
	}

	//This is added in the public interface: the private interface may do its own
	//calls that might interfere with this.
	paths.push_back({_path, _root});

	if(_rebuild) {

		reload_codex();
	}
}

void tools::i8n::add_files(
	const std::vector<std::string>& _paths,
	bool _rebuild
) {

	for(const auto& path : _paths) {

		add_file(path, false);
	}

	if(_rebuild) {

		reload_codex();
	}
}

void tools::i8n::build() {

	reload_codex();
}

void tools::i8n::lexicalize_file(
	const path_structure& _path, 
	std::map<std::string, std::vector<lexer::token>>& _lexer_tokens
) {

	const std::string path=0==_path.root.size() 
		? file_path+"/"+language+"/"+_path.path
		: _path.root+"/"+language+"/"+_path.path;

	std::ifstream file(path);

	if(!file) {
		throw i8n_exception_file_error(path);
	}

	lexer lx{delimiter_set};
	_lexer_tokens[_path.path]=lx.from_file(path);
}

void tools::i8n::set(
	const substitution& _sub
) {

	auto it=std::find_if(
		std::begin(substitutions), 
		std::end(substitutions), 
		[&_sub](const substitution& _substitution) {

			return _substitution.key==_sub.key;
	});

	if(std::end(substitutions)==it) {
		substitutions.push_back(_sub);
	}
	else {
		it->value=_sub.value;
	}
}

void tools::i8n::set_root(
	const std::string& _path
) {

	file_path=_path;
	reload_codex();
}

void tools::i8n::set_language(
	const std::string& _lan
) {

	language=_lan;
	reload_codex();
}

std::string tools::i8n::get(
	const std::string& _get
) const {

	if(!codex.count(_get)) {
		return fail_string(_get);
	}

	return codex.at(_get).get(substitutions);
}

std::string tools::i8n::get(
	const std::string& _get, 
	const std::vector<substitution>& _subs
) const {

	if(!codex.count(_get)) {
		return fail_string(_get);
	}

	return codex.at(_get).get(_subs, substitutions);
}

tools::i8n::delimiters tools::i8n::get_delimiters() const {

	return delimiter_set;
}

void tools::i8n::set_delimiters(
	const tools::i8n::delimiters& _delim
) {

	//Sanity check: are all delimiters the correct size?
	std::vector<std::string> del{
		_delim.open_label,
		_delim.close_label,
		_delim.open_value,
		_delim.close_value,
		_delim.open_var,
		_delim.close_var,
		_delim.open_embed,
		_delim.close_embed
	};

	if(std::any_of(std::begin(del), std::end(del), [](const std::string& _s) {return 2!=_s.size();})) {
		throw i8n_delimiter_exception{};
	}

	delimiter_set=_delim;
}

std::string tools::i8n::fail_string(
	const std::string& _get
) const {

	return fail_entry.get({{"__key__", _get}});
}

void tools::i8n::build_entries(
	std::map<std::string, std::vector<lexer::token>>& _lexer_tokens
) {

	parser pr;
	codex=pr.parse(_lexer_tokens);
}

void tools::i8n::reload_codex() {

	codex.clear();
	std::map<std::string, std::vector<lexer::token>> lexer_tokens;
	for(auto& p : paths) {
		lexicalize_file(p, lexer_tokens);
	}

	build_entries(lexer_tokens);
}

void tools::i8n::set_fail_entry(
	const std::string& _str
) {

	try {
		lexer lx{delimiter_set};
		parser pr;
		fail_entry=pr.parse(lx.from_string(_str));
	}
	catch(i8n_exception& e) {
		throw i8n_exception_invalid_fail_entry(_str+" : "+e.what());
	}
}

////////////////////////////////////////////////////////////////////////////////
// Lexer.

tools::i8n::lexer::lexer(
	const delimiters& _del
)
	:delim(_del) {

}

std::vector<tools::i8n::lexer::token> tools::i8n::lexer::from_file(
	const std::string& _filepath
) const {

	std::ifstream file(_filepath.c_str());
	if(!file) {
		throw i8n_lexer_generic_error("cannot open file "+_filepath);
	}

	try {
		return from_string(tools::dump_file(_filepath));
	}
	catch(i8n_lexer_generic_error& e) {
		throw i8n_lexer_error_with_file(e.what(), _filepath);
	}
}

std::vector<tools::i8n::lexer::token> tools::i8n::lexer::from_string(
	const std::string& _raw_text
) const {

	std::string line;
	int linenum=0, charnum=0;
	auto lines=tools::explode(_raw_text, tools::newline);

	//The lexer just reads line by line, storing data as tokens are found.
	std::string buffer;
	std::vector<tools::i8n::lexer::token>	result;

	while(lines.size()) {
		//Pop from front...
		line=lines.front();
		lines.erase(std::begin(lines));
		++linenum;
		charnum=0;

		//Skip comments... Blank lines will not be skipped, as they might carry meaning!
		//check line size because this fails an assertion in windows!
		if(0!=line.size() && delim.comment==line.front()) {
			continue;
		}

		line+=tools::newline; //Newlines are lost in getline, but they might carry significance here...
		while(line.size()) {

			++charnum;
			buffer+=line.front();
			line.erase(0, 1);

			auto size=buffer.size();
			if(size >= 2) {

				auto last_two=buffer.substr(size-2);

				auto type=scan_buffer(last_two);
				if(tokentypes::nothing!=type) {

					if(size > 2) {
						result.push_back({tokentypes::literal, buffer.substr(0, size-2), linenum, charnum-2});
					}

					result.push_back({type, last_two, linenum, charnum});
					buffer.clear();

					//When closing a value we discard the rest of the line. A little convenience thing.
					if(tokentypes::closevalue==type) {
						line.clear();
					}
				}
			}
		}
	}

	//!The last thing we expect is actually a delimiter, so this is an error.
	if(str_trim(buffer).size()) {
		throw i8n_lexer_generic_error("non-token found at the end of the stream: '"+buffer+"'");
	}

	return result;
}

tools::i8n::lexer::tokentypes tools::i8n::lexer::scan_buffer(
	const std::string& _control
) const {

	assert(2==_control.size());

	if(delim.open_label==_control) 		return tokentypes::openlabel;
	else if(delim.close_label==_control) 	return tokentypes::closelabel;
	else if(delim.open_value==_control) 	return tokentypes::openvalue;
	else if(delim.close_value==_control)	return tokentypes::closevalue;
	else if(delim.open_var==_control) 	return tokentypes::openvar;
	else if(delim.close_var==_control) 	return tokentypes::closevar;
	else if(delim.open_embed==_control) 	return tokentypes::openembed;
	else if(delim.close_embed==_control) 	return tokentypes::closeembed;
	return tokentypes::nothing;
}

std::string tools::i8n::lexer::typetostring(
	tokentypes _type
) {

	switch(_type) {
		case tokentypes::openlabel: return "open label";
		case tokentypes::closelabel: return "close label";
		case tokentypes::openvalue: return "open value";
		case tokentypes::closevalue: return "close value";
		case tokentypes::openvar: return "open variable";
		case tokentypes::closevar: return "close variable";
		case tokentypes::openembed: return "open embed";
		case tokentypes::closeembed: return "close embed";
		case tokentypes::literal:	return "literal";
		case tokentypes::nothing:	return "nothing";
	}

	return "shut up compiler";
}

////////////////////////////////////////////////////////////////////////////////
// Parser.

//TODO: I don't like how these two parse functions are radically different
//in how they work internally.
tools::i8n::codex_entry tools::i8n::parser::parse(
	const std::vector<lexer::token>& _tokens
) const {

	int curtoken=0, size=_tokens.size();
	return value_phase(_tokens, curtoken, size);
}

std::map<std::string, tools::i8n::codex_entry> tools::i8n::parser::parse(
	const std::map<std::string, std::vector<lexer::token>>& _lexer_tokens
) const {

	std::map<std::string, codex_entry>	entries;

	for(const auto& pair : _lexer_tokens) {
	
		try {
			interpret_tokens(pair.second, entries);
		}
		catch(i8n_parser_error& e) {
			
			throw i8n_parser_error(e.what()+std::string{" in "}+pair.first);
		}
	}

	check_integrity(entries);

	auto solved=compile_entries(entries);
	for(auto& pair : solved) {
		compact_entry(pair.second);
	}

	return solved;
}

void tools::i8n::parser::compact_entry(
	codex_entry& _entry
) const {

	auto it=std::begin(_entry.segments);

	while(true) {

		auto next=it+1;
		if(next >= std::end(_entry.segments)) {
			break;
		}

		if(entry_segment::types::literal==it->type && it->type==next->type) {
			it->value+=next->value;
			it=_entry.segments.erase(next);
			continue;
		}

		++it;
	}
}

std::map<std::string, tools::i8n::codex_entry> tools::i8n::parser::compile_entries(
	std::map<std::string, tools::i8n::codex_entry>& _entries
) const {

	std::map<std::string, codex_entry> solved;

	size_t last_solved=0;
	while(_entries.size()) {

		//Move solvable entries around...
		for(auto it=std::begin(_entries); it != std::end(_entries);) {

			if(solve_entry(it->second, solved)) {
				solved[it->first]=it->second;
				it=_entries.erase(it);
			}
			else {
				it++;
			}
		}

		//Nothing was solved in this pass: circular references!!!
		if(last_solved==solved.size()) {

			typedef const std::pair<std::string, codex_entry> tpair;
			throw i8n_parser_error(std::string("circular references found in data :")+tools::reduce(
				std::begin(_entries),
				std::end(_entries),
				[](std::string&& _carry, const tpair& _item){return _carry+=_item.first+" ";},
				"")
			);
		}

		last_solved=solved.size();
	}

	return solved;
}


#ifdef WITH_DEBUG_CODE
void tools::i8n::parser::debug(
	const std::vector<lexer::token>& _tokens, 
	std::ostream& _stream
) const {
#else
void tools::i8n::parser::debug(
	const std::vector<lexer::token>&, 
	std::ostream&
) const {
#endif

#ifdef WITH_DEBUG_CODE
	for(const auto& token : _tokens) {
		debug(token, _stream);
	}
#endif
}

#ifdef WITH_DEBUG_CODE
void tools::i8n::parser::debug(
	const lexer::token& _token, 
	std::ostream& _stream
) const {
#else
void tools::i8n::parser::debug(
	const lexer::token&, 
	std::ostream&
) const {
#endif

#ifdef WITH_DEBUG_CODE
	switch(_token.type) {
		case lexer::tokentypes::openlabel: 	_stream<<"[OPENLAB] "<<_token.line<<":"<<_token.charnum<<std::endl; break;
		case lexer::tokentypes::closelabel: _stream<<"[CLSELAB] "<<_token.line<<":"<<_token.charnum<<std::endl; break;
		case lexer::tokentypes::openvalue: 	_stream<<"[OPENVAL] "<<_token.line<<":"<<_token.charnum<<std::endl; break;
		case lexer::tokentypes::closevalue:	_stream<<"[CLSEVAL] "<<_token.line<<":"<<_token.charnum<<std::endl; break;
		case lexer::tokentypes::openvar:	_stream<<"[OPENVAR] "<<_token.line<<":"<<_token.charnum<<std::endl; break;
		case lexer::tokentypes::closevar:	_stream<<"[CLSEVAR] "<<_token.line<<":"<<_token.charnum<<std::endl; break;
		case lexer::tokentypes::openembed:	_stream<<"[OPENEMB] "<<_token.line<<":"<<_token.charnum<<std::endl; break;
		case lexer::tokentypes::closeembed:	_stream<<"[CLSEEMB] "<<_token.line<<":"<<_token.charnum<<std::endl; break;
		case lexer::tokentypes::nothing:	_stream<<"[NOTHING] "<<_token.line<<":"<<_token.charnum<<std::endl; break;
		case lexer::tokentypes::literal:	_stream<<"[LITERAL] "<<_token.line<<":"<<_token.charnum<<" '"<<_token.val<<"'"<<std::endl; break;
	}
#endif
}

#ifdef WITH_DEBUG_CODE
void tools::i8n::parser::debug(
	const codex_entry& _entry, 
	std::ostream& _stream
) const {
#else
void tools::i8n::parser::debug(
	const codex_entry&, 
	std::ostream&
) const {
#endif

#ifdef WITH_DEBUG_CODE
	for(const auto& s : _entry.segments) {
		debug(s, _stream);
	}
#endif
}

#ifdef WITH_DEBUG_CODE
void tools::i8n::parser::debug(
	const entry_segment& _segment, 
	std::ostream& _stream
) const {
#else
void tools::i8n::parser::debug(
	const entry_segment&, 
	std::ostream&
) const {
#endif

#ifdef WITH_DEBUG_CODE
	switch(_segment.type) {
		case entry_segment::types::literal: 	_stream<<"[LIT] "<<_segment.value<<std::endl; break;
		case entry_segment::types::variable:	_stream<<"[VAR] "<<_segment.value<<std::endl; break;
		case entry_segment::types::embed: 		_stream<<"[EMB] "<<_segment.value<<std::endl; break;
	}
#endif
}

void tools::i8n::parser::interpret_tokens(
	const std::vector<lexer::token>& _tokens, std::map<std::string, codex_entry>& _entries
) const {

	int curtoken=0,
		size=_tokens.size()-1;

	std::string curlabel;

	while(true) {

		curlabel=label_phase(_tokens, curtoken, size);
		_entries[curlabel]=value_phase(_tokens, curtoken, size);

		if(curtoken >= size) {
			break;
		}
	}
}

void tools::i8n::parser::check_integrity(
	const std::map<std::string, codex_entry>& _entries
) const {

	typedef const std::pair<std::string, codex_entry> tpair;
	size_t total=tools::reduce(std::begin(_entries), std::end(_entries), [](size_t&& _carry, tpair _pair) {return _carry+=_pair.second.segments.size();}, 0);
	std::vector<entry_segment> embeds(total);

	for(const auto& pair : _entries) {
		std::copy_if(std::begin(pair.second.segments), std::end(pair.second.segments), std::begin(embeds), [this, &_entries](const entry_segment& _seg) {
			return entry_segment::types::embed==_seg.type
				&& !_entries.count(_seg.value);
		});
	}

	if(embeds.size()) {
		std::string list=tools::reduce(std::begin(embeds), std::end(embeds), [](std::string&& _carry, const entry_segment& _seg) {_carry+=_seg.value+",";}, "");
		list.pop_back(); //Remove last comma.
		throw i8n_parser_error("undefined references found in data : "+list);
	}
}

bool tools::i8n::parser::solve_entry(
	codex_entry& _entry, 
	std::map<std::string, codex_entry>& _solved
) const {

	for(auto it=std::begin(_entry.segments); it != std::end(_entry.segments); it++) {

		auto& segment=*it;

		if(entry_segment::types::embed==segment.type) {

			const std::string key=segment.value;

			if(!_solved.count(key)) {
				return false;
			}

			//Remove the embed entry. Step back to insert the new stuff...
			it=_entry.segments.erase(it);

			//Add the new ones... Reset: the new segments may include embeds too!
			const auto& new_segments=_solved.at(key).segments;
			_entry.segments.insert(it, std::begin(new_segments), std::end(new_segments));
			it=std::begin(_entry.segments);
		}
	}

	return true;
}

std::string tools::i8n::parser::label_phase(
	const std::vector<lexer::token>& _tokens, 
	int& _curtoken, 
	const int _size
) const {

	//Trim all whitespace tokens before the first opening...
	_curtoken=find_next_of(_tokens, lexer::tokentypes::openlabel, _curtoken);

	//Either there are not tokens or there's only whitespace.
	if(_curtoken > _size) {
		throw i8n_parser_error("unexpected end, expecting open label");
	}

	std::string label=parse_open_close(_tokens, lexer::tokentypes::closelabel, _curtoken);
	_curtoken+=3;

	return label;
}

tools::i8n::codex_entry tools::i8n::parser::value_phase(
	const std::vector<lexer::token>& _tokens, 
	int& _curtoken, 
	const int _size
) const {

	//Store these, we are skipping to the "value open" now and might get to the end of the tokens.
	int line=_tokens[_curtoken].line,
		charnum=_tokens[_curtoken].charnum;

	_curtoken=find_next_of(_tokens, lexer::tokentypes::openvalue, _curtoken);
	if(_curtoken > _size) {
		throw i8n_parser_token_error("unexpected end, expecting value open", line, charnum);
	}

	//Curtoken is sits now at the beginning of the next token, if any...
	++_curtoken;

	codex_entry entry;

	//Now, read until we find a "close value...""
	while(true){

		//If there are no more tokens, the syntax of the file was not ok...
		if(_curtoken > _size) {
			throw i8n_parser_token_error("unexpected end, value not closed after ", line, charnum);
		}

		const auto& tok=_tokens[_curtoken];
		auto curtype=tok.type;

		//We are looking for literals, openvar or openembed...
		switch(curtype) {
			case lexer::tokentypes::literal:
				entry.segments.push_back({entry_segment::types::literal, tok.val});
			break;
			case lexer::tokentypes::openvar:
				entry.segments.push_back({entry_segment::types::variable, parse_open_close(_tokens, lexer::tokentypes::closevar, _curtoken) });
				_curtoken+=2;
			break;
			case lexer::tokentypes::openembed:
				entry.segments.push_back({entry_segment::types::embed, parse_open_close(_tokens, lexer::tokentypes::closeembed, _curtoken) });
				_curtoken+=2;
			break;
			case lexer::tokentypes::closevalue:
				++_curtoken;
				return entry;
			break;
			default:
				throw i8n_parser_token_error("unexpected '"+lexer::typetostring(curtype)+"' inside value", tok.line, tok.charnum);
		}

		++_curtoken;
	}
}

std::string tools::i8n::parser::parse_open_close(
	const std::vector<lexer::token>& _tokens, 
	lexer::tokentypes _closetype, 
	int _curtoken
) const {

	//Skip the opening...
	++_curtoken;
	if((size_t)_curtoken >= _tokens.size()) {
		throw i8n_parser_error("unexpected end of tokens");
	}

	//Check we have a literal...
	if(lexer::tokentypes::literal!=_tokens[_curtoken].type) {
		throw i8n_parser_token_error("unexpected '"+lexer::typetostring(_tokens[_curtoken].type)+"', expecting literal", _tokens[_curtoken].line, _tokens[_curtoken].charnum);
	}

	std::string result=_tokens[_curtoken].val;

	//Skip the value and check we are closing...
	++_curtoken;
	if((size_t)_curtoken >= _tokens.size()) {
		throw i8n_parser_error("unexpected end of file");
	}

	if(_closetype!=_tokens[_curtoken].type) {
		throw i8n_parser_token_error("unexpected '"+lexer::typetostring(_tokens[_curtoken].type)+"', expecting '"+lexer::typetostring(_closetype)+"'", _tokens[_curtoken].line, _tokens[_curtoken].charnum);
	}

	return result;
}

int tools::i8n::parser::find_next_of(
	const std::vector<lexer::token>& _tokens, 
	lexer::tokentypes _type, 
	int _curtoken
) const {

	int size=_tokens.size()-1;
	while(_curtoken < size) {

		const auto& tok=_tokens[_curtoken];
		if(_type==tok.type)  {
			return _curtoken;
		}

		if(lexer::tokentypes::literal==tok.type) {
			if(!str_trim(tok.val).size()) {
				++_curtoken;
				continue;
			}

			throw i8n_parser_token_error("unskippable "+lexer::typetostring(tok.type)+", expecting '"+lexer::typetostring(_type)+"'", tok.line, tok.charnum);
		}

		throw i8n_parser_token_error("unexpected token, expecting '"+lexer::typetostring(_type)+"', found '"+lexer::typetostring(tok.type)+"'", tok.line, tok.charnum);
	}

	return _curtoken;
}

////////////////////////////////////////////////////////////////////////////////
// Codex entry.

std::string tools::i8n::codex_entry::get(
	const std::vector<substitution>& _subs
) const {

	std::string result;
	for(const auto& seg : segments) {

		if(entry_segment::types::literal==seg.type) {
			result+=seg.value;
		}
		else if(entry_segment::types::variable==seg.type) {
			substitute(seg.value, _subs, result);
		}
	}

	return result;
}

std::string tools::i8n::codex_entry::get(
	const std::vector<substitution>& _subs, 
	const std::vector<substitution>& _base_subs
) const {

	std::string result;
	for(const auto& seg : segments) {

		if(entry_segment::types::literal==seg.type) {
			result+=seg.value;
		}
		else if(entry_segment::types::variable==seg.type) {
			if(!substitute(seg.value, _subs, result)) {
				substitute(seg.value, _base_subs, result);
			}
		}
	}

	return result;
}

bool tools::i8n::codex_entry::substitute(
	const std::string& _key, 
	const std::vector<substitution>& _subs, 
	std::string& _res
) const {

	const auto it=std::find_if(std::begin(_subs), std::end(_subs), [&_key](const substitution& _sub) {
		return _sub.key==_key;
	});

	if(std::end(_subs)!=it) {
		_res+=it->value;
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////
// Helpers.

bool tools::i8n::substitution::operator==(
	const substitution& _o
) const {

	return _o.key==key
		&& _o.value==value;
}

tools::i8n::delimiters::delimiters()
	:open_label{"[["},
	close_label{"]]"},
	open_value{"{{"},
	close_value{"}}"},
	open_var{"(("},
	close_var{"))"},
	open_embed{"<<"},
	close_embed{">>"},
	comment{'#'} {
}
