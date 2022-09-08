/*************************************************************************/
/*  rich_text_parser.cpp                                                  */
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

#include "rich_text_parser.h"

void RichTextParser::_set_width(int p_width) {
    width = p_width;
}

void RichTextParser::_set_height(int p_height) {
    height = p_height;
}

void RichTextParser::_set_root_item(RichTextItem *p_item) {
    root_item = p_item;
}

void RichTextParser::_set_parsed_text(const String &p_parsed_text) {
    parsed_text = p_parsed_text;
}

int RichTextParser::get_width() const {
    return width;
}

int RichTextParser::get_height() const {
    return height;
}

String RichTextParser::get_parsed_text() const {
    return parsed_text;
}

void RichTextParser::set_raw_text(const String &p_raw_text) {
    raw_text = p_raw_text;
}

String RichTextParser::get_raw_text() const {
    return raw_text;
}

RichTextItem *RichTextParser::get_root_item() const {
    return root_item;
}

void RichTextParser::parse() {
}

void RichTextParser::clear() {
    set_raw_text(String());
    _set_parsed_text(String());
    _set_width(0);
    _set_height(0);
    memdelete(root_item);
    _set_root_item(nullptr);
}

void RichTextParser::_bind_methods() {

    ClassDB::bind_method(D_METHOD("get_width"), &RichTextParser::get_width);
    ClassDB::bind_method(D_METHOD("get_height"), &RichTextParser::get_height);
    ClassDB::bind_method(D_METHOD("get_parsed_text"), &RichTextParser::get_parsed_text);

    ClassDB::bind_method(D_METHOD("set_raw_text", "raw"), &RichTextParser::set_raw_text);
    ClassDB::bind_method(D_METHOD("get_raw_text"), &RichTextParser::get_raw_text);

    ClassDB::bind_method(D_METHOD("parse"), &RichTextParser::parse);
    ClassDB::bind_method(D_METHOD("clear"), &RichTextParser::clear);

    ADD_SIGNAL(MethodInfo("item_entered", PropertyInfo(Variant::OBJECT, "item")));
    ADD_SIGNAL(MethodInfo("item_exited", PropertyInfo(Variant::OBJECT, "item")));

    ADD_SIGNAL(MethodInfo("text_processed", PropertyInfo(Variant::OBJECT, "item"), PropertyInfo(Variant::STRING, "text")));
}

RichTextParser::RichTextParser() {

}

RichTextParser::~RichTextParser() {
    if (root_item) {
        memdelete(root_item);
    }
}