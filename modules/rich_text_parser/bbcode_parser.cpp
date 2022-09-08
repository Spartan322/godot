/*************************************************************************/
/*  bbcode_parser.cpp                                                  */
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

#include "bbcode_parser.h"

void BBCodeParser::register_tag(const StringName &p_tag, const Callable &p_callable, const StringName &p_class_name) {
	ERR_FAIL_COND_MSG(tag_constructors.has(p_tag), String(p_tag) + " is already registered for tag construction.");
	ERR_FAIL_COND_MSG(tag_converters.has(p_tag), String(p_tag) + " is already registered for tag conversion.");

	if (p_callable.is_null()) {
		return;
	}

	tag_constructors[p_tag] = Pair(p_class_name, p_callable);
}

void BBCodeParser::deregister_tag(const StringName &p_tag) {
	tag_constructors.erase(p_tag);
}

void BBCodeParser::register_tag_class(const StringName &p_tag, const StringName &p_tag_class) {
	ClassDB::ClassInfo *class_info = ClassDB::classes.getptr(p_tag_class);
	if (class_info) {
		register_tag(p_tag, callable_mp_static(class_info->creation_func));
	}
}

void BBCodeParser::register_tag_handler(const Callable &p_converter) {
	tag_handlers.push_back(p_converter);
}

void BBCodeParser::deregister_tag_handler(const Callable &p_callable) {
	tag_handlers.erase(p_callable);
}

void BBCodeParser::register_tag_conversion(const StringName &p_tag, const StringName &p_replacement) {
	ERR_FAIL_COND_MSG(tag_constructors.has(p_tag), String(p_tag) + " is already registered for tag construction.");
	ERR_FAIL_COND_MSG(tag_converters.has(p_tag), String(p_tag) + " is already registered for tag conversion.");

	tag_converters[p_tag] = p_replacement;
}

void BBCodeParser::deregister_tag_conversion(const StringName &p_tag) {
	tag_converters.erase(p_tag);
}

Variant BBCodeParser::create_from(const StringName &p_tag, const String &p_tag_data, const Dictionary &p_tag_options, int p_start_tag_start, int p_start_tag_end, BBCodeDrawCursor *p_cursor) const {
	const StringName *converted_tag = tag_converters.getptr(p_tag);
	if (converted_tag) {
		return *converted_tag;
	}

	const Variant *argptrs[7];
	Variant parserv = this;
	Variant tagv = p_tag;
	Variant datav = p_tag_data;
	Variant optionsv = p_tag_options;
	Variant tagstartv = p_start_tag_start;
	Variant tagendv = p_start_tag_end;
	Variant drawcursorv = p_cursor;
	argptrs[0] = &parserv;
	argptrs[1] = &tagv;
	argptrs[2] = &datav;
	argptrs[3] = &optionsv;
	argptrs[4] = &tagstartv;
	argptrs[5] = &tagendv; 
	argptrs[6] = &drawcursorv;
	Variant result;
	Callable::CallError ce;

	const Pair<StringName, Callable> *tag_ctor_info = tag_constructors.getptr(p_tag);
	if (tag_ctor_info) {
		tag_ctor_info->second.callp(argptrs, 1, result, ce);
		if (ce.error == Callable::CallError::CALL_OK && !result.is_null()) {
			return result;
		}
	}

	for (const Callable &handler : tag_handlers) {
		handler.callp(argptrs, 1, result, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			continue;
		}

		Variant *item = Object::cast_to<Variant>(result);
		if (item) {
			return *item;
		}
	}

	return Variant();
}

void BBCodeParser::_push_item(BBCodeItem *p_item) {
	ERR_FAIL_NULL(p_item);

	p_item->parent = current_item;
	current_item->children.append(p_item);
	current_item = p_item;

	emit_signal(SNAME("item_entered"), p_item);
}

void BBCodeParser::add_text(const String &p_text) {
	if (p_text.is_empty()) {
		return;
	}

	push_text();
	set_raw_text(get_raw_text() + p_text);
	pop();

	emit_signal(SNAME("text_processed"), current_item, p_text);
}

void BBCodeParser::push_text() {
	BBCodeText *item = memnew(BBCodeText);
	_push_item(item, false);
}

void BBCodeParser::add_image(const Ref<Texture2D> &p_image, const int p_width, const int p_height, const Color &p_color, InlineAlignment p_alignment) {
	BBCodeImage *item = memnew(BBCodeImage);
	item->image = p_image;
	item->size = Size2i(p_width, p_height);
	item->color = p_color;
	item-> inline_align = p_alignment;
	_push_item(item);
	pop();
}

