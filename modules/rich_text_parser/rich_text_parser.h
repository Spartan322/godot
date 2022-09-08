/*************************************************************************/
/*  rich_text_parser.h                                                    */
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

#ifndef RICH_TEXT_PARSER_H
#define RICH_TEXT_PARSER_H

#include "core/object/ref_counted.h"
#include "scene/gui/control.h"
#include "rich_text_item.h"

class RichTextParser : public RefCounted {
	GDCLASS(RichTextParser, RefCounted);

	String raw_text;

	String parsed_text;

	RichTextItem *root_item = nullptr;

protected:
	void set_processed_text(const String &p_text);

	void set_root_item(RichTextItem *p_item);

	static void _bind_methods();

public:
	void set_raw_text(const String &p_raw_text);
	String get_raw_text() const;

	String get_processed_text() const;

	RichTextItem *get_root_item() const;

	virtual void parse();
	virtual void clear();

	RichTextParser();
	~RichTextParser();
};

#endif // RICH_TEXT_PARSER_H