/*************************************************************************/
/*  bbcode_parser.h                                                    */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef BBCODE_PARSER_H
#define BBCODE_PARSER_H

#include "rich_text_parser.h"
#include "bbcode_item.h"

#include <initializer_list>

class BBCodeParser : public RichTextParser {
	GDCLASS(BBCodeParser, RichTextParser);

public:
	enum ItemType {
		ITEM_NONE = -1,
		ITEM_TEXT,
		ITEM_IMAGE,
		ITEM_NEWLINE,
		ITEM_DROPCAP,
		ITEM_FONT,
		ITEM_FONT_SIZE,
		ITEM_OUTLINE_SIZE,
		ITEM_NORMAL,
		ITEM_BOLD,
		ITEM_BOLD_ITALICS,
		ITEM_ITALICS,
		ITEM_MONO,
		ITEM_COLOR,
		ITEM_OUTLINE_COLOR,
		ITEM_UNDERLINE,
		ITEM_STRIKETHROUGH,
		ITEM_PARAGRAPH,
		ITEM_INDENT,
		ITEM_LIST,
		ITEM_META,
		ITEM_HINT,
		ITEM_TABLE,
		ITEM_FADE,
		ITEM_SHAKE,
		ITEM_WAVE,
		ITEM_TORNADO,
		ITEM_RAINBOW,
		ITEM_BG_COLOR,
		ITEM_FG_COLOR,
		ITEM_CUSTOM
	};

	enum ListType {
		LIST_NUMBERS,
		LIST_LETTERS,
		LIST_ROMAN,
		LIST_DOTS
	};

private:
	HashMap<StringName, Pair<StringName, Callable>> tag_constructors;
	List<const Callable> tag_handlers;
	HashMap<StringName, StringName> tag_converters;

	template<class ItemT>
	void register_item(const StringName &p_tag);

	int parsing_current;
	int parsing_end;

	char32_t parse_get(int p_index = -1) const;
	int parse_get_current_index() const;
	bool parse_ended() const;
	bool parse_move();
	bool parse_is(std::initializer_list<char32_t> p_chars, int p_index = -1) const;
	bool parse_move_check(std::initializer_list<char32_t> p_chars);
	char32_t parse_expect(std::initializer_list<char32_t> p_chars);
	char32_t parse_expect_not(std::initializer_list<char32_t> p_chars);
	char32_t parse_consume(std::initializer_list<char32_t> p_chars);
	bool parse_skip_tag_name();
	bool parse_skip(std::initializer_list<char32_t> p_chars);
	bool parse_skip_whitespace_and(std::initializer_list<char32_t> p_chars);
	bool parse_skip_to(std::initializer_list<char32_t> p_chars);
	bool parse_skip_to_whitespace_and(std::initializer_list<char32_t> p_chars);
	bool parse_is(char32_t p_char, int p_index = -1) const;
	bool parse_is_whitespace(int p_index = -1) const;
	bool parse_move_check(char32_t p_char);
	char32_t parse_expect(char32_t p_char);
	char32_t parse_expect_not(char32_t p_char);
	char32_t parse_consume(char32_t p_char);
	bool parse_skip(char32_t p_char);
	bool parse_skip_to(char32_t p_char);
	bool parse_skip_whitespace();
	bool parse_skip_whitespace_and(char32_t p_char);
	bool parse_skip_to_whitespace();
	bool parse_skip_to_whitespace_and(char32_t p_char);
	bool parse_skip_to_string(const String &p_skip_to);

	BBCodeItem *current_item;

	void _push_item(BBCodeItem *p_item);
	ItemType _get_type_from_tag_name(const String &p_tag) const;
	void _push_tag(const ItemType p_type, const String &p_tag, const String &p_data, const HashMap<String, String> &p_options);

protected:
	void parse_from(int p_start, int p_stop = -1);

	static void _bind_methods();

public:
	void register_tag(const StringName &p_tag, const Callable &p_callable, const StringName &p_class_name = "");
	void deregister_tag(const StringName &p_tag);

	void register_tag_class(const StringName &p_tag, const StringName &p_tag_class);

	template <class ItemT>
	void register_tag_class_static(const StringName &p_tag) {
		register_tag_class(p_tag, ItemT::get_class_static());
	}

	void register_tag_handler(const Callable &p_callable);
	void deregister_tag_handler(const Callable &p_callable);

	void register_tag_conversion(const StringName &p_tag, const StringName &p_replacement);
	void deregister_tag_conversion(const StringName &p_tag);

	void register_default_tags();
	void deregister_all_tags();

	bool is_tag_registered(const StringName &p_tag_name) const;

	Variant create_tag_from(const StringName &p_tag, const String &p_tag_data, const Dictionary &p_tag_options, int p_start_tag_start, int p_start_tag_end) const;

	void pop();
	bool pop_expected(const StringName &p_tag);

	void push_item(BBCodeItem *p_item);
	BBCodeItem *get_current_item() const;

	void parse_bbcode(const String &p_bbcode);
	void append_text(const String &p_bbcode);

	virtual void clear() override;
	virtual void parse() override;

	void load_default_tag_parsing();
	void clear_tag_parsing();

	BBCodeParser();
	~BBCodeParser();
};


VARIANT_ENUM_CAST(BBCodeParser::ListType);

#endif // BBCODE_PARSER_H