void BBCodeParser::add_newline() {
	BBCodeNewline *item = memnew(BBCodeNewline);
	_push_item(item);
	pop();
}

bool BBCodeParser::remove_line(const int p_line) {
	emit_signal(SNAME("newline_removed"), p_line);
}

void BBCodeParser::push_dropcap(const String &p_string, const Ref<Font> &p_font, int p_size, const Rect2 &p_dropcap_margins, const Color &p_color, int p_ol_size, const Color &p_ol_color) {
	BBCodeDropcap *item = memnew(BBCodeDropcap);
	item->string = p_string;
	item->font = p_font;
	item->font_size = p_size;
	item->dropcap_margins = p_dropcap_margins;
	item->color = p_color;
	item->ol_size = p_ol_size;
	item->ol_color = p_ol_color;
	_push_item(item);
	pop();
}

void BBCodeParser::push_font(const Ref<Font> &p_font, int p_size) {
	BBCodeFont *item = memnew(BBCodeFont);
	item->font = p_font;
	item->font_size = p_size;
	_push_item(item);
}

void BBCodeParser::push_font_size(int p_font_size) {
	BBCodeFontSize *item = memnew(BBCodeFontSize);
	item->font_size = p_font_size;
	_push_item(item);
}

void BBCodeParser::push_outline_size(int p_font_size) {

}

void BBCodeParser::push_normal() {

}

void BBCodeParser::push_bold() {

}

void BBCodeParser::push_bold_italics() {

}

void BBCodeParser::push_italics() {

}

void BBCodeParser::push_mono() {

}

void BBCodeParser::push_color(const Color &p_color) {

}

void BBCodeParser::push_outline_color(const Color &p_color) {

}

void BBCodeParser::push_underline() {

}

void BBCodeParser::push_strikethrough() {

}

void BBCodeParser::push_paragraph(HorizontalAlignment p_alignment, Control::TextDirection p_direction, const String &p_language, TextServer::StructuredTextParser p_st_parser) {

}

void BBCodeParser::push_indent(int p_level) {

}

void BBCodeParser::push_list(int p_level, ListType p_list, bool p_capitalize) {

}

void BBCodeParser::push_meta(const Variant &p_meta) {

}

void BBCodeParser::push_hint(const String &p_string) {

}

void BBCodeParser::push_table(int p_columns, InlineAlignment p_alignment) {

}

void BBCodeParser::push_fade(int p_start_index, int p_length) {

}

void BBCodeParser::push_shake(int p_strength, float p_rate) {

}

void BBCodeParser::push_wave(float p_frequency, float p_amplitude) {

}

void BBCodeParser::push_tornado(float p_frequency, float p_radius) {

}

void BBCodeParser::push_rainbow(float p_saturation, float p_value, float p_frequency) {

}

void BBCodeParser::push_bgcolor(const Color &p_color) {

}

void BBCodeParser::push_fgcolor(const Color &p_color) {

}

// void BBCodeParser::push_customfx(Ref<RichTextEffect> p_custom_effect, Dictionary p_environment);
void BBCodeParser::set_table_column_expand(int p_column, bool p_expand, int p_ratio) {

}

void BBCodeParser::set_cell_row_background_color(const Color &p_odd_row_bg, const Color &p_even_row_bg) {

}

void BBCodeParser::set_cell_border_color(const Color &p_color) {

}

void BBCodeParser::set_cell_size_override(const Size2 &p_min_size, const Size2 &p_max_size) {

}

void BBCodeParser::set_cell_padding(const Rect2 &p_padding) {

}

int BBCodeParser::get_current_table_column() const {

}

void BBCodeParser::push_cell() {

}

void BBCodeParser::pop() {
	ERR_FAIL_COND(!current_item);

	emit_signal(SNAME("item_exited"), current_item);
	BBCodeItem *child = current_item;
	current_item = current_item->parent;
}

bool BBCodeParser::pop_expected(const StringName &p_tag) {
	ERR_FAIL_COND_V(!current_item, false);

	if (current_item->get_class_name() != p_tag) {
		return false;
	}

	pop();
	return true;
}

void BBCodeParser::parse_bbcode(const String &p_bbcode) {
	clear();
	append_text(p_bbcode);
}

