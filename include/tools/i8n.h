#pragma once

#include <string>
#include <vector>
#include <array>
#include <map>
#include <stdexcept>
#include <fstream>

namespace tools {

//!Base exception for the module.
class i8n_exception
	:public std::runtime_error {
	public:
					i8n_exception(const std::string&);
};

//!Thrown when calling "add" when no path has been specified.
class i8n_exception_no_path
	:public i8n_exception {
	public:
					i8n_exception_no_path();
};

//!Thrown when calling "set_fail_entry" with an invalid entry string.
class i8n_exception_invalid_fail_entry
	:public i8n_exception {
	public:
					i8n_exception_invalid_fail_entry(const std::string&);
};


//!Thrown when trying to add the same path multiple times.
class i8n_repeated_path
	:public i8n_exception {
	public:
					i8n_repeated_path(const std::string&);
};

//!Thrown when calling "add" when no language has been specified.
class i8n_exception_no_language
	:public i8n_exception {
	public:
					i8n_exception_no_language();
};

//!Thrown when calling "set_delimiters" with invalid sizes.
class i8n_delimiter_exception
	:public i8n_exception {
	public:
					i8n_delimiter_exception();
};

//!Thrown when a file cannot be added.
class i8n_exception_file_error
	:public i8n_exception {
	public:
					i8n_exception_file_error(const std::string&);
};

//!Lexer exception with file information
class i8n_lexer_error_with_file
	:public i8n_exception {
	public:
					i8n_lexer_error_with_file(const std::string&, const std::string&);
};

//!Minimal lexer exception.
class i8n_lexer_generic_error
	:public std::runtime_error {
	public:
					i8n_lexer_generic_error(const std::string&);
};

//!Unexpected tokens when parsing, inner use.
class i8n_parser_token_error
	:public i8n_exception {
	public:
					i8n_parser_token_error(const std::string, int, int);
};

//!Failure to parse, visible from the outside.
class i8n_parser_error
	:public i8n_exception {
	public:
					i8n_parser_error(const std::string);
};

//!Simple internationalization module. Supports embedding of entries and 
//!variables. As a design decision, all entries must exist whitin files, to make
//!sure the module can keep a list of data sources when the language changes.

//TODO: provide an interface, so we can provide a mock to work with it, just
//in case.

class i8n_mock {

	public:
						i8n_mock(const std::string&, const std::string&, const std::vector<std::string>&) {}
						i8n_mock(const std::string&, const std::string&) {}
	void					add_file(const std::string&) {}
	void					set_root(const std::string&) {}
	void					set_language(const std::string&) {}
	std::string				get(const std::string&) const {return "mock";}
	void					set_fail_entry(const std::string&) {}
};

class i8n {

	public:

	//!Fed to "get" methods, to substitute variables.
	struct substitution {
		std::string		key,
						value;
		bool 			operator==(const substitution&) const;
	};

	//!Delimiters for the lexer. Constructed by default with sensible
	//!values.
	struct delimiters {

							delimiters();

		std::string	open_label,
							close_label,
							open_value,
							close_value,
							open_var,
							close_var,
							open_embed,
							close_embed;

		char 			comment;
	};

	//!Class constructor with root path, default language and list of files. 
	//!Assumes that all files given are located under their respective
	//!languages in the root path!languages in the root path..
							i8n(const std::string&, const std::string&, const std::vector<std::string>&);
	//!Class constructor with root path and default language.
							i8n(const std::string&, const std::string&);

	//!Adds the given file to the database. Will throw on failure to
	//!or if no path/language has been set (along with parser and lexer
	//!errors. Calling add will trigger a recompilation of all texts, so
	//!the preferred way adding texts is by doing so in the constructor. If no
	//!recompilation if wanted the second parameter can be set to false.
	void					add_file(const std::string&, bool=true);

