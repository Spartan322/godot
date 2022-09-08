/*************************************************************************/
/*  bbcode_item.h                                                        */
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

#ifndef BBCODE_ITEM_H
#define BBCODE_ITEM_H

#include "rich_text_item.h"
#include "scene/resources/texture.h"
#include "scene/resources/font.h"
#include "core/templates/vector.h"
#include "scene/gui/control.h"

struct BBCodeDrawCursor {
	Color color;
	Ref<Font> font;
	int font_size;
	Color outline_color;
	int outline_size;
	Vector2 offset;
	HorizontalAlignment horizontal_alignment;
	VerticalAlignment vertical_alignment;
	InlineAlignment inline_alignment;
	Control::TextDirection text_direction;
	TextServer::StructuredTextParser structured_parser;
	Color background_color;
	Color foreground_color;
	Dictionary metadata;
};

class BBCodeItem : public RichTextItem {
	GDCLASS(BBCodeItem, RichTextItem);

	friend class BBCodeParser;

	class BBCodeParser *parser;

	BBCodeItem *parent;
	Vector<BBCodeItem *> children;

	BBCodeDrawCursor *last_draw_cursor;
	BBCodeDrawCursor *current_draw_cursor;

	StringName tag_name;
	String tag_data;
	Dictionary tag_options;

	int start_tag_start_index;
	int start_tag_close_index;

	int end_tag_start_index;
	int end_tag_end_index;

	int tag_content_start;
	int tag_content_end;

	String content_text_cache;

protected:
	bool _call_is_self_contained() const;

	bool is_self_contained() const;

	GDVIRTUAL0RC(bool, _is_self_contained);

	static void _bind_methods();

public:
	BBCodeParser *get_parser() const;

	BBCodeItem *get_parent() const;

	BBCodeItem *get_child(int p_index) const;
	int get_child_count() const;

	const StringName get_tag_name() const;
	const String get_tag_data() const;
	const Dictionary get_tag_options() const;

	int get_tag_content_start() const;
	int get_tag_content_end() const;
	
	int get_start_tag_start_index() const;
	int get_start_tag_close_index() const;

	int get_end_tag_start_index() const;
	int get_end_tag_close_index() const;

	String get_content_text() const;
	void set_content_text(const String &p_text);

	void insert_parsed_text(int p_index, const String &p_text);

	void push_color(const Color p_color);
	void push_font(const Ref<Font> p_font);
	void push_font_size(int p_font_size);
	void push_outline_color(const Color p_color);
	void push_outline_size(int p_size);
	void push_offset(const Vector2 p_offset);
	void push_horizontal_alignment(HorizontalAlignment p_alignment);
	void push_vertical_alignment(VerticalAlignment p_alignment);
	void push_inline_alignment(InlineAlignment p_alignment);
	void push_structured_parser(TextServer::StructuredTextParser p_parser);
	void push_background_color(const Color p_color);
	void push_foreground_color(const Color p_color);
	void push_metadata(const StringName &p_key, const Variant p_value);

	Variant get_metadata(const StringName &p_key) const;
	bool has_metadata(const StringName &p_key) const;

	virtual void _draw();

	BBCodeItem();
	~BBCodeItem();
};

class BBCodeText : public BBCodeItem {
	GDCLASS(BBCodeText, BBCodeItem);

public:

};

class BBCodeImage : public BBCodeItem {
	GDCLASS(BBCodeImage, BBCodeItem);

public:
	Ref<Texture2D> image;
	InlineAlignment inline_align = INLINE_ALIGNMENT_CENTER;
	Size2 size;
	Color color;
};

class BBCodeNewline : public BBCodeItem {
	GDCLASS(BBCodeNewline, BBCodeItem);

public:

};

class BBCodeDropcap : public BBCodeItem {
	GDCLASS(BBCodeDropcap, BBCodeItem);

public:
	String string;
	Ref<Font> font;
	int font_size = 0;
	Color color;
	int ol_size = 0;
	Color ol_color;
	Rect2 dropcap_margins; 

};

class BBCodeFont : public BBCodeItem {
	GDCLASS(BBCodeFont, BBCodeItem);

public:
	Ref<Font> font;
	int font_size = 0;

};

class BBCodeFontSize : public BBCodeItem {
	GDCLASS(BBCodeFontSize, BBCodeItem);

public:
	int font_size = 16;

};

class BBCodeOutlineSize : public BBCodeItem {
	GDCLASS(BBCodeOutlineSize, BBCodeItem);

public:

};

class BBCodeNormal : public BBCodeItem {
	GDCLASS(BBCodeNormal, BBCodeItem);

public:

};

class BBCodeBold : public BBCodeItem {
	GDCLASS(BBCodeBold, BBCodeItem);

public:

};

class BBCodeBoldItalics : public BBCodeItem {
	GDCLASS(BBCodeBoldItalics, BBCodeItem);

public:

};

class BBCodeItalics : public BBCodeItem {
	GDCLASS(BBCodeItalics, BBCodeItem);

public:

};

class BBCodeMono : public BBCodeItem {
	GDCLASS(BBCodeMono, BBCodeItem);

public:

};

class BBCodeColor : public BBCodeItem {
	GDCLASS(BBCodeColor, BBCodeItem);

public:

};

class BBCodeOutlineColor : public BBCodeItem {
	GDCLASS(BBCodeOutlineColor, BBCodeItem);

public:

};

class BBCodeUnderline : public BBCodeItem {
	GDCLASS(BBCodeUnderline, BBCodeItem);

public:

};

class BBCodeStrikethrough : public BBCodeItem {
	GDCLASS(BBCodeStrikethrough, BBCodeItem);

public:

};

class BBCodeParagraph : public BBCodeItem {
	GDCLASS(BBCodeParagraph, BBCodeItem);

public:

};

class BBCodeIndent : public BBCodeItem {
	GDCLASS(BBCodeIndent, BBCodeItem);

public:

};

class BBCodeList : public BBCodeItem {
	GDCLASS(BBCodeList, BBCodeItem);

public:

};

class BBCodeMeta : public BBCodeItem {
	GDCLASS(BBCodeMeta, BBCodeItem);

public:

};

class BBCodeHint : public BBCodeItem {
	GDCLASS(BBCodeHint, BBCodeItem);

public:

};

class BBCodeTable : public BBCodeItem {
	GDCLASS(BBCodeTable, BBCodeItem);

public:

};

class BBCodeFade : public BBCodeItem {
	GDCLASS(BBCodeFade, BBCodeItem);

public:

};

class BBCodeShake : public BBCodeItem {
	GDCLASS(BBCodeShake, BBCodeItem);

public:

};

class BBCodeWave : public BBCodeItem {
	GDCLASS(BBCodeWave, BBCodeItem);

public:

};

class BBCodeTornado : public BBCodeItem {
	GDCLASS(BBCodeTornado, BBCodeItem);

public:

};

class BBCodeRainbow : public BBCodeItem {
	GDCLASS(BBCodeRainbow, BBCodeItem);

public:

};

class BBCodeBgColor : public BBCodeItem {
	GDCLASS(BBCodeBgColor, BBCodeItem);

public:

};

class BBCodeFgColor : public BBCodeItem {
	GDCLASS(BBCodeFgColor, BBCodeItem);

public:

};

#endif // BBCODE_ITEM_H