void BBCodeParser::append_text(const String &p_bbcode) {
	int raw_end = get_raw_text().length();
	set_raw_text(get_raw_text() + p_bbcode);
	parse_from(raw_end);
}

void BBCodeParser::parse() {
	parse_from(0);
}

char32_t BBCodeParser::parse_get(int p_index) const {
	if (p_index == -1) {
		p_index = parsing_current;
	}

	ERR_FAIL_COND_V(p_index > parsing_end, '\0');

	if (p_index == -1) {
		return '\0';
	}

	return get_raw_text()[p_index];
}

int BBCodeParser::parse_get_current_index() const {
	if (parsing_current == -1) {
		return parsing_end;
	}

	return parsing_current;
}

bool BBCodeParser::parse_ended() const {
	return parsing_current == -1;
}

bool BBCodeParser::parse_move() {
	ERR_FAIL_COND_V(parsing_current > parsing_end, false);

	if (parsing_current == parsing_end) {
		parsing_current = -1;
	}

	if (parsing_current == -1) {
		return false;
	}

	parsing_current++;
	return true;
}

bool BBCodeParser::parse_is(std::initializer_list<char32_t> p_chars, int p_index) const {
	for (char32_t c : p_chars) {
		if (parse_get(p_index) == c) {
			return true;
		}
	}

	return false;
}

bool BBCodeParser::parse_is(char32_t p_char, int p_index) const {
	return parse_is({ p_char }, p_index);
}

bool BBCodeParser::parse_is_whitespace(int p_index) const {
	return is_whitespace(parse_get(p_index));
}

bool BBCodeParser::parse_move_check(std::initializer_list<char32_t> p_chars) {
	bool check = parse_is(p_chars);
	parse_move();
	return check;
}

bool BBCodeParser::parse_move_check(char32_t p_char) {
	return parse_move_check({ p_char });
}

char32_t BBCodeParser::parse_expect(std::initializer_list<char32_t> p_chars) {
	if (parse_is(p_chars)) {
		return get_raw_text()[parsing_current];
	}

	return '\0';
}

char32_t BBCodeParser::parse_expect(char32_t p_char) {
	return parse_expect({ p_char });
}

char32_t BBCodeParser::parse_expect_not(std::initializer_list<char32_t> p_chars) {
	if (!parse_is(p_chars)) {
		return get_raw_text()[parsing_current];
	}

	return '\0';
}

char32_t BBCodeParser::parse_expect_not(char32_t p_char) {
	return parse_expect_not({ p_char });
}

char32_t BBCodeParser::parse_consume(std::initializer_list<char32_t> p_chars) {
	char32_t current = parse_expect(p_chars);
	parse_move();
	return current;
}

char32_t BBCodeParser::parse_consume(char32_t p_char) {
	return parse_consume({ p_char });
}

bool BBCodeParser::parse_skip_tag_name() {
	while (is_ascii_char(parse_get())) {
		if (!parse_move()) {
			return false;
		}
	}
	return true;
}

bool BBCodeParser::parse_skip(std::initializer_list<char32_t> p_chars) {
	while (parse_is(p_chars)) {
		if (!parse_move()) {
			return false;
		}
	}
	return true;
}

bool BBCodeParser::parse_skip(char32_t p_char) {
	return parse_skip({ p_char });
}

bool BBCodeParser::parse_skip_whitespace_and(std::initializer_list<char32_t> p_chars) {
	while (parse_is(p_chars) || parse_is_whitespace()) {
		if (!parse_move()) {
			return false;
		}
	}
	return true;
}

bool BBCodeParser::parse_skip_whitespace_and(char32_t p_char) {
	return parse_skip_whitespace_and({ p_char });
}

bool BBCodeParser::parse_skip_to(std::initializer_list<char32_t> p_chars) {
	while (!parse_is(p_chars)) {
		if (!parse_move()) {
			return false;
		}
	}
	return true;
}

bool BBCodeParser::parse_skip_to(char32_t p_char) {
	return parse_skip_to({ p_char });
}

bool BBCodeParser::parse_skip_to_whitespace_and(std::initializer_list<char32_t> p_chars) {
	while (!(parse_is(p_chars) || parse_is_whitespace())) {
		if (!parse_move()) {
			return false;
		}
	}
	return true;
}

bool BBCodeParser::parse_skip_to_whitespace_and(char32_t p_char) {
	return parse_skip_whitespace_and({ p_char });
}