	//!Same as add_file, but uses a list of paths. If the second parameter is
	//!true a recompilation of texts will take place after the last path in
	//!the list has been processed.
	void					add_files(const std::vector<std::string>&, bool=true);

	//!Like add_file, but points to a path that is not under the root described
	//!in this class, for example, an user provided path that can be anywhere
	//!into the filesystem represented by the second argument. These must 
	//!respect the conventions for this class, meaning that valid arguments
	//!for this method would be "myfile.i8n", "path/to/mydir", but my_dir
	//!would have to be followed by the language string, thus files would
	//!be in path/to/mydir/en/myfile.i8n.
	void					add_user_file(const std::string&, const std::string&, bool=true);

	//!Triggers a recompilation of all texts, which includes solving 
	//!substitutions of embeds and variables. This only makes sense when 
	//!add_file/add_files has been called with the second parameter set to false.
	void                    build();

	//!Adds a permanent substitution.
	void					set(const substitution&);

	//!Sets the root of the files in the filesystem. Will reload the database of texts.
	//!The root directory is the one under which the subdirectories representing
	//!languages for the application is, which assumes a centralized directory
	//!where all these can be found. The add_user_file method can be used to
	//!point to files anywhere else.
	void					set_root(const std::string&);

	//!Sets the current language key. Will reload the database of texts
	//!and trigger a recompilation. The recompilation will fail if the new
	//!language does not include the same complete set of files as the
	//!original.
	void					set_language(const std::string&);

	//!Retrieves - from the key database - the given text.
	//!Returns a fail string if not found.
	std::string				get(const std::string&) const;

	//!Retrieves - from the key database - the given text performing the
	//!substitions passed. Returns a fail string if not found. Substitutions are
	//!checked first agains the parameter, then against the "substitutions"
	//!property.
	std::string				get(const std::string&, const std::vector<substitution>&) const;

	//!Allows passing a value string that will act as a codex_entry to be
	//!translated when a key cannot be found in a call to get. The entry
	//!must accept the variable __key__, which will represent the failed key,
	//!Will throw parser and lexer errors if the string cannot be parsed.
	void					set_fail_entry(const std::string&);

	//!Returns a copy of the delimiters.
	delimiters				get_delimiters() const;

	//!Assigns the delimiters. Checks them for valid lengths. Throws if
	//!invalid lengths are found.
	void					set_delimiters(const delimiters&);

	private:

	//!Represents a path there text lives, which can under be under its language
	//!in the root directory or somewhere else (e.g, an user provided path).
	struct path_structure {

		std::string path,
		            root{""}; //empty always unless this is a user provided path!
	};

	//!Files are resolved to entries (one entry per data item). Each entry is
	//!composed by segments, which represent a fixed test or a variable to be resolved.
	struct entry_segment {
		enum class types {literal, variable, embed}	type;	//!<These are the different entry types. Embed should only exist when compiling.
		std::string									value;
	};

	//!Entry in the i8n dictionary.
	struct codex_entry {
		std::vector<entry_segment>		segments;
		//!Returns a translation of the segments, substituting variables for the vectors given.
		std::string						get(const std::vector<substitution>&) const;
		std::string						get(const std::vector<substitution>&, const std::vector<substitution>&) const;

		private:
		//!Performs the substitution of the given key with the substitution vector, into the last string.
		bool							substitute(const std::string&, const std::vector<substitution>&, std::string&) const;
	};

	//!Internal lexer: converts files into streams of tokens.
	class lexer {
		public:
					lexer(const delimiters&);


		//!The different ytpes
		enum class tokentypes {openlabel, closelabel, openvalue, closevalue,
			openvar, closevar, openembed, closeembed, nothing, literal};

		//!Represents a single lexer token (linguistic token or literal).
		struct token {
			tokentypes	type;
			std::string	val;
			int			line, charnum;
		};

		static std::string	typetostring(tokentypes);