bool BBCodeParser::parse_skip_whitespace() {
	while (is_whitespace(parse_get())) {
		if (!parse_move()) {
			return false;
		}
	}
	return true;
}

bool BBCodeParser::parse_skip_to_whitespace() {
	while (!is_whitespace(parse_get())) {
		if (!parse_move()) {
			return false;
		}
	}
	return true;
}

bool BBCodeParser::parse_skip_to_string(const String &p_skip_to) {
	ERR_FAIL_COND_V(p_skip_to.length() == 0, false);

	while (true) {
		if (!parse_skip_to(p_skip_to[0])) {
			return false;
		}

		bool match_found = true;
		for (int i = 1; i < p_skip_to.length(); i++, parse_move()) {
			if (!parse_is(p_skip_to[i])) {
				match_found = false;
				break;
			}
		}

		if (match_found) {
			break;
		}
	}
	return true;
}

ItemType BBCodeParser::_get_type_from_tag_name(const String &p_tag) const {
	if (p_tag == "text") {
		return ITEM_TEXT;
	}
	if (p_tag == "b") {
		return ITEM_BOLD;
	}
	if (p_tag == "i") {
		return ITEM_ITALICS;
	}
	if (p_tag == "u") {
		return ITEM_UNDERLINE;
	}
	if (p_tag == "s") {
		return ITEM_STRIKETHROUGH;
	}
	if (p_tag == "code") {
		return ITEM_MONO;
	}
	if (p_tag == "p" || p_tag == "center" || p_tag == "left" || p_tag == "right" || p_tag == "fill") {
		return ITEM_PARAGRAPH;
	}
	if (p_tag == "indent") {
		return ITEM_INDENT;
	}
	if (p_tag == "url") {
		return ITEM_META;
	}
	if (p_tag == "img") {
		return ITEM_IMAGE;
	}
	if (p_tag == "font" || p_tag == "opentype_features") {
		return ITEM_FONT;
	}
	if (p_tag == "font_size") {
		return ITEM_FONT_SIZE;
	}
	if (p_tag == "color") {
		return ITEM_COLOR;
	}
	if (p_tag == "bgcolor") {
		return ITEM_BG_COLOR;
	}
	if (p_tag == "fgcolor") {
		return ITEM_FG_COLOR;
	}
	if (p_tag == "outline_size") {
		return ITEM_OUTLINE_SIZE;
	}
	if (p_tag == "outline_color") {
		return ITEM_OUTLINE_COLOR;
	}
	if (p_tag == "table" || p_tag == "cell") {
		return ITEM_TABLE;
	}
	if (p_tag == "ul" || p_tag == "ol") {
		return ITEM_LIST;
	}
	if (p_tag == "dropcap") {
		return ITEM_DROPCAP;
	}
	if (p_tag == "hint") {
		return ITEM_HINT;
	}
	if (p_tag == "fade") {
		return ITEM_FADE;
	}
	if (p_tag == "shake") {
		return ITEM_SHAKE;
	}
	if (p_tag == "wave") {
		return ITEM_WAVE;
	}
	if (p_tag == "tornado") {
		return ITEM_TORNADO;
	}
	if (p_tag == "rainbow") {
		return ITEM_RAINBOW;
	}
	return ITEM_TEXT;
	if (p_tag == "lb" || p_tag == "rb") {
		return ITEM_TEXT;
	}
	if (p_tag == "lrm") {
		
	}
	if (p_tag == "rlm") {
		
	}
	if (p_tag == "lre") {
		
	}
	if (p_tag == "rle") {
		
	}
	if (p_tag == "lro") {
		
	}
	if (p_tag == "rlo") {
		
	}
	if (p_tag == "pdf") {
		
	}
	if (p_tag == "alm") {
		
	}
	if (p_tag == "lri") {
		
	}
	if (p_tag == "rli") {
		
	}
	if (p_tag == "fsi") {
		
	}
	if (p_tag == "pdi") {
		
	}
	if (p_tag == "zwj") {
		
	}
	if (p_tag == "zwnj") {
		
	}
	if (p_tag == "wj") {
		
	}
	if (p_tag == "shy") {
		
	}
}

void BBCodeParser::_push_tag(const ItemType p_type, const String &p_tag, const String &p_data, const HashMap<String, String> &p_options) {
	switch(p_type) {
		case ITEM_TEXT:
			push_text();
		case ITEM_IMAGE:
			
		case ITEM_NEWLINE:
			add_newline();
		case ITEM_DROPCAP:

		case ITEM_FONT:
		case ITEM_FONT_SIZE:
		case ITEM_OUTLINE_SIZE:
		case ITEM_NORMAL:
		case ITEM_BOLD:
		case ITEM_BOLD_ITALICS:
		case ITEM_ITALICS:
		case ITEM_MONO:
		case ITEM_COLOR:
		case ITEM_OUTLINE_COLOR:
		case ITEM_UNDERLINE:
		case ITEM_STRIKETHROUGH:
		case ITEM_PARAGRAPH:
		case ITEM_INDENT:
		case ITEM_LIST:
		case ITEM_META:
		case ITEM_HINT:
		case ITEM_TABLE:
		case ITEM_FADE:
		case ITEM_SHAKE:
		case ITEM_WAVE:
		case ITEM_TORNADO:
		case ITEM_RAINBOW:
		case ITEM_BG_COLOR:
		case ITEM_FG_COLOR:
		case ITEM_CUSTOM:
		case ITEM_NONE:
			break;
	}
}