		//!Processes the file of the given filename. Returns a list of
		//!lexer tokens.
		std::vector<token>	from_file(const std::string&) const;
		//!Processes tokens from the raw string. Returns a list of
		//!lexer tokens.
		std::vector<token>	from_string(const std::string&) const;

		private:

		//!Scans two characters to see if they correspond with delimiters,
		//!returning the token type (nothing if none detected).
		tokentypes			scan_buffer(const std::string&) const;

		const delimiters&		delim;
	};

	//!Internal parser, converts tokens into codex entries. Given that codex
	//!entries can have dependencies between them, this also solves then in
	//!as many passes as needed. Circular dependencies cause it to throw.
	class parser {

		public:

		//!Parses a map of string to tokens to a map of strings to codex entries.
		std::map<std::string, codex_entry>			parse(const std::map<std::string, std::vector<lexer::token>> &) const;
		//!Parses the tokens to a codex entry.
		codex_entry									parse(const std::vector<lexer::token>&) const;

		//!Prints out the tokens to the given stream, for debug purposes.
		void			debug(const std::vector<lexer::token>&, std::ostream&) const;
		void			debug(const lexer::token&, std::ostream&) const;
		void			debug(const codex_entry&, std::ostream&) const;
		void			debug(const entry_segment&, std::ostream&) const;

		private:

		//!Replaces every embed entry with its corresponding literals. Empties the parameter in the process.
		std::map<std::string, codex_entry>	compile_entries(std::map<std::string, codex_entry>&) const;

		//!Compacts consecutive literal entries into one.
		void			compact_entry(codex_entry&) const;
		//!Parsers all the tokens from a file.
		void			interpret_tokens(const std::vector<lexer::token>&, std::map<std::string, codex_entry>&) const;
		//!Starts the label phase: skip all whitespace until a label is reached, store the label. Returns false if there is no error and the tokens ended.
		std::string		label_phase(const std::vector<lexer::token>& _tokens, int&, const int _size) const;
		//!Parse the value contents until a "close value" is found.
		codex_entry 	value_phase(const std::vector<lexer::token>& _tokens, int& _curtoken, const int _size) const;
		//!Returns the index of the next token that maches type, skipping whitespace literals, from _curtoken. Any other type than whitespace literals will throw!.
		int				find_next_of(const std::vector<lexer::token>& _tokens, lexer::tokentypes _type, int _curtoken) const;
		//!Parses the inner component open+identifier+close from _curtoken. Returns the middle token once checked that it is a literal.
		std::string		parse_open_close(const std::vector<lexer::token>& _tokens, lexer::tokentypes _closetype, int _curtoken) const;
		//!Callback that will add a new entry to "entries".
		void			create_entry(const std::string&) const;
		//!Checks that every entry is solvable. Throws if it can't.
		void			check_integrity(const std::map<std::string, codex_entry>&) const;
		//!In compile mode, tries to replace all embed entries with their resulting static or variable segments. Returns true if the entry has no embeds.
		bool 			solve_entry(codex_entry& _entry, std::map<std::string, codex_entry>&) const;
	};

	delimiters								delimiter_set; //!< Current set of delimiters.
	std::string								file_path,	//<!File path where files are located.
											language;	//<!Language string, must be a subdirectory of the file_path.

	std::vector<substitution>				substitutions;	//<!Permanent substitutions.
	std::vector<path_structure>             paths;			//<!List of currently added paths.
	std::map<std::string, codex_entry>		codex;	//<!All data.
	codex_entry								fail_entry;

	//!Translates the fail string with the given key.
	std::string				fail_string(const std::string&) const;

	//!Reloads all entries.
	void					reload_codex();
	//!Internally adds a file, does not trigger recompilation.
	void					lexicalize_file(const path_structure&, std::map<std::string, std::vector<lexer::token>>&);
	//!Compiles the lexer tokens into the codex entries.
	void					build_entries(std::map<std::string, std::vector<lexer::token>>&);
	//!Creates the default error entry.
	void					create_default_error_entry();
};

}