void BBCodeParser::parse_from(int p_start, int p_stop) {
	ERR_FAIL_COND(p_start < 0);

	if (p_stop < 0) {
		p_stop += get_raw_text().length();
	}

	ERR_FAIL_COND(p_stop >= get_raw_text().length());

	const String text = get_raw_text();

	parsing_current = p_start;
	parsing_end = p_stop;

	int last_tag_end = 0;

	int font_size = get_font_size();
	Ref<Font> normal_font = get_normal_font();
	Ref<Font> bold_font = get_bold_font();
	Ref<Font> italics_font = get_italics_font();
	Ref<Font> bold_italics_font = get_bold_italics_font();
	Ref<Font> mono_font = get_mono_font();
	Color base_color = get_default_color();

	BBCodeDrawCursor *draw_cursor = memnew(BBCodeDrawCursor);
	draw_cursor->font = normal_font;
	draw_cursor->font_size = font_size;
	draw_cursor->color = base_color;

	while (parse_move()) {
		if (!parse_is('[')) {
			continue;
		}

		if (parse_get_current_index() - last_tag_end > 0) {
			_set_parsed_text(get_parsed_text() + text.substr(last_tag_end, parse_get_current_index() - last_tag_end));
			last_tag_end = parse_get_current_index();
		}

		int tag_start = parse_get_current_index();

		// Pass the [ character
		if (!parse_move()) {
			break;
		}

		bool is_end = parse_is('/');

		if (is_end) {
			if (!parse_move()) {
				// Tag parsing left scope, end parse
				break;
			}
		}

		int tag_name_start = parse_get_current_index();

		if (!parse_skip_tag_name()) {
			// Tag name left parsing scope, end parse
			break;
		}

		if (is_end) {
			// Tag end found to be invalid, ignore
			if(!parse_is(']')) {
				continue;
			}
		}

		const String tag_name = text.substr(tag_name_start, parse_get_current_index() - tag_name_start);

		parse_move();

		if (!is_end) {
			String tag_data;
			if (parse_is('=')) {
				if (!parse_move()) {
					break;
				}

				int tag_data_start = parse_get_current_index();
				if (parse_is('{')) {
					// Iterate to the top depth }
					int json_depth = 1;
					while(json_depth > 0 && parse_move()) {
						if (parse_is('}')) {
							json_depth--;
						}
						if (parse_is('{')) {
							json_depth++;
						}
					}

					if (parse_ended()) {
						WARN_PRINT(vformat("BBCode tag %s data parsing has left scope.", tag_name));
					}
				} else {
					parse_skip_to_whitespace();
				}

				// Tag data left parsing scope, end parse
				if (parse_ended()) {
					break;
				}

				tag_data = text.substr(tag_data_start, parse_get_current_index() - tag_data_start);
			}

			// Tag parsing unfinished in scope, end parse
			if (!parse_skip_whitespace()) {
				break;
			}

			Dictionary tag_options;
			if (!parse_is(']')) {
				while (!parse_move()) {
					// Tag parsing unfinished in scope or tag has reached end, finalize option parse
					if (!parse_skip_whitespace() || parse_is(']')) {
						break;
					}

					int tag_option_start = parse_get_current_index();

					if (!parse_skip_to_whitespace_and('=')) {
						break;
					}

					String tag_option_name = text.substr(tag_option_start, parse_get_current_index() - tag_option_start);

					if (!parse_is('=')) {
						tag_options[tag_option_name] = String();
						break;
					}

					parse_move();

					tag_option_start = parse_get_current_index();

					if (!parse_skip_to_whitespace()) {
						break;
					}

					String tag_option_data = text.substr(tag_option_start, parse_get_current_index() - tag_option_start);

					tag_options[tag_option_name] = tag_option_data;
				}

				if (parse_ended()) {
					break;
				}
			}

			Variant tag_result;
			if (parse_is(']')) {
				bool can_create_tag = true;
				int bbcode_depth = 0;
				for (int check_index = parse_get_current_index() + 1; check_index < parsing_end; check_index++) {
					char32_t check = parse_get(check_index);
					if (check == '[') {
						if (parse_get(check_index + 1) == '/') {
							bbcode_depth--;
						} else {
							bbcode_depth++;
						}
						if (bbcode_depth < -1) {
							can_create_tag = false;
							break;
						}
						if (bbcode_depth == -1) {
							if (parsing_end > check_index + 2 + tag_name.length()) {
								can_create_tag = text.substr(check_index + 2, tag_name.length()) == tag_name;
							}
							break;
						}
					}
				}

				if (can_create_tag) {
					tag_result = create_from(tag_name, tag_data, tag_options, tag_start, parse_get_current_index(), draw_cursor);
					switch (tag_result.get_type()) {
						case Variant::Type::STRING: {
							const String &tag_string = tag_result;
							_set_parsed_text(get_parsed_text() + tag_string);
						}
						break;
						case Variant::Type::STRING_NAME: {
							const StringName &tag_string = tag_result;
							_set_parsed_text(get_parsed_text() + tag_string);
						}
						break;
						case Variant::Type::OBJECT: {
							Object *tag_object = tag_result;
							BBCodeItem *tag_item = Object::cast_to<BBCodeItem>(tag_object);
							if (!tag_item) {
								tag_result = Variant();
								break;
							}

							tag_item->tag_name = tag_name;
							tag_item->tag_data = tag_data;
							tag_item->tag_options = tag_options;
							tag_item->start_tag_start_index = tag_start;
							tag_item->start_tag_close_index = parse_get_current_index();
							tag_item->parser = this;
							tag_item->last_draw_cursor = draw_cursor;

							draw_cursor = tag_item->current_draw_cursor;

							_push_item(tag_item);
						}
						break;
						default:
							break;
					}
				}
			}

			// No valid tag found
			if (tag_result.is_null()) {
				last_tag_end = tag_start;
				continue;
			}
		} else {
			if (current_item->get_tag_name() == tag_name) {
				current_item->end_tag_start_index = tag_start;
				current_item->end_tag_end_index = parse_get_current_index();
				draw_cursor = current_item->last_draw_cursor;
				pop();
			} else {
				last_tag_end = tag_start;
				continue;
			}

			// Tag mismatch was found
			// BBCode does not allow mistmatched tags, if a closing tag is found that mismatches the last open tag it should not be parsed
			// if (tag_name != current_item->get_tag_string()) {
			// 	last_tag_end = tag_start;
			// 	continue;
			// }
		}

		parse_move();

		// Tag syntax was successfully parsed, don't display for parsed string
		last_tag_end = parse_get_current_index();
#if 0
		if (tag_name == "text") {
			if (!is_end) {
				push_text();
			} else {
				pop_expected(ITEM_TEXT);
			}
		} else if (tag_name == "b") {
			if (!is_end) {
				push_bold();
			} else {
				pop_expected(ITEM_BOLD);
			}
		} else if (tag_name == "i") {
			if (!is_end) {
				push_italics();
			} else {
				pop_expected(ITEM_ITALICS);
			}
			
		} else if (tag_name == "u") {
			
		} else if (tag_name == "s") {
			
		} else if (tag_name == "code") {
			
		} else if (tag_name == "p") {
			
		} else if (tag_name == "center") {
			
		} else if (tag_name == "left") {
			
		} else if (tag_name == "right") {
			
		} else if (tag_name == "fill") {
			
		} else if (tag_name == "indent") {
			
		} else if (tag_name == "url") {
			
		} else if (tag_name == "img") {
			
		} else if (tag_name == "font") {
			
		} else if (tag_name == "font_size") {
			
		} else if (tag_name == "opentype_features") {
			
		} else if (tag_name == "color") {
			
		} else if (tag_name == "bgcolor") {
			
		} else if (tag_name == "fgcolor") {
			
		} else if (tag_name == "outline_size") {
			
		} else if (tag_name == "outline_color") {
			
		} else if (tag_name == "table") {
			
		} else if (tag_name == "cell") {
			
		} else if (tag_name == "ul") {
			
		} else if (tag_name == "ol") {
			
		} else if (tag_name == "lb") {
			
		} else if (tag_name == "rb") {
			
		} else if (tag_name == "dropcap") {
			
		} else if (tag_name == "hint") {
			
		} else if (tag_name == "fade") {
			
		} else if (tag_name == "shake") {
			
		} else if (tag_name == "wave") {
			
		} else if (tag_name == "tornado") {
			
		} else if (tag_name == "rainbow") {
			
		} else if (tag_name == "lrm") {
			
		} else if (tag_name == "rlm") {
			
		} else if (tag_name == "lre") {
			
		} else if (tag_name == "rle") {
			
		} else if (tag_name == "lro") {
			
		} else if (tag_name == "rlo") {
			
		} else if (tag_name == "pdf") {
			
		} else if (tag_name == "alm") {
			
		} else if (tag_name == "lri") {
			
		} else if (tag_name == "rli") {
			
		} else if (tag_name == "fsi") {
			
		} else if (tag_name == "pdi") {
			
		} else if (tag_name == "zwj") {
			
		} else if (tag_name == "zwnj") {
			
		} else if (tag_name == "wj") {
			
		} else if (tag_name == "shy") {
			
		}
#endif
	}

	// Add the non-parsed end of the text
	_set_parsed_text(get_parsed_text() + text.substr(last_tag_end, last_tag_end - parsing_end));
}

void BBCodeParser::clear() {
	RichTextParser::clear();
}

void BBCodeParser::load_default_tag_parsing() {
	register_tag_conversion("lb", "[");
	register_tag_conversion("rb", "]");
	register_tag_conversion("lrm", String::chr(0x200E));
	register_tag_conversion("rlm", String::chr(0x200F));
	register_tag_conversion("lre", String::chr(0x202A));
	register_tag_conversion("rle", String::chr(0x202B));
	register_tag_conversion("lro", String::chr(0x202D));
	register_tag_conversion("rlo", String::chr(0x202E));
	register_tag_conversion("pdf", String::chr(0x202C));
	register_tag_conversion("alm", String::chr(0x061c));
	register_tag_conversion("lri", String::chr(0x2066));
	register_tag_conversion("rli", String::chr(0x2027));
	register_tag_conversion("fsi", String::chr(0x2068));
	register_tag_conversion("pdi", String::chr(0x2069));
	register_tag_conversion("zwj", String::chr(0x200D));
	register_tag_conversion("zwnj", String::chr(0x200C));
	register_tag_conversion("wj", String::chr(0x2060));
	register_tag_conversion("shy", String::chr(0x00AD));

	register_item<BBCodeImage>("img");
	register_item<BBCodeDropcap>("dropcap");
	register_item<BBCodeList>("ol");
	register_item<BBCodeList>("ul");
	register_item<BBCodeTable>("table");
	register_item<BBCodeIndent>("indent");
	register_item<BBCodeFont>("font");
	register_item<BBCodeFontSize>("size");

}

void BBCodeParser::clear_tag_parsing() {
	tag_converters.clear();
	tag_constructors.clear();
	tag_handlers.clear();
}

void BBCodeParser::_bind_methods() {
	ClassDB::bind_method(D_METHOD("register_tag", "tag", "constructor"), &BBCodeParser::register_tag);
	ClassDB::bind_method(D_METHOD("deregister_tag", "tag"), &BBCodeParser::deregister_tag);

	ClassDB::bind_method(D_METHOD("register_tag_class", "tag", "class_name"), &BBCodeParser::register_tag_class);

	ClassDB::bind_method(D_METHOD("register_tag_handler", "handler"), &BBCodeParser::register_tag_handler);
	ClassDB::bind_method(D_METHOD("deregister_tag_handler", "handler"), &BBCodeParser::deregister_tag_handler);

	ClassDB::bind_method(D_METHOD("create_from", "tag"), &BBCodeParser::create_from);

	ADD_SIGNAL(MethodInfo("newline_removed", PropertyInfo(Variant::INT, "position")));
}