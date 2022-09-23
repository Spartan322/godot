/*************************************************************************/
/*  script_text_editor.cpp                                               */
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

#include "script_text_editor.h"

#include "core/config/project_settings.h"
#include "core/math/expression.h"
#include "core/os/keyboard.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/editor_command_palette.h"
#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "editor/editor_settings.h"

void ConnectionInfoDialog::ok_pressed() {
}

void ConnectionInfoDialog::popup_connections(String p_method, Vector<Node *> p_nodes) {
	method->set_text(p_method);

	tree->clear();
	TreeItem *root = tree->create_item();

	for (int i = 0; i < p_nodes.size(); i++) {
		List<Connection> all_connections;
		p_nodes[i]->get_signals_connected_to_this(&all_connections);

		for (const Connection &connection : all_connections) {
			if (connection.callable.get_method() != p_method) {
				continue;
			}

			TreeItem *node_item = tree->create_item(root);

			node_item->set_text(0, Object::cast_to<Node>(connection.signal.get_object())->get_name());
			node_item->set_icon(0, EditorNode::get_singleton()->get_object_icon(connection.signal.get_object(), "Node"));
			node_item->set_selectable(0, false);
			node_item->set_editable(0, false);

			node_item->set_text(1, connection.signal.get_name());
			Control *p = Object::cast_to<Control>(get_parent());
			node_item->set_icon(1, p->get_theme_icon(SNAME("Slot"), SNAME("EditorIcons")));
			node_item->set_selectable(1, false);
			node_item->set_editable(1, false);

			node_item->set_text(2, Object::cast_to<Node>(connection.callable.get_object())->get_name());
			node_item->set_icon(2, EditorNode::get_singleton()->get_object_icon(connection.callable.get_object(), "Node"));
			node_item->set_selectable(2, false);
			node_item->set_editable(2, false);
		}
	}

	popup_centered(Size2(600, 300) * EDSCALE);
}

ConnectionInfoDialog::ConnectionInfoDialog() {
	set_title(TTR("Connections to method:"));

	VBoxContainer *vbc = memnew(VBoxContainer);
	vbc->set_anchor_and_offset(SIDE_LEFT, Control::ANCHOR_BEGIN, 8 * EDSCALE);
	vbc->set_anchor_and_offset(SIDE_TOP, Control::ANCHOR_BEGIN, 8 * EDSCALE);
	vbc->set_anchor_and_offset(SIDE_RIGHT, Control::ANCHOR_END, -8 * EDSCALE);
	vbc->set_anchor_and_offset(SIDE_BOTTOM, Control::ANCHOR_END, -8 * EDSCALE);
	add_child(vbc);

	method = memnew(Label);
	method->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	vbc->add_child(method);

	tree = memnew(Tree);
	tree->set_columns(3);
	tree->set_hide_root(true);
	tree->set_column_titles_visible(true);
	tree->set_column_title(0, TTR("Source"));
	tree->set_column_title(1, TTR("Signal"));
	tree->set_column_title(2, TTR("Target"));
	vbc->add_child(tree);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tree->set_allow_rmb_select(true);
}

////////////////////////////////////////////////////////////////////////////////

/* Taken from:                               */
/* /editor/editor_help.cpp - _add_text_to_rt */
static void _add_text_to_rt(const String &p_bbcode, RichTextLabel *p_rt) {
	DocTools *doc = EditorHelp::get_doc_data();
	String base_path;

	Ref<Font> doc_font = p_rt->get_theme_font(SNAME("doc"), SNAME("EditorFonts"));
	Ref<Font> doc_bold_font = p_rt->get_theme_font(SNAME("doc_bold"), SNAME("EditorFonts"));
	Ref<Font> doc_italic_font = p_rt->get_theme_font(SNAME("doc_italic"), SNAME("EditorFonts"));
	Ref<Font> doc_code_font = p_rt->get_theme_font(SNAME("doc_source"), SNAME("EditorFonts"));
	Ref<Font> doc_kbd_font = p_rt->get_theme_font(SNAME("doc_keyboard"), SNAME("EditorFonts"));

	Color link_color = p_rt->get_theme_color(SNAME("link_color"), SNAME("EditorHelp"));
	Color code_color = p_rt->get_theme_color(SNAME("code_color"), SNAME("EditorHelp"));
	Color kbd_color = p_rt->get_theme_color(SNAME("kbd_color"), SNAME("EditorHelp"));

	String bbcode = p_bbcode.dedent().replace("\t", "").replace("\r", "").strip_edges();

	// Select the correct code examples.
	switch ((int)EDITOR_GET("text_editor/help/class_reference_examples")) {
		case 0: // GDScript
			bbcode = bbcode.replace("[gdscript]", "[codeblock]");
			bbcode = bbcode.replace("[/gdscript]", "[/codeblock]");

			for (int pos = bbcode.find("[csharp]"); pos != -1; pos = bbcode.find("[csharp]")) {
				int end_pos = bbcode.find("[/csharp]");
				if (end_pos == -1) {
					WARN_PRINT("Unclosed [csharp] block or parse fail in code (search for tag errors)");
					break;
				}

				bbcode = bbcode.left(pos) + bbcode.substr(end_pos + 9); // 9 is length of "[/csharp]".
				while (bbcode[pos] == '\n') {
					bbcode = bbcode.left(pos) + bbcode.substr(pos + 1);
				}
			}
			break;
		case 1: // C#
			bbcode = bbcode.replace("[csharp]", "[codeblock]");
			bbcode = bbcode.replace("[/csharp]", "[/codeblock]");

			for (int pos = bbcode.find("[gdscript]"); pos != -1; pos = bbcode.find("[gdscript]")) {
				int end_pos = bbcode.find("[/gdscript]");
				if (end_pos == -1) {
					WARN_PRINT("Unclosed [gdscript] block or parse fail in code (search for tag errors)");
					break;
				}

				bbcode = bbcode.left(pos) + bbcode.substr(end_pos + 11); // 11 is length of "[/gdscript]".
				while (bbcode[pos] == '\n') {
					bbcode = bbcode.left(pos) + bbcode.substr(pos + 1);
				}
			}
			break;
		case 2: // GDScript and C#
			bbcode = bbcode.replace("[csharp]", "[b]C#:[/b]\n[codeblock]");
			bbcode = bbcode.replace("[gdscript]", "[b]GDScript:[/b]\n[codeblock]");

			bbcode = bbcode.replace("[/csharp]", "[/codeblock]");
			bbcode = bbcode.replace("[/gdscript]", "[/codeblock]");
			break;
	}

	// Remove codeblocks (they would be printed otherwise).
	bbcode = bbcode.replace("[codeblocks]\n", "");
	bbcode = bbcode.replace("\n[/codeblocks]", "");
	bbcode = bbcode.replace("[codeblocks]", "");
	bbcode = bbcode.replace("[/codeblocks]", "");

	// Remove extra new lines around code blocks.
	bbcode = bbcode.replace("[codeblock]\n", "[codeblock]");
	bbcode = bbcode.replace("\n[/codeblock]", "[/codeblock]");

	List<String> tag_stack;
	bool code_tag = false;
	bool codeblock_tag = false;

	int pos = 0;
	while (pos < bbcode.length()) {
		int brk_pos = bbcode.find("[", pos);

		if (brk_pos < 0) {
			brk_pos = bbcode.length();
		}

		if (brk_pos > pos) {
			String text = bbcode.substr(pos, brk_pos - pos);
			if (!code_tag && !codeblock_tag) {
				text = text.replace("\n", "\n\n");
			}
			p_rt->add_text(text);
		}

		if (brk_pos == bbcode.length()) {
			break; // Nothing else to add.
		}

		int brk_end = bbcode.find("]", brk_pos + 1);

		if (brk_end == -1) {
			String text = bbcode.substr(brk_pos, bbcode.length() - brk_pos);
			if (!code_tag && !codeblock_tag) {
				text = text.replace("\n", "\n\n");
			}
			p_rt->add_text(text);

			break;
		}

		String tag = bbcode.substr(brk_pos + 1, brk_end - brk_pos - 1);

		if (tag.begins_with("/")) {
			bool tag_ok = tag_stack.size() && tag_stack.front()->get() == tag.substr(1, tag.length());

			if (!tag_ok) {
				p_rt->add_text("[");
				pos = brk_pos + 1;
				continue;
			}

			tag_stack.pop_front();
			pos = brk_end + 1;
			if (tag != "/img") {
				p_rt->pop();
				if (code_tag) {
					// Pop both color and background color.
					p_rt->pop();
					p_rt->pop();
				} else if (codeblock_tag) {
					// Pop color, cell and table.
					p_rt->pop();
					p_rt->pop();
					p_rt->pop();
				}
			}
			code_tag = false;
			codeblock_tag = false;

		} else if (code_tag || codeblock_tag) {
			p_rt->add_text("[");
			pos = brk_pos + 1;

		} else if (tag.begins_with("method ") || tag.begins_with("member ") || tag.begins_with("signal ") || tag.begins_with("enum ") || tag.begins_with("constant ") || tag.begins_with("theme_item ")) {
			const int tag_end = tag.find(" ");
			const String link_tag = tag.substr(0, tag_end);
			const String link_target = tag.substr(tag_end + 1, tag.length()).lstrip(" ");

			// Use monospace font with translucent colored background color to make clickable references
			// easier to distinguish from inline code and other text.
			p_rt->push_font(doc_code_font);
			p_rt->push_color(link_color);
			p_rt->push_bgcolor(code_color * Color(1, 1, 1, 0.15));
			p_rt->push_meta("@" + link_tag + " " + link_target);
			p_rt->add_text(link_target + (tag.begins_with("method ") ? "()" : ""));
			p_rt->pop();
			p_rt->pop();
			p_rt->pop();
			p_rt->pop();
			pos = brk_end + 1;

		} else if (tag.begins_with("param ")) {
			const int tag_end = tag.find(" ");
			const String param_name = tag.substr(tag_end + 1, tag.length()).lstrip(" ");

			// Use monospace font with translucent background color to make code easier to distinguish from other text.
			p_rt->push_font(doc_code_font);
			p_rt->push_bgcolor(Color(0.5, 0.5, 0.5, 0.15));
			p_rt->push_color(code_color);
			p_rt->add_text(param_name);
			p_rt->pop();
			p_rt->pop();
			p_rt->pop();

			pos = brk_end + 1;

		} else if (doc->class_list.has(tag)) {
			// Class reference tag such as [Node2D] or [SceneTree].
			// Use monospace font with translucent colored background color to make clickable references
			// easier to distinguish from inline code and other text.
			p_rt->push_font(doc_code_font);
			p_rt->push_color(link_color);
			p_rt->push_bgcolor(code_color * Color(1, 1, 1, 0.15));
			p_rt->push_meta("#" + tag);
			p_rt->add_text(tag);
			p_rt->pop();
			p_rt->pop();
			p_rt->pop();
			p_rt->pop();
			pos = brk_end + 1;

		} else if (tag == "b") {
			// Use bold font.
			p_rt->push_font(doc_bold_font);
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		} else if (tag == "i") {
			// Use italics font.
			p_rt->push_font(doc_italic_font);
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		} else if (tag == "code") {
			// Use monospace font with translucent background color to make code easier to distinguish from other text.
			p_rt->push_font(doc_code_font);
			p_rt->push_bgcolor(Color(0.5, 0.5, 0.5, 0.15));
			p_rt->push_color(code_color);
			code_tag = true;
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		} else if (tag == "codeblock") {
			// Use monospace font with translucent background color to make code easier to distinguish from other text.
			// Use a single-column table with cell row background color instead of `[bgcolor]`.
			// This makes the background color highlight cover the entire block, rather than individual lines.
			p_rt->push_font(doc_code_font);
			p_rt->push_table(1);
			p_rt->push_cell();
			p_rt->set_cell_row_background_color(Color(0.5, 0.5, 0.5, 0.15), Color(0.5, 0.5, 0.5, 0.15));
			p_rt->set_cell_padding(Rect2(10 * EDSCALE, 10 * EDSCALE, 10 * EDSCALE, 10 * EDSCALE));
			p_rt->push_color(code_color);
			codeblock_tag = true;
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		} else if (tag == "kbd") {
			// Use keyboard font with custom color and background color.
			p_rt->push_font(doc_kbd_font);
			p_rt->push_bgcolor(Color(0.5, 0.5, 0.5, 0.15));
			p_rt->push_color(kbd_color);
			code_tag = true; // Though not strictly a code tag, logic is similar.
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		} else if (tag == "center") {
			// Align to center.
			p_rt->push_paragraph(HORIZONTAL_ALIGNMENT_CENTER, Control::TEXT_DIRECTION_AUTO, "");
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		} else if (tag == "br") {
			// Force a line break.
			p_rt->add_newline();
			pos = brk_end + 1;
		} else if (tag == "u") {
			// Use underline.
			p_rt->push_underline();
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		} else if (tag == "s") {
			// Use strikethrough.
			p_rt->push_strikethrough();
			pos = brk_end + 1;
			tag_stack.push_front(tag);

		} else if (tag == "url") {
			int end = bbcode.find("[", brk_end);
			if (end == -1) {
				end = bbcode.length();
			}
			String url = bbcode.substr(brk_end + 1, end - brk_end - 1);
			p_rt->push_meta(url);

			pos = brk_end + 1;
			tag_stack.push_front(tag);
		} else if (tag.begins_with("url=")) {
			String url = tag.substr(4, tag.length());
			p_rt->push_meta(url);
			pos = brk_end + 1;
			tag_stack.push_front("url");
		} else if (tag == "img") {
			int end = bbcode.find("[", brk_end);
			if (end == -1) {
				end = bbcode.length();
			}
			String image = bbcode.substr(brk_end + 1, end - brk_end - 1);

			Ref<Texture2D> texture = ResourceLoader::load(base_path.plus_file(image), "Texture2D");
			if (texture.is_valid()) {
				p_rt->add_image(texture);
			}

			pos = end;
			tag_stack.push_front(tag);
		} else if (tag.begins_with("color=")) {
			String col = tag.substr(6, tag.length());
			Color color = Color::from_string(col, Color());
			p_rt->push_color(color);
			pos = brk_end + 1;
			tag_stack.push_front("color");

		} else if (tag.begins_with("font=")) {
			String fnt = tag.substr(5, tag.length());

			Ref<Font> font = ResourceLoader::load(base_path.plus_file(fnt), "Font");
			if (font.is_valid()) {
				p_rt->push_font(font);
			} else {
				p_rt->push_font(doc_font);
			}

			pos = brk_end + 1;
			tag_stack.push_front("font");

		} else {
			p_rt->add_text("["); //ignore
			pos = brk_pos + 1;
		}
	}
}

void SymbolHintHelpBit::_go_to_help(String p_what) {
	EditorNode::get_singleton()->set_visible_editor(EditorNode::EDITOR_SCRIPT);
	ScriptEditor::get_singleton()->goto_help(p_what);
	emit_signal(SNAME("request_hide"));
}

void SymbolHintHelpBit::_meta_clicked(String p_select) {
	if (p_select.begins_with("$")) { //enum

		String select = p_select.substr(1, p_select.length());
		String class_name;
		if (select.contains(".")) {
			class_name = select.get_slice(".", 0);
		} else {
			class_name = "@Global";
		}
		_go_to_help("class_enum:" + class_name + ":" + select);
		return;
	} else if (p_select.begins_with("#")) {
		_go_to_help("class_name:" + p_select.substr(1, p_select.length()));
		return;
	} else if (p_select.begins_with("@")) {
		String m = p_select.substr(1, p_select.length());

		if (m.contains(".")) {
			_go_to_help("class_method:" + m.get_slice(".", 0) + ":" + m.get_slice(".", 0)); // Must go somewhere else.
		}
	}
}

void SymbolHintHelpBit::_bind_methods() {
	ADD_SIGNAL(MethodInfo("request_hide"));
}

void SymbolHintHelpBit::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED: {
			rich_text->add_theme_color_override("selection_color", get_theme_color(SNAME("selection_color")));
			set_symbol_hint(symbol_hint);
		} break;
	}
}

void SymbolHintHelpBit::_push_text(const String &p_text, bool p_pop) {
	rich_text->add_text(p_text);
	if (p_pop) {
		rich_text->pop();
	}
}

void SymbolHintHelpBit::set_symbol_hint(const ScriptLanguage::SymbolHint &p_symbol_hint) {
	RichTextLabel *label = get_rich_text();

	symbol_hint = p_symbol_hint;

	if (p_symbol_hint.type == ScriptLanguage::SymbolHint::SYMBOL_UNKNOWN) {
		return;
	}

	Color hint_symbol_color = EDITOR_GET("text_editor/hints/hover/symbol_color");
	Color hint_symbol_type_color = EDITOR_GET("text_editor/hints/hover/symbol_type_color");
	Color hint_symbol_value_color = EDITOR_GET("text_editor/hints/hover/value_color");

	rich_text->clear();
	switch (p_symbol_hint.type) {
		case ScriptLanguage::SymbolHint::SYMBOL_CLASS:
			label->push_color(hint_symbol_type_color);
			_push_text(TTR("class"));
			_push_text(" ", false);
			if (!p_symbol_hint._namespace.is_empty()) {
				label->push_color(hint_symbol_color);
				_push_text(p_symbol_hint._namespace);
			    _push_text("::", false);
			}
			label->push_color(hint_symbol_color);
			_push_text(p_symbol_hint.symbol);
			break;
		case ScriptLanguage::SymbolHint::SYMBOL_METHOD:
		case ScriptLanguage::SymbolHint::SYMBOL_SIGNAL: {
			bool is_method = p_symbol_hint.type == ScriptLanguage::SymbolHint::SYMBOL_METHOD;
			label->push_color(hint_symbol_type_color);
			_push_text(is_method ? TTR("method") : TTR("signal"));
		    _push_text(" ", false);
			if (!p_symbol_hint._namespace.is_empty()) {
				label->push_color(hint_symbol_color);
				_push_text(p_symbol_hint._namespace);
			    _push_text("::", false);
			}
			label->push_color(hint_symbol_color);
			_push_text(p_symbol_hint.symbol);
		    _push_text("(", false);
			for (int i = 0; i < p_symbol_hint.parameters.size(); i++) {
				if (i != 0) {
				    _push_text(",", false);
				}
				_push_text(" ", false);
				_push_text(p_symbol_hint.parameters[i].name, false);
				if (!p_symbol_hint.parameters[i].datatype.is_empty()) {
				    _push_text(":", false);
					label->push_color(hint_symbol_color);
					_push_text(p_symbol_hint.parameters[i].datatype);
				}
				if (p_symbol_hint.parameters[i].default_value.get_type() != Variant::NIL) {
				    _push_text(" = ", false);
					label->push_color(hint_symbol_value_color);
					_push_text(p_symbol_hint.parameters[i].default_value.stringify());
				}
			}
		    _push_text(")", false);
			if (is_method && !p_symbol_hint.datatype.is_empty()) {
			    _push_text(" -> ", false);
				label->push_color(hint_symbol_color);
				_push_text(p_symbol_hint.datatype);
			}
		} break;
		case ScriptLanguage::SymbolHint::SYMBOL_PROPERTY:
			label->push_color(hint_symbol_type_color);
			_push_text(TTR("property"));
		    _push_text(" ", false);
			if (!p_symbol_hint._namespace.is_empty()) {
				label->push_color(hint_symbol_color);
				_push_text(p_symbol_hint._namespace);
			    _push_text("::", false);
			}
			label->push_color(hint_symbol_color);
			_push_text(p_symbol_hint.symbol);
			if (!p_symbol_hint.datatype.is_empty()) {
			    _push_text(":", false);
				label->push_color(hint_symbol_color);
				_push_text(p_symbol_hint.datatype);
			}
			break;
		case ScriptLanguage::SymbolHint::SYMBOL_CONSTANT: {
			label->push_color(hint_symbol_type_color);
			_push_text(TTR("constant"));
		    _push_text(" ", false);
			if (!p_symbol_hint._namespace.is_empty()) {
				label->push_color(hint_symbol_color);
				_push_text(p_symbol_hint._namespace);
			    _push_text("::", false);
			}
			label->push_color(hint_symbol_color);
			_push_text(p_symbol_hint.symbol);
		    _push_text(" = ", false);
			label->push_color(hint_symbol_value_color);
			_push_text(p_symbol_hint.value.stringify());
		} break;
		case ScriptLanguage::SymbolHint::SYMBOL_LOCAL_VAR:
			label->push_color(hint_symbol_type_color);
			_push_text(TTR("local var"));
		    _push_text(" ", false);
			if (!p_symbol_hint._namespace.is_empty()) {
				label->push_color(hint_symbol_color);
				_push_text(p_symbol_hint._namespace);
			    _push_text(" ", false);
			}
			label->push_color(hint_symbol_color);
			_push_text(p_symbol_hint.symbol);
			if (!p_symbol_hint.datatype.is_empty()) {
			    _push_text(" : ", false);
				label->push_color(hint_symbol_color);
				_push_text(p_symbol_hint.datatype);
			}
			break;
		case ScriptLanguage::SymbolHint::SYMBOL_ANNOTATION:
			label->push_color(hint_symbol_type_color);
			_push_text(TTR("annotation"));
		    _push_text(" ", false);
			label->push_color(hint_symbol_color);
			_push_text(p_symbol_hint.symbol);
			_push_text("(", false);
			for (int i = 0; i < p_symbol_hint.parameters.size(); i++) {
				if (i != 0) {
				    _push_text(",", false);
				}
				_push_text(" ", false);
				_push_text(p_symbol_hint.parameters[i].name, false);
				if (!p_symbol_hint.parameters[i].datatype.is_empty()) {
				    _push_text(":", false);
					label->push_color(hint_symbol_color);
					_push_text(p_symbol_hint.parameters[i].datatype);
				}
			}
		    _push_text(")", false);
			break;
		case ScriptLanguage::SymbolHint::SYMBOL_ENUM:
			label->push_color(hint_symbol_type_color);
			_push_text(TTR("enum"));
		    _push_text(" ", false);
			if (!p_symbol_hint._namespace.is_empty()) {
				label->push_color(hint_symbol_color);
				_push_text(p_symbol_hint._namespace);
			    _push_text("::", false);
			}
			label->push_color(hint_symbol_color);
			_push_text(p_symbol_hint.symbol);
		    _push_text(" = ", false);
			label->push_color(hint_symbol_value_color);
			_push_text(p_symbol_hint.value.stringify());
			break;
		default:
			symbol_hint.type = ScriptLanguage::SymbolHint::SYMBOL_UNKNOWN;
			return;
	}

	String description;
	if (!symbol_hint.description.is_empty()) {
		label->add_newline();
		description = symbol_hint.description.strip_edges();
		Color description_color = EDITOR_GET("text_editor/hints/hover/description_default_color");
		label->push_color(description_color);
		_add_text_to_rt(description, label);
		label->pop();
	}
}

Size2 SymbolHintHelpBit::get_minimum_size() const {
	return MarginContainer::get_minimum_size() + Size2(MIN(rich_text->get_content_width(), 300 * EDSCALE), MIN(rich_text->get_content_height(), 400 * EDSCALE));
}

SymbolHintHelpBit::SymbolHintHelpBit() {
	rich_text = memnew(RichTextLabel);
	add_child(rich_text);
	rich_text->connect("meta_clicked", callable_mp(this, &SymbolHintHelpBit::_meta_clicked));
	rich_text->set_override_selected_font_color(false);
	rich_text->set_fit_content_height(false);
	rich_text->set_shortcut_keys_enabled(false);
	rich_text->set_use_bbcode(true);
	rich_text->set_scroll_active(true);
	rich_text->set_autowrap_mode(TextServer::AutowrapMode::AUTOWRAP_WORD);
	rich_text->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
}

////////////////////////////////////////////////////////////////////////////////

Vector<String> ScriptTextEditor::get_functions() {
	CodeEdit *te = code_editor->get_text_editor();
	String text = te->get_text();
	List<String> fnc;

	if (script->get_language()->validate(text, script->get_path(), &fnc)) {
		//if valid rewrite functions to latest
		functions.clear();
		for (const String &E : fnc) {
			functions.push_back(E);
		}
	}

	return functions;
}

void ScriptTextEditor::apply_code() {
	if (script.is_null()) {
		return;
	}
	script->set_source_code(code_editor->get_text_editor()->get_text());
	script->update_exports();
	code_editor->get_text_editor()->get_syntax_highlighter()->update_cache();
}

Ref<Resource> ScriptTextEditor::get_edited_resource() const {
	return script;
}

void ScriptTextEditor::set_edited_resource(const Ref<Resource> &p_res) {
	ERR_FAIL_COND(script.is_valid());
	ERR_FAIL_COND(p_res.is_null());

	script = p_res;

	code_editor->get_text_editor()->set_text(script->get_source_code());
	code_editor->get_text_editor()->clear_undo_history();
	code_editor->get_text_editor()->tag_saved_version();

	emit_signal(SNAME("name_changed"));
	code_editor->update_line_and_column();
}

void ScriptTextEditor::enable_editor() {
	if (editor_enabled) {
		return;
	}

	editor_enabled = true;

	_enable_code_editor();

	_validate_script();
}

void ScriptTextEditor::_load_theme_settings() {
	CodeEdit *text_edit = code_editor->get_text_editor();

	Color updated_marked_line_color = EDITOR_GET("text_editor/theme/highlighting/mark_color");
	Color updated_safe_line_number_color = EDITOR_GET("text_editor/theme/highlighting/safe_line_number_color");

	bool safe_line_number_color_updated = updated_safe_line_number_color != safe_line_number_color;
	bool marked_line_color_updated = updated_marked_line_color != marked_line_color;
	if (safe_line_number_color_updated || marked_line_color_updated) {
		safe_line_number_color = updated_safe_line_number_color;
		for (int i = 0; i < text_edit->get_line_count(); i++) {
			if (marked_line_color_updated && text_edit->get_line_background_color(i) == marked_line_color) {
				text_edit->set_line_background_color(i, updated_marked_line_color);
			}

			if (safe_line_number_color_updated && text_edit->get_line_gutter_item_color(i, line_number_gutter) != default_line_number_color) {
				text_edit->set_line_gutter_item_color(i, line_number_gutter, safe_line_number_color);
			}
		}
		marked_line_color = updated_marked_line_color;
	}

	theme_loaded = true;
	if (!script.is_null()) {
		_set_theme_for_script();
	}
}

void ScriptTextEditor::_set_theme_for_script() {
	if (!theme_loaded) {
		return;
	}

	CodeEdit *text_edit = code_editor->get_text_editor();
	text_edit->get_syntax_highlighter()->update_cache();

	List<String> strings;
	script->get_language()->get_string_delimiters(&strings);
	text_edit->clear_string_delimiters();
	for (const String &string : strings) {
		String beg = string.get_slice(" ", 0);
		String end = string.get_slice_count(" ") > 1 ? string.get_slice(" ", 1) : String();
		if (!text_edit->has_string_delimiter(beg)) {
			text_edit->add_string_delimiter(beg, end, end.is_empty());
		}

		if (!end.is_empty() && !text_edit->has_auto_brace_completion_open_key(beg)) {
			text_edit->add_auto_brace_completion_pair(beg, end);
		}
	}

	List<String> comments;
	script->get_language()->get_comment_delimiters(&comments);
	text_edit->clear_comment_delimiters();
	for (const String &comment : comments) {
		String beg = comment.get_slice(" ", 0);
		String end = comment.get_slice_count(" ") > 1 ? comment.get_slice(" ", 1) : String();
		text_edit->add_comment_delimiter(beg, end, end.is_empty());

		if (!end.is_empty() && !text_edit->has_auto_brace_completion_open_key(beg)) {
			text_edit->add_auto_brace_completion_pair(beg, end);
		}
	}
}

void ScriptTextEditor::_show_errors_panel(bool p_show) {
	errors_panel->set_visible(p_show);
}

void ScriptTextEditor::_show_warnings_panel(bool p_show) {
	warnings_panel->set_visible(p_show);
}

void ScriptTextEditor::_warning_clicked(Variant p_line) {
	if (p_line.get_type() == Variant::INT) {
		goto_line_centered(p_line.operator int64_t());
	} else if (p_line.get_type() == Variant::DICTIONARY) {
		Dictionary meta = p_line.operator Dictionary();
		const int line = meta["line"].operator int64_t() - 1;

		CodeEdit *text_editor = code_editor->get_text_editor();
		String prev_line = line > 0 ? text_editor->get_line(line - 1) : "";
		if (prev_line.contains("@warning_ignore")) {
			const int closing_bracket_idx = prev_line.find(")");
			const String text_to_insert = ", " + meta["code"].operator String();
			prev_line = prev_line.insert(closing_bracket_idx, text_to_insert);
			text_editor->set_line(line - 1, prev_line);
		} else {
			const int indent = text_editor->get_indent_level(line) / text_editor->get_indent_size();
			String annotation_indent;
			if (!text_editor->is_indent_using_spaces()) {
				annotation_indent = String("\t").repeat(indent);
			} else {
				annotation_indent = String(" ").repeat(text_editor->get_indent_size() * indent);
			}
			text_editor->insert_line_at(line, annotation_indent + "@warning_ignore(" + meta["code"].operator String() + ")");
		}

		_validate_script();
	}
}

void ScriptTextEditor::_error_clicked(Variant p_line) {
	if (p_line.get_type() == Variant::INT) {
		code_editor->get_text_editor()->set_caret_line(p_line.operator int64_t());
	}
}

void ScriptTextEditor::reload_text() {
	ERR_FAIL_COND(script.is_null());

	CodeEdit *te = code_editor->get_text_editor();
	int column = te->get_caret_column();
	int row = te->get_caret_line();
	int h = te->get_h_scroll();
	int v = te->get_v_scroll();

	te->set_text(script->get_source_code());
	te->set_caret_line(row);
	te->set_caret_column(column);
	te->set_h_scroll(h);
	te->set_v_scroll(v);

	te->tag_saved_version();

	code_editor->update_line_and_column();
}

void ScriptTextEditor::add_callback(const String &p_function, PackedStringArray p_args) {
	String code = code_editor->get_text_editor()->get_text();
	int pos = script->get_language()->find_function(p_function, code);
	if (pos == -1) {
		//does not exist
		code_editor->get_text_editor()->deselect();
		pos = code_editor->get_text_editor()->get_line_count() + 2;
		String func = script->get_language()->make_function("", p_function, p_args);
		//code=code+func;
		code_editor->get_text_editor()->set_caret_line(pos + 1);
		code_editor->get_text_editor()->set_caret_column(1000000); //none shall be that big
		code_editor->get_text_editor()->insert_text_at_caret("\n\n" + func);
	}
	code_editor->get_text_editor()->set_caret_line(pos);
	code_editor->get_text_editor()->set_caret_column(1);
}

bool ScriptTextEditor::show_members_overview() {
	return true;
}

void ScriptTextEditor::update_settings() {
	symbol_hint_popup->set_max_size(EditorSettings::get_singleton()->get("text_editor/hints/hover/max_size"));
	
	code_editor->get_text_editor()->set_gutter_draw(connection_gutter, EditorSettings::get_singleton()->get("text_editor/appearance/gutters/show_info_gutter"));
	code_editor->update_editor_settings();
}

bool ScriptTextEditor::is_unsaved() {
	const bool unsaved =
			code_editor->get_text_editor()->get_version() != code_editor->get_text_editor()->get_saved_version() ||
			script->get_path().is_empty(); // In memory.
	return unsaved;
}

Variant ScriptTextEditor::get_edit_state() {
	return code_editor->get_edit_state();
}

void ScriptTextEditor::set_edit_state(const Variant &p_state) {
	code_editor->set_edit_state(p_state);

	Dictionary state = p_state;
	if (state.has("syntax_highlighter")) {
		int idx = highlighter_menu->get_item_idx_from_text(state["syntax_highlighter"]);
		if (idx >= 0) {
			_change_syntax_highlighter(idx);
		}
	}

	if (editor_enabled) {
#ifndef ANDROID_ENABLED
		ensure_focus();
#endif
	}
}

void ScriptTextEditor::_convert_case(CodeTextEditor::CaseStyle p_case) {
	code_editor->convert_case(p_case);
}

void ScriptTextEditor::trim_trailing_whitespace() {
	code_editor->trim_trailing_whitespace();
}

void ScriptTextEditor::insert_final_newline() {
	code_editor->insert_final_newline();
}

void ScriptTextEditor::convert_indent_to_spaces() {
	code_editor->convert_indent_to_spaces();
}

void ScriptTextEditor::convert_indent_to_tabs() {
	code_editor->convert_indent_to_tabs();
}

void ScriptTextEditor::tag_saved_version() {
	code_editor->get_text_editor()->tag_saved_version();
}

void ScriptTextEditor::goto_line(int p_line, bool p_with_error) {
	code_editor->goto_line(p_line);
}

void ScriptTextEditor::goto_line_selection(int p_line, int p_begin, int p_end) {
	code_editor->goto_line_selection(p_line, p_begin, p_end);
}

void ScriptTextEditor::goto_line_centered(int p_line) {
	code_editor->goto_line_centered(p_line);
}

void ScriptTextEditor::set_executing_line(int p_line) {
	code_editor->set_executing_line(p_line);
}

void ScriptTextEditor::clear_executing_line() {
	code_editor->clear_executing_line();
}

void ScriptTextEditor::ensure_focus() {
	code_editor->get_text_editor()->grab_focus();
}

String ScriptTextEditor::get_name() {
	String name;

	name = script->get_path().get_file();
	if (name.is_empty()) {
		// This appears for newly created built-in scripts before saving the scene.
		name = TTR("[unsaved]");
	} else if (script->is_built_in()) {
		const String &script_name = script->get_name();
		if (!script_name.is_empty()) {
			// If the built-in script has a custom resource name defined,
			// display the built-in script name as follows: `ResourceName (scene_file.tscn)`
			name = vformat("%s (%s)", script_name, name.get_slice("::", 0));
		}
	}

	if (is_unsaved()) {
		name += "(*)";
	}

	return name;
}

Ref<Texture2D> ScriptTextEditor::get_theme_icon() {
	if (get_parent_control()) {
		String icon_name = script->get_class();
		if (script->is_built_in()) {
			icon_name += "Internal";
		}

		if (get_parent_control()->has_theme_icon(icon_name, SNAME("EditorIcons"))) {
			return get_parent_control()->get_theme_icon(icon_name, SNAME("EditorIcons"));
		} else if (get_parent_control()->has_theme_icon(script->get_class(), SNAME("EditorIcons"))) {
			return get_parent_control()->get_theme_icon(script->get_class(), SNAME("EditorIcons"));
		}
	}

	return Ref<Texture2D>();
}

void ScriptTextEditor::_validate_script() {
	CodeEdit *te = code_editor->get_text_editor();

	String text = te->get_text();
	List<String> fnc;

	warnings.clear();
	errors.clear();
	safe_lines.clear();

	if (!script->get_language()->validate(text, script->get_path(), &fnc, &errors, &warnings, &safe_lines)) {
		// TRANSLATORS: Script error pointing to a line and column number.
		String error_text = vformat(TTR("Error at (%d, %d):"), errors[0].line, errors[0].column) + " " + errors[0].message;
		code_editor->set_error(error_text);
		code_editor->set_error_pos(errors[0].line - 1, errors[0].column - 1);
		script_is_valid = false;
	} else {
		code_editor->set_error("");
		if (!script->is_tool()) {
			script->set_source_code(text);
			script->update_exports();
			te->get_syntax_highlighter()->update_cache();
		}

		functions.clear();
		for (const String &E : fnc) {
			functions.push_back(E);
		}
		script_is_valid = true;
	}
	_update_connected_methods();
	_update_warnings();
	_update_errors();

	emit_signal(SNAME("name_changed"));
	emit_signal(SNAME("edited_script_changed"));
}

void ScriptTextEditor::_update_warnings() {
	int warning_nb = warnings.size();
	warnings_panel->clear();

	bool has_connections_table = false;
	// Add missing connections.
	if (GLOBAL_GET("debug/gdscript/warnings/enable").booleanize()) {
		Node *base = get_tree()->get_edited_scene_root();
		if (base && missing_connections.size() > 0) {
			has_connections_table = true;
			warnings_panel->push_table(1);
			for (const Connection &connection : missing_connections) {
				String base_path = base->get_name();
				String source_path = base == connection.signal.get_object() ? base_path : base_path + "/" + base->get_path_to(Object::cast_to<Node>(connection.signal.get_object()));
				String target_path = base == connection.callable.get_object() ? base_path : base_path + "/" + base->get_path_to(Object::cast_to<Node>(connection.callable.get_object()));

				warnings_panel->push_cell();
				warnings_panel->push_color(warnings_panel->get_theme_color(SNAME("warning_color"), SNAME("Editor")));
				warnings_panel->add_text(vformat(TTR("Missing connected method '%s' for signal '%s' from node '%s' to node '%s'."), connection.callable.get_method(), connection.signal.get_name(), source_path, target_path));
				warnings_panel->pop(); // Color.
				warnings_panel->pop(); // Cell.
			}
			warnings_panel->pop(); // Table.

			warning_nb += missing_connections.size();
		}
	}

	code_editor->set_warning_count(warning_nb);

	if (has_connections_table) {
		warnings_panel->add_newline();
	}

	// Add script warnings.
	warnings_panel->push_table(3);
	for (const ScriptLanguage::Warning &w : warnings) {
		Dictionary ignore_meta;
		ignore_meta["line"] = w.start_line;
		ignore_meta["code"] = w.string_code.to_lower();
		warnings_panel->push_cell();
		warnings_panel->push_meta(ignore_meta);
		warnings_panel->push_color(
				warnings_panel->get_theme_color(SNAME("accent_color"), SNAME("Editor")).lerp(warnings_panel->get_theme_color(SNAME("mono_color"), SNAME("Editor")), 0.5f));
		warnings_panel->add_text(TTR("[Ignore]"));
		warnings_panel->pop(); // Color.
		warnings_panel->pop(); // Meta ignore.
		warnings_panel->pop(); // Cell.

		warnings_panel->push_cell();
		warnings_panel->push_meta(w.start_line - 1);
		warnings_panel->push_color(warnings_panel->get_theme_color(SNAME("warning_color"), SNAME("Editor")));
		warnings_panel->add_text(TTR("Line") + " " + itos(w.start_line));
		warnings_panel->add_text(" (" + w.string_code + "):");
		warnings_panel->pop(); // Color.
		warnings_panel->pop(); // Meta goto.
		warnings_panel->pop(); // Cell.

		warnings_panel->push_cell();
		warnings_panel->add_text(w.message);
		warnings_panel->pop(); // Cell.
	}
	warnings_panel->pop(); // Table.
}

void ScriptTextEditor::_update_errors() {
	code_editor->set_error_count(errors.size());

	errors_panel->clear();
	errors_panel->push_table(2);
	for (const ScriptLanguage::ScriptError &err : errors) {
		errors_panel->push_cell();
		errors_panel->push_meta(err.line - 1);
		errors_panel->push_color(warnings_panel->get_theme_color(SNAME("error_color"), SNAME("Editor")));
		errors_panel->add_text(TTR("Line") + " " + itos(err.line) + ":");
		errors_panel->pop(); // Color.
		errors_panel->pop(); // Meta goto.
		errors_panel->pop(); // Cell.

		errors_panel->push_cell();
		errors_panel->add_text(err.message);
		errors_panel->pop(); // Cell.
	}
	errors_panel->pop(); // Table

	CodeEdit *te = code_editor->get_text_editor();
	bool highlight_safe = EDITOR_GET("text_editor/appearance/gutters/highlight_type_safe_lines");
	bool last_is_safe = false;
	for (int i = 0; i < te->get_line_count(); i++) {
		if (errors.is_empty()) {
			te->set_line_background_color(i, Color(0, 0, 0, 0));
		} else {
			for (const ScriptLanguage::ScriptError &E : errors) {
				bool error_line = i == E.line - 1;
				te->set_line_background_color(i, error_line ? marked_line_color : Color(0, 0, 0, 0));
				if (error_line) {
					break;
				}
			}
		}

		if (highlight_safe) {
			if (safe_lines.has(i + 1)) {
				te->set_line_gutter_item_color(i, line_number_gutter, safe_line_number_color);
				last_is_safe = true;
			} else if (last_is_safe && (te->is_in_comment(i) != -1 || te->get_line(i).strip_edges().is_empty())) {
				te->set_line_gutter_item_color(i, line_number_gutter, safe_line_number_color);
			} else {
				te->set_line_gutter_item_color(i, line_number_gutter, default_line_number_color);
				last_is_safe = false;
			}
		} else {
			te->set_line_gutter_item_color(i, 1, default_line_number_color);
		}
	}
}

void ScriptTextEditor::_update_bookmark_list() {
	bookmarks_menu->clear();
	bookmarks_menu->reset_size();

	bookmarks_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_bookmark"), BOOKMARK_TOGGLE);
	bookmarks_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/remove_all_bookmarks"), BOOKMARK_REMOVE_ALL);
	bookmarks_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/goto_next_bookmark"), BOOKMARK_GOTO_NEXT);
	bookmarks_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/goto_previous_bookmark"), BOOKMARK_GOTO_PREV);

	PackedInt32Array bookmark_list = code_editor->get_text_editor()->get_bookmarked_lines();
	if (bookmark_list.size() == 0) {
		return;
	}

	bookmarks_menu->add_separator();

	for (int i = 0; i < bookmark_list.size(); i++) {
		// Strip edges to remove spaces or tabs.
		// Also replace any tabs by spaces, since we can't print tabs in the menu.
		String line = code_editor->get_text_editor()->get_line(bookmark_list[i]).replace("\t", "  ").strip_edges();

		// Limit the size of the line if too big.
		if (line.length() > 50) {
			line = line.substr(0, 50);
		}

		bookmarks_menu->add_item(String::num((int)bookmark_list[i] + 1) + " - `" + line + "`");
		bookmarks_menu->set_item_metadata(-1, bookmark_list[i]);
	}
}

void ScriptTextEditor::_bookmark_item_pressed(int p_idx) {
	if (p_idx < 4) { // Any item before the separator.
		_edit_option(bookmarks_menu->get_item_id(p_idx));
	} else {
		code_editor->goto_line_centered(bookmarks_menu->get_item_metadata(p_idx));
	}
}

static Vector<Node *> _find_all_node_for_script(Node *p_base, Node *p_current, const Ref<Script> &p_script) {
	Vector<Node *> nodes;

	if (p_current->get_owner() != p_base && p_base != p_current) {
		return nodes;
	}

	Ref<Script> c = p_current->get_script();
	if (c == p_script) {
		nodes.push_back(p_current);
	}

	for (int i = 0; i < p_current->get_child_count(); i++) {
		Vector<Node *> found = _find_all_node_for_script(p_base, p_current->get_child(i), p_script);
		nodes.append_array(found);
	}

	return nodes;
}

static Node *_find_node_for_script(Node *p_base, Node *p_current, const Ref<Script> &p_script) {
	if (p_current->get_owner() != p_base && p_base != p_current) {
		return nullptr;
	}
	Ref<Script> c = p_current->get_script();
	if (c == p_script) {
		return p_current;
	}
	for (int i = 0; i < p_current->get_child_count(); i++) {
		Node *found = _find_node_for_script(p_base, p_current->get_child(i), p_script);
		if (found) {
			return found;
		}
	}

	return nullptr;
}

static void _find_changed_scripts_for_external_editor(Node *p_base, Node *p_current, HashSet<Ref<Script>> &r_scripts) {
	if (p_current->get_owner() != p_base && p_base != p_current) {
		return;
	}
	Ref<Script> c = p_current->get_script();

	if (c.is_valid()) {
		r_scripts.insert(c);
	}

	for (int i = 0; i < p_current->get_child_count(); i++) {
		_find_changed_scripts_for_external_editor(p_base, p_current->get_child(i), r_scripts);
	}
}

void ScriptEditor::_update_modified_scripts_for_external_editor(Ref<Script> p_for_script) {
	if (!bool(EditorSettings::get_singleton()->get("text_editor/external/use_external_editor"))) {
		return;
	}

	ERR_FAIL_COND(!get_tree());

	HashSet<Ref<Script>> scripts;

	Node *base = get_tree()->get_edited_scene_root();
	if (base) {
		_find_changed_scripts_for_external_editor(base, base, scripts);
	}

	for (const Ref<Script> &E : scripts) {
		Ref<Script> script = E;

		if (p_for_script.is_valid() && p_for_script != script) {
			continue;
		}

		if (script->is_built_in()) {
			continue; //internal script, who cares, though weird
		}

		uint64_t last_date = script->get_last_modified_time();
		uint64_t date = FileAccess::get_modified_time(script->get_path());

		if (last_date != date) {
			Ref<Script> rel_script = ResourceLoader::load(script->get_path(), script->get_class(), ResourceFormatLoader::CACHE_MODE_IGNORE);
			ERR_CONTINUE(!rel_script.is_valid());
			script->set_source_code(rel_script->get_source_code());
			script->set_last_modified_time(rel_script->get_last_modified_time());
			script->update_exports();

			_trigger_live_script_reload();
		}
	}
}

void ScriptTextEditor::_code_complete_scripts(void *p_ud, const String &p_code, List<ScriptLanguage::CodeCompletionOption> *r_options, bool &r_force) {
	ScriptTextEditor *ste = (ScriptTextEditor *)p_ud;
	ste->_code_complete_script(p_code, r_options, r_force);
}

void ScriptTextEditor::_code_complete_script(const String &p_code, List<ScriptLanguage::CodeCompletionOption> *r_options, bool &r_force) {
	if (color_panel->is_visible()) {
		return;
	}
	Node *base = get_tree()->get_edited_scene_root();
	if (base) {
		base = _find_node_for_script(base, base, script);
	}
	String hint;
	Error err = script->get_language()->complete_code(p_code, script->get_path(), base, r_options, r_force, hint);

	r_options->sort_custom_inplace<CodeCompletionOptionCompare>();

	if (err == OK) {
		code_editor->get_text_editor()->set_code_hint(hint);
	}
}

void ScriptTextEditor::_update_breakpoint_list() {
	breakpoints_menu->clear();
	breakpoints_menu->reset_size();

	breakpoints_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_breakpoint"), DEBUG_TOGGLE_BREAKPOINT);
	breakpoints_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/remove_all_breakpoints"), DEBUG_REMOVE_ALL_BREAKPOINTS);
	breakpoints_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/goto_next_breakpoint"), DEBUG_GOTO_NEXT_BREAKPOINT);
	breakpoints_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/goto_previous_breakpoint"), DEBUG_GOTO_PREV_BREAKPOINT);

	PackedInt32Array breakpoint_list = code_editor->get_text_editor()->get_breakpointed_lines();
	if (breakpoint_list.size() == 0) {
		return;
	}

	breakpoints_menu->add_separator();

	for (int i = 0; i < breakpoint_list.size(); i++) {
		// Strip edges to remove spaces or tabs.
		// Also replace any tabs by spaces, since we can't print tabs in the menu.
		String line = code_editor->get_text_editor()->get_line(breakpoint_list[i]).replace("\t", "  ").strip_edges();

		// Limit the size of the line if too big.
		if (line.length() > 50) {
			line = line.substr(0, 50);
		}

		breakpoints_menu->add_item(String::num((int)breakpoint_list[i] + 1) + " - `" + line + "`");
		breakpoints_menu->set_item_metadata(-1, breakpoint_list[i]);
	}
}

void ScriptTextEditor::_breakpoint_item_pressed(int p_idx) {
	if (p_idx < 4) { // Any item before the separator.
		_edit_option(breakpoints_menu->get_item_id(p_idx));
	} else {
		code_editor->goto_line(breakpoints_menu->get_item_metadata(p_idx));
		code_editor->get_text_editor()->call_deferred(SNAME("center_viewport_to_caret")); //Need to be deferred, because goto uses call_deferred().
	}
}

void ScriptTextEditor::_breakpoint_toggled(int p_row) {
	EditorDebuggerNode::get_singleton()->set_breakpoint(script->get_path(), p_row + 1, code_editor->get_text_editor()->is_line_breakpointed(p_row));
}

void ScriptTextEditor::_lookup_symbol(const String &p_symbol, int p_row, int p_column) {
	Node *base = get_tree()->get_edited_scene_root();
	if (base) {
		base = _find_node_for_script(base, base, script);
	}

	ScriptLanguage::LookupResult result;
	if (ScriptServer::is_global_class(p_symbol)) {
		EditorNode::get_singleton()->load_resource(ScriptServer::get_global_class_path(p_symbol));
	} else if (p_symbol.is_resource_file()) {
		List<String> scene_extensions;
		ResourceLoader::get_recognized_extensions_for_type("PackedScene", &scene_extensions);

		if (scene_extensions.find(p_symbol.get_extension())) {
			EditorNode::get_singleton()->load_scene(p_symbol);
		} else {
			EditorNode::get_singleton()->load_resource(p_symbol);
		}

	} else if (script->get_language()->lookup_code(code_editor->get_text_editor()->get_text_for_symbol_lookup(), p_symbol, script->get_path(), base, result) == OK) {
		_goto_line(p_row);

		switch (result.type) {
			case ScriptLanguage::LOOKUP_RESULT_SCRIPT_LOCATION: {
				if (result.script.is_valid()) {
					emit_signal(SNAME("request_open_script_at_line"), result.script, result.location - 1);
				} else {
					emit_signal(SNAME("request_save_history"));
					goto_line_centered(result.location - 1);
				}
			} break;
			case ScriptLanguage::LOOKUP_RESULT_CLASS: {
				emit_signal(SNAME("go_to_help"), "class_name:" + result.class_name);
			} break;
			case ScriptLanguage::LOOKUP_RESULT_CLASS_CONSTANT: {
				StringName cname = result.class_name;
				bool success;
				while (true) {
					ClassDB::get_integer_constant(cname, result.class_member, &success);
					if (success) {
						result.class_name = cname;
						cname = ClassDB::get_parent_class(cname);
					} else {
						break;
					}
				}

				emit_signal(SNAME("go_to_help"), "class_constant:" + result.class_name + ":" + result.class_member);

			} break;
			case ScriptLanguage::LOOKUP_RESULT_CLASS_PROPERTY: {
				emit_signal(SNAME("go_to_help"), "class_property:" + result.class_name + ":" + result.class_member);

			} break;
			case ScriptLanguage::LOOKUP_RESULT_CLASS_METHOD: {
				StringName cname = result.class_name;

				while (true) {
					if (ClassDB::has_method(cname, result.class_member)) {
						result.class_name = cname;
						cname = ClassDB::get_parent_class(cname);
					} else {
						break;
					}
				}

				emit_signal(SNAME("go_to_help"), "class_method:" + result.class_name + ":" + result.class_member);

			} break;
			case ScriptLanguage::LOOKUP_RESULT_CLASS_SIGNAL: {
				StringName cname = result.class_name;

				while (true) {
					if (ClassDB::has_signal(cname, result.class_member)) {
						result.class_name = cname;
						cname = ClassDB::get_parent_class(cname);
					} else {
						break;
					}
				}

				emit_signal(SNAME("go_to_help"), "class_signal:" + result.class_name + ":" + result.class_member);

			} break;
			case ScriptLanguage::LOOKUP_RESULT_CLASS_ENUM: {
				StringName cname = result.class_name;
				StringName success;
				while (true) {
					success = ClassDB::get_integer_constant_enum(cname, result.class_member, true);
					if (success != StringName()) {
						result.class_name = cname;
						cname = ClassDB::get_parent_class(cname);
					} else {
						break;
					}
				}

				emit_signal(SNAME("go_to_help"), "class_enum:" + result.class_name + ":" + result.class_member);

			} break;
			case ScriptLanguage::LOOKUP_RESULT_CLASS_ANNOTATION: {
				emit_signal(SNAME("go_to_help"), "class_annotation:" + result.class_name + ":" + result.class_member);
			} break;
			case ScriptLanguage::LOOKUP_RESULT_CLASS_TBD_GLOBALSCOPE: {
				emit_signal(SNAME("go_to_help"), "class_global:" + result.class_name + ":" + result.class_member);
			} break;
			default: {
			}
		}
	} else if (ProjectSettings::get_singleton()->has_autoload(p_symbol)) {
		// Check for Autoload scenes.
		const ProjectSettings::AutoloadInfo &info = ProjectSettings::get_singleton()->get_autoload(p_symbol);
		if (info.is_singleton) {
			EditorNode::get_singleton()->load_scene(info.path);
		}
	} else if (p_symbol.is_relative_path()) {
		// Every symbol other than absolute path is relative path so keep this condition at last.
		String path = _get_absolute_path(p_symbol);
		if (FileAccess::exists(path)) {
			List<String> scene_extensions;
			ResourceLoader::get_recognized_extensions_for_type("PackedScene", &scene_extensions);

			if (scene_extensions.find(path.get_extension())) {
				EditorNode::get_singleton()->load_scene(path);
			} else {
				EditorNode::get_singleton()->load_resource(path);
			}
		}
	}
}

void ScriptTextEditor::_validate_symbol(const String &p_symbol) {
	_hide_symbol_hint();

	CodeEdit *text_edit = code_editor->get_text_editor();

	Node *base = get_tree()->get_edited_scene_root();
	if (base) {
		base = _find_node_for_script(base, base, script);
	}

	ScriptLanguage::LookupResult result;
	result.hint.symbol = p_symbol;
	if (ScriptServer::is_global_class(p_symbol) || p_symbol.is_resource_file() || script->get_language()->lookup_code(code_editor->get_text_editor()->get_text_for_symbol_lookup(), p_symbol, script->get_path(), base, result) == OK || (ProjectSettings::get_singleton()->has_autoload(p_symbol) && ProjectSettings::get_singleton()->get_autoload(p_symbol).is_singleton)) {
		text_edit->set_symbol_lookup_word_as_valid(true);
	} else if (p_symbol.is_relative_path()) {
		String path = _get_absolute_path(p_symbol);
		if (FileAccess::exists(path)) {
			text_edit->set_symbol_lookup_word_as_valid(true);
		} else {
			text_edit->set_symbol_lookup_word_as_valid(false);
		}
	} else {
		text_edit->set_symbol_lookup_word_as_valid(false);
	}
}

void ScriptTextEditor::_hovered_symbol(const String &p_symbol, Point2i m_column_line) {
	if (!EDITOR_GET("text_editor/hints/hover/enabled") || p_symbol.is_empty() || m_column_line.y == -1) {
		_hide_symbol_hint();
		return;
	}

	Node *base = get_tree()->get_edited_scene_root();
	if (base) {
		base = _find_node_for_script(base, base, script);
	}

	ScriptLanguage::LookupResult result;
	result.hint.symbol = p_symbol;
	if (ScriptServer::is_global_class(p_symbol)) {
		result.hint.type = ScriptLanguage::SymbolHint::SYMBOL_CLASS;
		result.hint.description = "In " + ScriptServer::get_global_class_path(p_symbol);
		// TODO: Description.
	} else if (script->get_language()->lookup_code(code_editor->get_text_editor()->get_text_for_symbol_lookup(), p_symbol, script->get_path(), base, result) != OK) {
		_hide_symbol_hint();
		return;
	}

	if ((ProjectSettings::get_singleton()->has_autoload(p_symbol) && ProjectSettings::get_singleton()->get_autoload(p_symbol).is_singleton)) {
		result.hint.type = ScriptLanguage::SymbolHint::SYMBOL_PROPERTY;
		result.hint.datatype = "Resource";
	}

	if (symbol_hint_column_line == m_column_line && result.hint.symbol == symbol_hint_help_bit->get_symbol_hint().symbol) {
		return;
	}

	symbol_hint_column_line = m_column_line;

	if (result.hint.type == ScriptLanguage::SymbolHint::SYMBOL_UNKNOWN) {
		_hide_symbol_hint();
		return;
	}

	symbol_hint = result.hint;

	_hide_symbol_hint();
	symbol_hint_help_bit->set_symbol_hint(symbol_hint);
	symbol_hint_popup->reset_size();

	symbol_hint_timer = get_tree()->create_timer(EDITOR_GET("text_editor/hints/hover/hover_delay"));
	symbol_hint_timer->set_ignore_time_scale(true);
	symbol_hint_timer->connect("timeout", callable_mp(this, &ScriptTextEditor::_show_symbol_hint), CONNECT_DEFERRED);
}

void ScriptTextEditor::_show_symbol_hint() {
	if (symbol_hint_timer.is_valid()) {
		symbol_hint_timer->release_connections();
		symbol_hint_timer = Ref<SceneTreeTimer>();
	}

	CodeEdit *text_edit = code_editor->get_text_editor();

	// symbol_hint_popup_position.x is the symbol's first column
	// calculated to align the left of the hint with the first column
	// symbol_hint_popup_position.y is the symbol's line
	// calculated to align the top of the hint with the bottom of the symbol's line.
	Point2i symbol_hint_popup_position = text_edit->get_pos_at_line_column(symbol_hint_column_line.y, symbol_hint_column_line.x);
	symbol_hint_popup_position = get_screen_transform().xform(symbol_hint_popup_position);

	symbol_hint_popup->set_current_screen(symbol_hint_popup->get_parent_visible_window()->get_current_screen());
	symbol_hint_popup->set_position(symbol_hint_popup_position);
	symbol_hint_popup->show();
}

void ScriptTextEditor::_hide_symbol_hint() {
	if (symbol_hint_timer.is_valid()) {
		symbol_hint_timer->release_connections();
		symbol_hint_timer = Ref<SceneTreeTimer>();
	}
	if (symbol_hint_popup->is_visible()) {
		symbol_hint_popup->hide();
		symbol_hint_help_bit->set_symbol_hint(ScriptLanguage::SymbolHint());
	}
}

String ScriptTextEditor::_get_absolute_path(const String &rel_path) {
	String base_path = script->get_path().get_base_dir();
	String path = base_path.path_join(rel_path);
	return path.replace("///", "//").simplify_path();
}

void ScriptTextEditor::update_toggle_scripts_button() {
	code_editor->update_toggle_scripts_button();
}

void ScriptTextEditor::_update_connected_methods() {
	CodeEdit *text_edit = code_editor->get_text_editor();
	text_edit->set_gutter_width(connection_gutter, text_edit->get_line_height());
	for (int i = 0; i < text_edit->get_line_count(); i++) {
		if (text_edit->get_line_gutter_metadata(i, connection_gutter) == "") {
			continue;
		}
		text_edit->set_line_gutter_metadata(i, connection_gutter, "");
		text_edit->set_line_gutter_icon(i, connection_gutter, nullptr);
		text_edit->set_line_gutter_clickable(i, connection_gutter, false);
	}
	missing_connections.clear();

	if (!script_is_valid) {
		return;
	}

	Node *base = get_tree()->get_edited_scene_root();
	if (!base) {
		return;
	}

	Vector<Node *> nodes = _find_all_node_for_script(base, base, script);
	HashSet<StringName> methods_found;
	for (int i = 0; i < nodes.size(); i++) {
		List<Connection> connections;
		nodes[i]->get_signals_connected_to_this(&connections);

		for (const Connection &connection : connections) {
			if (!(connection.flags & CONNECT_PERSIST)) {
				continue;
			}

			// As deleted nodes are still accessible via the undo/redo system, check if they're still on the tree.
			Node *source = Object::cast_to<Node>(connection.signal.get_object());
			if (source && !source->is_inside_tree()) {
				continue;
			}

			const StringName method = connection.callable.get_method();
			if (methods_found.has(method)) {
				continue;
			}

			if (!ClassDB::has_method(script->get_instance_base_type(), method)) {
				int line = -1;

				for (int j = 0; j < functions.size(); j++) {
					String name = functions[j].get_slice(":", 0);
					if (name == method) {
						line = functions[j].get_slice(":", 1).to_int() - 1;
						text_edit->set_line_gutter_metadata(line, connection_gutter, method);
						text_edit->set_line_gutter_icon(line, connection_gutter, get_parent_control()->get_theme_icon(SNAME("Slot"), SNAME("EditorIcons")));
						text_edit->set_line_gutter_clickable(line, connection_gutter, true);
						methods_found.insert(method);
						break;
					}
				}

				if (line >= 0) {
					continue;
				}

				// There is a chance that the method is inherited from another script.
				bool found_inherited_function = false;
				Ref<Script> inherited_script = script->get_base_script();
				while (!inherited_script.is_null()) {
					if (inherited_script->has_method(method)) {
						found_inherited_function = true;
						break;
					}

					inherited_script = inherited_script->get_base_script();
				}

				if (!found_inherited_function) {
					missing_connections.push_back(connection);
				}
			}
		}
	}
}

void ScriptTextEditor::_update_gutter_indexes() {
	for (int i = 0; i < code_editor->get_text_editor()->get_gutter_count(); i++) {
		if (code_editor->get_text_editor()->get_gutter_name(i) == "connection_gutter") {
			connection_gutter = i;
			continue;
		}

		if (code_editor->get_text_editor()->get_gutter_name(i) == "line_numbers") {
			line_number_gutter = i;
			continue;
		}
	}
}

void ScriptTextEditor::_gutter_clicked(int p_line, int p_gutter) {
	if (p_gutter != connection_gutter) {
		return;
	}

	String method = code_editor->get_text_editor()->get_line_gutter_metadata(p_line, p_gutter);
	if (method.is_empty()) {
		return;
	}

	Node *base = get_tree()->get_edited_scene_root();
	if (!base) {
		return;
	}

	Vector<Node *> nodes = _find_all_node_for_script(base, base, script);
	connection_info_dialog->popup_connections(method, nodes);
}

void ScriptTextEditor::_edit_option(int p_op) {
	CodeEdit *tx = code_editor->get_text_editor();

	switch (p_op) {
		case EDIT_UNDO: {
			tx->undo();
			tx->call_deferred(SNAME("grab_focus"));
		} break;
		case EDIT_REDO: {
			tx->redo();
			tx->call_deferred(SNAME("grab_focus"));
		} break;
		case EDIT_CUT: {
			tx->cut();
			tx->call_deferred(SNAME("grab_focus"));
		} break;
		case EDIT_COPY: {
			tx->copy();
			tx->call_deferred(SNAME("grab_focus"));
		} break;
		case EDIT_PASTE: {
			tx->paste();
			tx->call_deferred(SNAME("grab_focus"));
		} break;
		case EDIT_SELECT_ALL: {
			tx->select_all();
			tx->call_deferred(SNAME("grab_focus"));
		} break;
		case EDIT_MOVE_LINE_UP: {
			code_editor->move_lines_up();
		} break;
		case EDIT_MOVE_LINE_DOWN: {
			code_editor->move_lines_down();
		} break;
		case EDIT_INDENT_LEFT: {
			Ref<Script> scr = script;
			if (scr.is_null()) {
				return;
			}

			tx->unindent_lines();
		} break;
		case EDIT_INDENT_RIGHT: {
			Ref<Script> scr = script;
			if (scr.is_null()) {
				return;
			}

			tx->indent_lines();
		} break;
		case EDIT_DELETE_LINE: {
			code_editor->delete_lines();
		} break;
		case EDIT_DUPLICATE_SELECTION: {
			code_editor->duplicate_selection();
		} break;
		case EDIT_TOGGLE_FOLD_LINE: {
			tx->toggle_foldable_line(tx->get_caret_line());
			tx->queue_redraw();
		} break;
		case EDIT_FOLD_ALL_LINES: {
			tx->fold_all_lines();
			tx->queue_redraw();
		} break;
		case EDIT_UNFOLD_ALL_LINES: {
			tx->unfold_all_lines();
			tx->queue_redraw();
		} break;
		case EDIT_TOGGLE_COMMENT: {
			_edit_option_toggle_inline_comment();
		} break;
		case EDIT_COMPLETE: {
			tx->request_code_completion(true);
		} break;
		case EDIT_AUTO_INDENT: {
			String text = tx->get_text();
			Ref<Script> scr = script;
			if (scr.is_null()) {
				return;
			}

			tx->begin_complex_operation();
			int begin, end;
			if (tx->has_selection()) {
				begin = tx->get_selection_from_line();
				end = tx->get_selection_to_line();
				// ignore if the cursor is not past the first column
				if (tx->get_selection_to_column() == 0) {
					end--;
				}
			} else {
				begin = 0;
				end = tx->get_line_count() - 1;
			}
			scr->get_language()->auto_indent_code(text, begin, end);
			Vector<String> lines = text.split("\n");
			for (int i = begin; i <= end; ++i) {
				tx->set_line(i, lines[i]);
			}

			tx->end_complex_operation();
		} break;
		case EDIT_TRIM_TRAILING_WHITESAPCE: {
			trim_trailing_whitespace();
		} break;
		case EDIT_CONVERT_INDENT_TO_SPACES: {
			convert_indent_to_spaces();
		} break;
		case EDIT_CONVERT_INDENT_TO_TABS: {
			convert_indent_to_tabs();
		} break;
		case EDIT_PICK_COLOR: {
			color_panel->popup();
		} break;
		case EDIT_TO_UPPERCASE: {
			_convert_case(CodeTextEditor::UPPER);
		} break;
		case EDIT_TO_LOWERCASE: {
			_convert_case(CodeTextEditor::LOWER);
		} break;
		case EDIT_CAPITALIZE: {
			_convert_case(CodeTextEditor::CAPITALIZE);
		} break;
		case EDIT_EVALUATE: {
			Expression expression;
			Vector<String> lines = code_editor->get_text_editor()->get_selected_text().split("\n");
			PackedStringArray results;

			for (int i = 0; i < lines.size(); i++) {
				String line = lines[i];
				String whitespace = line.substr(0, line.size() - line.strip_edges(true, false).size()); //extract the whitespace at the beginning

				if (expression.parse(line) == OK) {
					Variant result = expression.execute(Array(), Variant(), false, true);
					if (expression.get_error_text().is_empty()) {
						results.push_back(whitespace + result.get_construct_string());
					} else {
						results.push_back(line);
					}
				} else {
					results.push_back(line);
				}
			}

			code_editor->get_text_editor()->begin_complex_operation(); //prevents creating a two-step undo
			code_editor->get_text_editor()->insert_text_at_caret(String("\n").join(results));
			code_editor->get_text_editor()->end_complex_operation();
		} break;
		case SEARCH_FIND: {
			code_editor->get_find_replace_bar()->popup_search();
		} break;
		case SEARCH_FIND_NEXT: {
			code_editor->get_find_replace_bar()->search_next();
		} break;
		case SEARCH_FIND_PREV: {
			code_editor->get_find_replace_bar()->search_prev();
		} break;
		case SEARCH_REPLACE: {
			code_editor->get_find_replace_bar()->popup_replace();
		} break;
		case SEARCH_IN_FILES: {
			String selected_text = code_editor->get_text_editor()->get_selected_text();

			// Yep, because it doesn't make sense to instance this dialog for every single script open...
			// So this will be delegated to the ScriptEditor.
			emit_signal(SNAME("search_in_files_requested"), selected_text);
		} break;
		case REPLACE_IN_FILES: {
			String selected_text = code_editor->get_text_editor()->get_selected_text();

			emit_signal(SNAME("replace_in_files_requested"), selected_text);
		} break;
		case SEARCH_LOCATE_FUNCTION: {
			quick_open->popup_dialog(get_functions());
			quick_open->set_title(TTR("Go to Function"));
		} break;
		case SEARCH_GOTO_LINE: {
			goto_line_dialog->popup_find_line(tx);
		} break;
		case BOOKMARK_TOGGLE: {
			code_editor->toggle_bookmark();
		} break;
		case BOOKMARK_GOTO_NEXT: {
			code_editor->goto_next_bookmark();
		} break;
		case BOOKMARK_GOTO_PREV: {
			code_editor->goto_prev_bookmark();
		} break;
		case BOOKMARK_REMOVE_ALL: {
			code_editor->remove_all_bookmarks();
		} break;
		case DEBUG_TOGGLE_BREAKPOINT: {
			int line = tx->get_caret_line();
			bool dobreak = !tx->is_line_breakpointed(line);
			tx->set_line_as_breakpoint(line, dobreak);
			EditorDebuggerNode::get_singleton()->set_breakpoint(script->get_path(), line + 1, dobreak);
		} break;
		case DEBUG_REMOVE_ALL_BREAKPOINTS: {
			PackedInt32Array bpoints = tx->get_breakpointed_lines();

			for (int i = 0; i < bpoints.size(); i++) {
				int line = bpoints[i];
				bool dobreak = !tx->is_line_breakpointed(line);
				tx->set_line_as_breakpoint(line, dobreak);
				EditorDebuggerNode::get_singleton()->set_breakpoint(script->get_path(), line + 1, dobreak);
			}
		} break;
		case DEBUG_GOTO_NEXT_BREAKPOINT: {
			PackedInt32Array bpoints = tx->get_breakpointed_lines();
			if (bpoints.size() <= 0) {
				return;
			}

			int line = tx->get_caret_line();

			// wrap around
			if (line >= (int)bpoints[bpoints.size() - 1]) {
				tx->unfold_line(bpoints[0]);
				tx->set_caret_line(bpoints[0]);
				tx->center_viewport_to_caret();
			} else {
				for (int i = 0; i < bpoints.size(); i++) {
					int bline = bpoints[i];
					if (bline > line) {
						tx->unfold_line(bline);
						tx->set_caret_line(bline);
						tx->center_viewport_to_caret();
						return;
					}
				}
			}

		} break;
		case DEBUG_GOTO_PREV_BREAKPOINT: {
			PackedInt32Array bpoints = tx->get_breakpointed_lines();
			if (bpoints.size() <= 0) {
				return;
			}

			int line = tx->get_caret_line();
			// wrap around
			if (line <= (int)bpoints[0]) {
				tx->unfold_line(bpoints[bpoints.size() - 1]);
				tx->set_caret_line(bpoints[bpoints.size() - 1]);
				tx->center_viewport_to_caret();
			} else {
				for (int i = bpoints.size() - 1; i >= 0; i--) {
					int bline = bpoints[i];
					if (bline < line) {
						tx->unfold_line(bline);
						tx->set_caret_line(bline);
						tx->center_viewport_to_caret();
						return;
					}
				}
			}

		} break;
		case HELP_CONTEXTUAL: {
			String text = tx->get_selected_text();
			if (text.is_empty()) {
				text = tx->get_word_under_caret();
			}
			if (!text.is_empty()) {
				emit_signal(SNAME("request_help"), text);
			}
		} break;
		case LOOKUP_SYMBOL: {
			String text = tx->get_word_under_caret();
			if (text.is_empty()) {
				text = tx->get_selected_text();
			}
			if (!text.is_empty()) {
				_lookup_symbol(text, tx->get_caret_line(), tx->get_caret_column());
			}
		} break;
	}
}

void ScriptTextEditor::_edit_option_toggle_inline_comment() {
	if (script.is_null()) {
		return;
	}

	String delimiter = "#";
	List<String> comment_delimiters;
	script->get_language()->get_comment_delimiters(&comment_delimiters);

	for (const String &script_delimiter : comment_delimiters) {
		if (!script_delimiter.contains(" ")) {
			delimiter = script_delimiter;
			break;
		}
	}

	code_editor->toggle_inline_comment(delimiter);
}

void ScriptTextEditor::add_syntax_highlighter(Ref<EditorSyntaxHighlighter> p_highlighter) {
	ERR_FAIL_COND(p_highlighter.is_null());

	highlighters[p_highlighter->_get_name()] = p_highlighter;
	highlighter_menu->add_radio_check_item(p_highlighter->_get_name());
}

void ScriptTextEditor::set_syntax_highlighter(Ref<EditorSyntaxHighlighter> p_highlighter) {
	ERR_FAIL_COND(p_highlighter.is_null());

	HashMap<String, Ref<EditorSyntaxHighlighter>>::Iterator el = highlighters.begin();
	while (el) {
		int highlighter_index = highlighter_menu->get_item_idx_from_text(el->key);
		highlighter_menu->set_item_checked(highlighter_index, el->value == p_highlighter);
		++el;
	}

	CodeEdit *te = code_editor->get_text_editor();
	p_highlighter->_set_edited_resource(script);
	te->set_syntax_highlighter(p_highlighter);
}

void ScriptTextEditor::_change_syntax_highlighter(int p_idx) {
	set_syntax_highlighter(highlighters[highlighter_menu->get_item_text(p_idx)]);
}

void ScriptTextEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED:
			if (!editor_enabled) {
				break;
			}
			if (is_visible_in_tree()) {
				_update_warnings();
				_update_errors();
			}
			[[fallthrough]];
		case NOTIFICATION_ENTER_TREE: {
			code_editor->get_text_editor()->set_gutter_width(connection_gutter, code_editor->get_text_editor()->get_line_height());
		} break;
	}
}

void ScriptTextEditor::_bind_methods() {
	ClassDB::bind_method("_update_connected_methods", &ScriptTextEditor::_update_connected_methods);

	ClassDB::bind_method("_get_drag_data_fw", &ScriptTextEditor::get_drag_data_fw);
	ClassDB::bind_method("_can_drop_data_fw", &ScriptTextEditor::can_drop_data_fw);
	ClassDB::bind_method("_drop_data_fw", &ScriptTextEditor::drop_data_fw);
}

Control *ScriptTextEditor::get_edit_menu() {
	return edit_hb;
}

void ScriptTextEditor::clear_edit_menu() {
	if (editor_enabled) {
		memdelete(edit_hb);
	}
}

void ScriptTextEditor::set_find_replace_bar(FindReplaceBar *p_bar) {
	code_editor->set_find_replace_bar(p_bar);
}

void ScriptTextEditor::reload(bool p_soft) {
	CodeEdit *te = code_editor->get_text_editor();
	Ref<Script> scr = script;
	if (scr.is_null()) {
		return;
	}
	scr->set_source_code(te->get_text());
	bool soft = p_soft || scr->get_instance_base_type() == "EditorPlugin"; // Always soft-reload editor plugins.

	scr->get_language()->reload_tool_script(scr, soft);
}

PackedInt32Array ScriptTextEditor::get_breakpoints() {
	return code_editor->get_text_editor()->get_breakpointed_lines();
}

void ScriptTextEditor::set_breakpoint(int p_line, bool p_enabled) {
	code_editor->get_text_editor()->set_line_as_breakpoint(p_line, p_enabled);
}

void ScriptTextEditor::clear_breakpoints() {
	code_editor->get_text_editor()->clear_breakpointed_lines();
}

void ScriptTextEditor::set_tooltip_request_func(const Callable &p_toolip_callback) {
	Variant args[1] = { this };
	const Variant *argp[] = { &args[0] };
	code_editor->get_text_editor()->set_tooltip_request_func(p_toolip_callback.bindp(argp, 1));
}

void ScriptTextEditor::set_debugger_active(bool p_active) {
}

Control *ScriptTextEditor::get_base_editor() const {
	return code_editor->get_text_editor();
}

Variant ScriptTextEditor::get_drag_data_fw(const Point2 &p_point, Control *p_from) {
	return Variant();
}

bool ScriptTextEditor::can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const {
	Dictionary d = p_data;
	if (d.has("type") &&
			(String(d["type"]) == "resource" ||
					String(d["type"]) == "files" ||
					String(d["type"]) == "nodes" ||
					String(d["type"]) == "obj_property" ||
					String(d["type"]) == "files_and_dirs")) {
		return true;
	}

	return false;
}

static Node *_find_script_node(Node *p_edited_scene, Node *p_current_node, const Ref<Script> &script) {
	// Check scripts only for the nodes belonging to the edited scene.
	if (p_current_node == p_edited_scene || p_current_node->get_owner() == p_edited_scene) {
		Ref<Script> scr = p_current_node->get_script();
		if (scr.is_valid() && scr == script) {
			return p_current_node;
		}
	}

	// Traverse all children, even the ones not owned by the edited scene as they
	// can still have child nodes added within the edited scene and thus owned by
	// it (e.g. nodes added to subscene's root or to its editable children).
	for (int i = 0; i < p_current_node->get_child_count(); i++) {
		Node *n = _find_script_node(p_edited_scene, p_current_node->get_child(i), script);
		if (n) {
			return n;
		}
	}

	return nullptr;
}

void ScriptTextEditor::drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	const String quote_style = EDITOR_GET("text_editor/completion/use_single_quotes") ? "'" : "\"";

	Dictionary d = p_data;

	CodeEdit *te = code_editor->get_text_editor();
	Point2i pos = te->get_line_column_at_pos(p_point);
	int row = pos.y;
	int col = pos.x;

	if (d.has("type") && String(d["type"]) == "resource") {
		Ref<Resource> res = d["resource"];
		if (!res.is_valid()) {
			return;
		}

		if (res->get_path().is_resource_file()) {
			EditorNode::get_singleton()->show_warning(TTR("Only resources from filesystem can be dropped."));
			return;
		}

		te->set_caret_line(row);
		te->set_caret_column(col);
		te->insert_text_at_caret(res->get_path());
		te->grab_focus();
	}

	if (d.has("type") && (String(d["type"]) == "files" || String(d["type"]) == "files_and_dirs")) {
		Array files = d["files"];

		String text_to_drop;
		bool preload = Input::get_singleton()->is_key_pressed(Key::CTRL);
		for (int i = 0; i < files.size(); i++) {
			if (i > 0) {
				text_to_drop += ", ";
			}

			if (preload) {
				text_to_drop += "preload(" + String(files[i]).c_escape().quote(quote_style) + ")";
			} else {
				text_to_drop += String(files[i]).c_escape().quote(quote_style);
			}
		}

		te->set_caret_line(row);
		te->set_caret_column(col);
		te->insert_text_at_caret(text_to_drop);
		te->grab_focus();
	}

	if (d.has("type") && String(d["type"]) == "nodes") {
		Node *scene_root = get_tree()->get_edited_scene_root();
		if (!scene_root) {
			EditorNode::get_singleton()->show_warning(TTR("Can't drop nodes without an open scene."));
			return;
		}

		Node *sn = _find_script_node(scene_root, scene_root, script);
		if (!sn) {
			EditorNode::get_singleton()->show_warning(vformat(TTR("Can't drop nodes because script '%s' is not used in this scene."), get_name()));
			return;
		}

		Array nodes = d["nodes"];
		String text_to_drop;

		if (Input::get_singleton()->is_key_pressed(Key::CTRL)) {
			bool use_type = EDITOR_GET("text_editor/completion/add_type_hints");
			for (int i = 0; i < nodes.size(); i++) {
				NodePath np = nodes[i];
				Node *node = get_node(np);
				if (!node) {
					continue;
				}

				bool is_unique = false;
				String path;
				if (node->is_unique_name_in_owner()) {
					path = node->get_name();
					is_unique = true;
				} else {
					path = sn->get_path_to(node);
				}
				for (const String &segment : path.split("/")) {
					if (!segment.is_valid_identifier()) {
						path = path.c_escape().quote(quote_style);
						break;
					}
				}

				String variable_name = String(node->get_name()).to_snake_case().validate_identifier();
				if (use_type) {
					text_to_drop += vformat("@onready var %s: %s = %s%s\n", variable_name, node->get_class_name(), is_unique ? "%" : "$", path);
				} else {
					text_to_drop += vformat("@onready var %s = %s%s\n", variable_name, is_unique ? "%" : "$", path);
				}
			}
		} else {
			for (int i = 0; i < nodes.size(); i++) {
				if (i > 0) {
					text_to_drop += ", ";
				}

				NodePath np = nodes[i];
				Node *node = get_node(np);
				if (!node) {
					continue;
				}

				bool is_unique = false;
				String path;
				if (node->is_unique_name_in_owner()) {
					path = node->get_name();
					is_unique = true;
				} else {
					path = sn->get_path_to(node);
				}

				for (const String &segment : path.split("/")) {
					if (!segment.is_valid_identifier()) {
						path = path.c_escape().quote(quote_style);
						break;
					}
				}
				text_to_drop += (is_unique ? "%" : "$") + path;
			}
		}

		te->set_caret_line(row);
		te->set_caret_column(col);
		te->insert_text_at_caret(text_to_drop);
		te->grab_focus();
	}

	if (d.has("type") && String(d["type"]) == "obj_property") {
		const String text_to_drop = String(d["property"]).c_escape().quote(quote_style);

		te->set_caret_line(row);
		te->set_caret_column(col);
		te->insert_text_at_caret(text_to_drop);
		te->grab_focus();
	}
}

void ScriptTextEditor::_text_edit_gui_input(const Ref<InputEvent> &ev) {
	Ref<InputEventMouseButton> mb = ev;
	Ref<InputEventKey> k = ev;
	Point2 local_pos;
	bool create_menu = false;

	CodeEdit *tx = code_editor->get_text_editor();
	if (mb.is_valid() && mb->get_button_index() == MouseButton::RIGHT && mb->is_pressed()) {
		local_pos = mb->get_global_position() - tx->get_global_position();
		create_menu = true;
	} else if (k.is_valid() && k->is_action("ui_menu", true)) {
		tx->adjust_viewport_to_caret();
		local_pos = tx->get_caret_draw_pos();
		create_menu = true;
	}

	if (create_menu) {
		Point2i pos = tx->get_line_column_at_pos(local_pos);
		int row = pos.y;
		int col = pos.x;

		tx->set_move_caret_on_right_click_enabled(EditorSettings::get_singleton()->get("text_editor/behavior/navigation/move_caret_on_right_click"));
		if (tx->is_move_caret_on_right_click_enabled()) {
			if (tx->has_selection()) {
				int from_line = tx->get_selection_from_line();
				int to_line = tx->get_selection_to_line();
				int from_column = tx->get_selection_from_column();
				int to_column = tx->get_selection_to_column();

				if (row < from_line || row > to_line || (row == from_line && col < from_column) || (row == to_line && col > to_column)) {
					// Right click is outside the selected text
					tx->deselect();
				}
			}
			if (!tx->has_selection()) {
				tx->set_caret_line(row, false, false);
				tx->set_caret_column(col);
			}
		}

		String word_at_pos = tx->get_word_at_pos(local_pos);
		if (word_at_pos.is_empty()) {
			word_at_pos = tx->get_word_under_caret();
		}
		if (word_at_pos.is_empty()) {
			word_at_pos = tx->get_selected_text();
		}

		bool has_color = (word_at_pos == "Color");
		bool foldable = tx->can_fold_line(row) || tx->is_line_folded(row);
		bool open_docs = false;
		bool goto_definition = false;

		if (word_at_pos.is_resource_file()) {
			open_docs = true;
		} else {
			Node *base = get_tree()->get_edited_scene_root();
			if (base) {
				base = _find_node_for_script(base, base, script);
			}
			ScriptLanguage::LookupResult result;
			if (script->get_language()->lookup_code(code_editor->get_text_editor()->get_text_for_symbol_lookup(), word_at_pos, script->get_path(), base, result) == OK) {
				open_docs = true;
			}
		}

		if (has_color) {
			String line = tx->get_line(row);
			color_position.x = row;
			color_position.y = col;

			int begin = 0;
			int end = 0;
			bool valid = false;
			for (int i = col; i < line.length(); i++) {
				if (line[i] == '(') {
					begin = i;
					continue;
				} else if (line[i] == ')') {
					end = i + 1;
					valid = true;
					break;
				}
			}
			if (valid) {
				color_args = line.substr(begin, end - begin);
				String stripped = color_args.replace(" ", "").replace("(", "").replace(")", "");
				Vector<float> color = stripped.split_floats(",");
				if (color.size() > 2) {
					float alpha = color.size() > 3 ? color[3] : 1.0f;
					color_picker->set_pick_color(Color(color[0], color[1], color[2], alpha));
				}
				color_panel->set_position(get_screen_position() + local_pos);
			} else {
				has_color = false;
			}
		}
		_make_context_menu(tx->has_selection(), has_color, foldable, open_docs, goto_definition, local_pos);
	}
}

void ScriptTextEditor::_color_changed(const Color &p_color) {
	String new_args;
	if (p_color.a == 1.0f) {
		new_args = String("(" + rtos(p_color.r) + ", " + rtos(p_color.g) + ", " + rtos(p_color.b) + ")");
	} else {
		new_args = String("(" + rtos(p_color.r) + ", " + rtos(p_color.g) + ", " + rtos(p_color.b) + ", " + rtos(p_color.a) + ")");
	}

	String line = code_editor->get_text_editor()->get_line(color_position.x);
	String line_with_replaced_args = line.replace(color_args, new_args);

	color_args = new_args;
	code_editor->get_text_editor()->begin_complex_operation();
	code_editor->get_text_editor()->set_line(color_position.x, line_with_replaced_args);
	code_editor->get_text_editor()->end_complex_operation();
	code_editor->get_text_editor()->queue_redraw();
}

void ScriptTextEditor::_prepare_edit_menu() {
	const CodeEdit *tx = code_editor->get_text_editor();
	PopupMenu *popup = edit_menu->get_popup();
	popup->set_item_disabled(popup->get_item_index(EDIT_UNDO), !tx->has_undo());
	popup->set_item_disabled(popup->get_item_index(EDIT_REDO), !tx->has_redo());
}

void ScriptTextEditor::_make_context_menu(bool p_selection, bool p_color, bool p_foldable, bool p_open_docs, bool p_goto_definition, Vector2 p_pos) {
	context_menu->clear();
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_undo"), EDIT_UNDO);
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_redo"), EDIT_REDO);

	context_menu->add_separator();
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_cut"), EDIT_CUT);
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_copy"), EDIT_COPY);
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_paste"), EDIT_PASTE);

	context_menu->add_separator();
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_text_select_all"), EDIT_SELECT_ALL);

	context_menu->add_separator();
	context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent_left"), EDIT_INDENT_LEFT);
	context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent_right"), EDIT_INDENT_RIGHT);
	context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_comment"), EDIT_TOGGLE_COMMENT);
	context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_bookmark"), BOOKMARK_TOGGLE);

	if (p_selection) {
		context_menu->add_separator();
		context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/convert_to_uppercase"), EDIT_TO_UPPERCASE);
		context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/convert_to_lowercase"), EDIT_TO_LOWERCASE);
		context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/evaluate_selection"), EDIT_EVALUATE);
	}
	if (p_foldable) {
		context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_fold_line"), EDIT_TOGGLE_FOLD_LINE);
	}

	if (p_color || p_open_docs || p_goto_definition) {
		context_menu->add_separator();
		if (p_open_docs) {
			context_menu->add_item(TTR("Lookup Symbol"), LOOKUP_SYMBOL);
		}
		if (p_color) {
			context_menu->add_item(TTR("Pick Color"), EDIT_PICK_COLOR);
		}
	}

	const CodeEdit *tx = code_editor->get_text_editor();
	context_menu->set_item_disabled(context_menu->get_item_index(EDIT_UNDO), !tx->has_undo());
	context_menu->set_item_disabled(context_menu->get_item_index(EDIT_REDO), !tx->has_redo());

	context_menu->set_position(get_screen_position() + p_pos);
	context_menu->reset_size();
	context_menu->popup();
}

void ScriptTextEditor::_enable_code_editor() {
	ERR_FAIL_COND(code_editor->get_parent());

	VSplitContainer *editor_box = memnew(VSplitContainer);
	add_child(editor_box);
	editor_box->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	editor_box->set_v_size_flags(SIZE_EXPAND_FILL);

	editor_box->add_child(code_editor);
	code_editor->connect("show_errors_panel", callable_mp(this, &ScriptTextEditor::_show_errors_panel));
	code_editor->connect("show_warnings_panel", callable_mp(this, &ScriptTextEditor::_show_warnings_panel));
	code_editor->connect("validate_script", callable_mp(this, &ScriptTextEditor::_validate_script));
	code_editor->connect("load_theme_settings", callable_mp(this, &ScriptTextEditor::_load_theme_settings));
	code_editor->get_text_editor()->connect("symbol_lookup", callable_mp(this, &ScriptTextEditor::_lookup_symbol));
	code_editor->get_text_editor()->connect("symbol_validate", callable_mp(this, &ScriptTextEditor::_validate_symbol));
	code_editor->get_text_editor()->connect("symbol_hovered", callable_mp(this, &ScriptTextEditor::_hovered_symbol));
	code_editor->get_text_editor()->connect("gutter_added", callable_mp(this, &ScriptTextEditor::_update_gutter_indexes));
	code_editor->get_text_editor()->connect("gutter_removed", callable_mp(this, &ScriptTextEditor::_update_gutter_indexes));
	code_editor->get_text_editor()->connect("gutter_clicked", callable_mp(this, &ScriptTextEditor::_gutter_clicked));
	code_editor->get_text_editor()->connect("gui_input", callable_mp(this, &ScriptTextEditor::_text_edit_gui_input));
	code_editor->show_toggle_scripts_button();
	_update_gutter_indexes();

	editor_box->add_child(warnings_panel);
	warnings_panel->add_theme_font_override(
			"normal_font", EditorNode::get_singleton()->get_gui_base()->get_theme_font(SNAME("main"), SNAME("EditorFonts")));
	warnings_panel->add_theme_font_size_override(
			"normal_font_size", EditorNode::get_singleton()->get_gui_base()->get_theme_font_size(SNAME("main_size"), SNAME("EditorFonts")));
	warnings_panel->connect("meta_clicked", callable_mp(this, &ScriptTextEditor::_warning_clicked));

	editor_box->add_child(errors_panel);
	errors_panel->add_theme_font_override(
			"normal_font", EditorNode::get_singleton()->get_gui_base()->get_theme_font(SNAME("main"), SNAME("EditorFonts")));
	errors_panel->add_theme_font_size_override(
			"normal_font_size", EditorNode::get_singleton()->get_gui_base()->get_theme_font_size(SNAME("main_size"), SNAME("EditorFonts")));
	errors_panel->connect("meta_clicked", callable_mp(this, &ScriptTextEditor::_error_clicked));

	add_child(context_menu);
	context_menu->connect("id_pressed", callable_mp(this, &ScriptTextEditor::_edit_option));

	add_child(color_panel);

	color_picker = memnew(ColorPicker);
	color_picker->set_deferred_mode(true);
	color_picker->connect("color_changed", callable_mp(this, &ScriptTextEditor::_color_changed));
	color_panel->connect("about_to_popup", callable_mp(EditorNode::get_singleton(), &EditorNode::setup_color_picker).bind(color_picker));

	color_panel->add_child(color_picker);

	quick_open = memnew(ScriptEditorQuickOpen);
	quick_open->connect("goto_line", callable_mp(this, &ScriptTextEditor::_goto_line));
	add_child(quick_open);

	goto_line_dialog = memnew(GotoLineDialog);
	add_child(goto_line_dialog);

	add_child(connection_info_dialog);

	edit_hb->add_child(search_menu);
	search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/find"), SEARCH_FIND);
	search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/find_next"), SEARCH_FIND_NEXT);
	search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/find_previous"), SEARCH_FIND_PREV);
	search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/replace"), SEARCH_REPLACE);
	search_menu->get_popup()->add_separator();
	search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/find_in_files"), SEARCH_IN_FILES);
	search_menu->get_popup()->add_separator();
	search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/contextual_help"), HELP_CONTEXTUAL);
	search_menu->get_popup()->connect("id_pressed", callable_mp(this, &ScriptTextEditor::_edit_option));

	edit_hb->add_child(edit_menu);
	edit_menu->connect("about_to_popup", callable_mp(this, &ScriptTextEditor::_prepare_edit_menu));
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("ui_undo"), EDIT_UNDO);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("ui_redo"), EDIT_REDO);
	edit_menu->get_popup()->add_separator();
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("ui_cut"), EDIT_CUT);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("ui_copy"), EDIT_COPY);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("ui_paste"), EDIT_PASTE);
	edit_menu->get_popup()->add_separator();
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("ui_text_select_all"), EDIT_SELECT_ALL);
	edit_menu->get_popup()->add_separator();
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/move_up"), EDIT_MOVE_LINE_UP);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/move_down"), EDIT_MOVE_LINE_DOWN);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent_left"), EDIT_INDENT_LEFT);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent_right"), EDIT_INDENT_RIGHT);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/delete_line"), EDIT_DELETE_LINE);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_comment"), EDIT_TOGGLE_COMMENT);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_fold_line"), EDIT_TOGGLE_FOLD_LINE);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/fold_all_lines"), EDIT_FOLD_ALL_LINES);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/unfold_all_lines"), EDIT_UNFOLD_ALL_LINES);
	edit_menu->get_popup()->add_separator();
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/duplicate_selection"), EDIT_DUPLICATE_SELECTION);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("ui_text_completion_query"), EDIT_COMPLETE);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/evaluate_selection"), EDIT_EVALUATE);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/trim_trailing_whitespace"), EDIT_TRIM_TRAILING_WHITESAPCE);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/convert_indent_to_spaces"), EDIT_CONVERT_INDENT_TO_SPACES);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/convert_indent_to_tabs"), EDIT_CONVERT_INDENT_TO_TABS);
	edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/auto_indent"), EDIT_AUTO_INDENT);
	edit_menu->get_popup()->connect("id_pressed", callable_mp(this, &ScriptTextEditor::_edit_option));
	edit_menu->get_popup()->add_separator();

	edit_menu->get_popup()->add_child(convert_case);
	edit_menu->get_popup()->add_submenu_item(TTR("Convert Case"), "convert_case");
	convert_case->add_shortcut(ED_SHORTCUT("script_text_editor/convert_to_uppercase", TTR("Uppercase"), KeyModifierMask::SHIFT | Key::F4), EDIT_TO_UPPERCASE);
	convert_case->add_shortcut(ED_SHORTCUT("script_text_editor/convert_to_lowercase", TTR("Lowercase"), KeyModifierMask::SHIFT | Key::F5), EDIT_TO_LOWERCASE);
	convert_case->add_shortcut(ED_SHORTCUT("script_text_editor/capitalize", TTR("Capitalize"), KeyModifierMask::SHIFT | Key::F6), EDIT_CAPITALIZE);
	convert_case->connect("id_pressed", callable_mp(this, &ScriptTextEditor::_edit_option));

	edit_menu->get_popup()->add_child(highlighter_menu);
	edit_menu->get_popup()->add_submenu_item(TTR("Syntax Highlighter"), "highlighter_menu");
	highlighter_menu->connect("id_pressed", callable_mp(this, &ScriptTextEditor::_change_syntax_highlighter));

	_load_theme_settings();

	search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/replace_in_files"), REPLACE_IN_FILES);
	edit_hb->add_child(goto_menu);
	goto_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/goto_function"), SEARCH_LOCATE_FUNCTION);
	goto_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/goto_line"), SEARCH_GOTO_LINE);
	goto_menu->get_popup()->add_separator();

	goto_menu->get_popup()->add_child(bookmarks_menu);
	goto_menu->get_popup()->add_submenu_item(TTR("Bookmarks"), "Bookmarks");
	_update_bookmark_list();
	bookmarks_menu->connect("about_to_popup", callable_mp(this, &ScriptTextEditor::_update_bookmark_list));
	bookmarks_menu->connect("index_pressed", callable_mp(this, &ScriptTextEditor::_bookmark_item_pressed));

	goto_menu->get_popup()->add_child(breakpoints_menu);
	goto_menu->get_popup()->add_submenu_item(TTR("Breakpoints"), "Breakpoints");
	_update_breakpoint_list();
	breakpoints_menu->connect("about_to_popup", callable_mp(this, &ScriptTextEditor::_update_breakpoint_list));
	breakpoints_menu->connect("index_pressed", callable_mp(this, &ScriptTextEditor::_breakpoint_item_pressed));

	goto_menu->get_popup()->connect("id_pressed", callable_mp(this, &ScriptTextEditor::_edit_option));

	add_child(symbol_hint_popup);

	symbol_hint_help_bit = memnew(SymbolHintHelpBit);
	symbol_hint_popup->add_child(symbol_hint_help_bit);
	symbol_hint_help_bit->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	symbol_hint_help_bit->connect(SNAME("request_hide"), callable_mp(this, &ScriptTextEditor::_hide_symbol_hint));
}

ScriptTextEditor::ScriptTextEditor() {
	code_editor = memnew(CodeTextEditor);
	code_editor->add_theme_constant_override("separation", 2);
	code_editor->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	code_editor->set_code_complete_func(_code_complete_scripts, this);
	code_editor->set_v_size_flags(SIZE_EXPAND_FILL);

	code_editor->get_text_editor()->set_draw_breakpoints_gutter(true);
	code_editor->get_text_editor()->set_draw_executing_lines_gutter(true);
	code_editor->get_text_editor()->connect("breakpoint_toggled", callable_mp(this, &ScriptTextEditor::_breakpoint_toggled));

	connection_gutter = 1;
	code_editor->get_text_editor()->add_gutter(connection_gutter);
	code_editor->get_text_editor()->set_gutter_name(connection_gutter, "connection_gutter");
	code_editor->get_text_editor()->set_gutter_draw(connection_gutter, false);
	code_editor->get_text_editor()->set_gutter_overwritable(connection_gutter, true);
	code_editor->get_text_editor()->set_gutter_type(connection_gutter, TextEdit::GUTTER_TYPE_ICON);

	warnings_panel = memnew(RichTextLabel);
	warnings_panel->set_custom_minimum_size(Size2(0, 100 * EDSCALE));
	warnings_panel->set_h_size_flags(SIZE_EXPAND_FILL);
	warnings_panel->set_meta_underline(true);
	warnings_panel->set_selection_enabled(true);
	warnings_panel->set_focus_mode(FOCUS_CLICK);
	warnings_panel->hide();

	errors_panel = memnew(RichTextLabel);
	errors_panel->set_custom_minimum_size(Size2(0, 100 * EDSCALE));
	errors_panel->set_h_size_flags(SIZE_EXPAND_FILL);
	errors_panel->set_meta_underline(true);
	errors_panel->set_selection_enabled(true);
	errors_panel->set_focus_mode(FOCUS_CLICK);
	errors_panel->hide();

	symbol_hint_popup = memnew(PopupPanel);
	symbol_hint_popup->set_theme_type_variation(SNAME("TooltipPanel"));
	symbol_hint_popup->set_transient(true);
	symbol_hint_popup->set_flag(Window::FLAG_NO_FOCUS, true);
	symbol_hint_popup->set_flag(Window::FLAG_POPUP, false);
	symbol_hint_popup->set_wrap_controls(true);

	update_settings();

	code_editor->get_text_editor()->set_code_hint_draw_below(EditorSettings::get_singleton()->get("text_editor/completion/put_callhint_tooltip_below_current_line"));

	code_editor->get_text_editor()->set_symbol_lookup_on_click_enabled(true);
	code_editor->get_text_editor()->set_context_menu_enabled(false);

	context_menu = memnew(PopupMenu);

	color_panel = memnew(PopupPanel);

	edit_hb = memnew(HBoxContainer);

	edit_menu = memnew(MenuButton);
	edit_menu->set_text(TTR("Edit"));
	edit_menu->set_switch_on_hover(true);
	edit_menu->set_shortcut_context(this);

	convert_case = memnew(PopupMenu);
	convert_case->set_name("convert_case");

	highlighter_menu = memnew(PopupMenu);
	highlighter_menu->set_name("highlighter_menu");

	Ref<EditorPlainTextSyntaxHighlighter> plain_highlighter;
	plain_highlighter.instantiate();
	add_syntax_highlighter(plain_highlighter);

	Ref<EditorStandardSyntaxHighlighter> highlighter;
	highlighter.instantiate();
	add_syntax_highlighter(highlighter);
	set_syntax_highlighter(highlighter);

	search_menu = memnew(MenuButton);
	search_menu->set_text(TTR("Search"));
	search_menu->set_switch_on_hover(true);
	search_menu->set_shortcut_context(this);

	goto_menu = memnew(MenuButton);
	goto_menu->set_text(TTR("Go To"));
	goto_menu->set_switch_on_hover(true);
	goto_menu->set_shortcut_context(this);

	bookmarks_menu = memnew(PopupMenu);
	bookmarks_menu->set_name("Bookmarks");

	breakpoints_menu = memnew(PopupMenu);
	breakpoints_menu->set_name("Breakpoints");

	connection_info_dialog = memnew(ConnectionInfoDialog);

	code_editor->get_text_editor()->set_drag_forwarding(this);
}

ScriptTextEditor::~ScriptTextEditor() {
	highlighters.clear();

	if (!editor_enabled) {
		memdelete(code_editor);
		memdelete(warnings_panel);
		memdelete(errors_panel);
		memdelete(context_menu);
		memdelete(color_panel);
		memdelete(edit_hb);
		memdelete(edit_menu);
		memdelete(convert_case);
		memdelete(highlighter_menu);
		memdelete(search_menu);
		memdelete(goto_menu);
		memdelete(bookmarks_menu);
		memdelete(breakpoints_menu);
		memdelete(connection_info_dialog);
		memdelete(symbol_hint_popup);
	}
}

static ScriptEditorBase *create_editor(const Ref<Resource> &p_resource) {
	if (Object::cast_to<Script>(*p_resource)) {
		return memnew(ScriptTextEditor);
	}
	return nullptr;
}

void ScriptTextEditor::register_editor() {
	ED_SHORTCUT("script_text_editor/move_up", TTR("Move Up"), KeyModifierMask::ALT | Key::UP);
	ED_SHORTCUT("script_text_editor/move_down", TTR("Move Down"), KeyModifierMask::ALT | Key::DOWN);
	ED_SHORTCUT("script_text_editor/delete_line", TTR("Delete Line"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::K);

	// Leave these at zero, same can be accomplished with tab/shift-tab, including selection.
	// The next/previous in history shortcut in this case makes a lot more sense.

	ED_SHORTCUT("script_text_editor/indent_left", TTR("Indent Left"), Key::NONE);
	ED_SHORTCUT("script_text_editor/indent_right", TTR("Indent Right"), Key::NONE);
	ED_SHORTCUT("script_text_editor/toggle_comment", TTR("Toggle Comment"), KeyModifierMask::CMD_OR_CTRL | Key::K);
	ED_SHORTCUT("script_text_editor/toggle_fold_line", TTR("Fold/Unfold Line"), KeyModifierMask::ALT | Key::F);
	ED_SHORTCUT("script_text_editor/fold_all_lines", TTR("Fold All Lines"), Key::NONE);
	ED_SHORTCUT("script_text_editor/unfold_all_lines", TTR("Unfold All Lines"), Key::NONE);
	ED_SHORTCUT("script_text_editor/duplicate_selection", TTR("Duplicate Selection"), KeyModifierMask::SHIFT | KeyModifierMask::CTRL | Key::D);
	ED_SHORTCUT_OVERRIDE("script_text_editor/duplicate_selection", "macos", KeyModifierMask::SHIFT | KeyModifierMask::META | Key::C);
	ED_SHORTCUT("script_text_editor/evaluate_selection", TTR("Evaluate Selection"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::E);
	ED_SHORTCUT("script_text_editor/trim_trailing_whitespace", TTR("Trim Trailing Whitespace"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::T);
	ED_SHORTCUT("script_text_editor/convert_indent_to_spaces", TTR("Convert Indent to Spaces"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::Y);
	ED_SHORTCUT("script_text_editor/convert_indent_to_tabs", TTR("Convert Indent to Tabs"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::I);
	ED_SHORTCUT("script_text_editor/auto_indent", TTR("Auto Indent"), KeyModifierMask::CMD_OR_CTRL | Key::I);

	ED_SHORTCUT_AND_COMMAND("script_text_editor/find", TTR("Find..."), KeyModifierMask::CMD_OR_CTRL | Key::F);

	ED_SHORTCUT("script_text_editor/find_next", TTR("Find Next"), Key::F3);
	ED_SHORTCUT_OVERRIDE("script_text_editor/find_next", "macos", KeyModifierMask::META | Key::G);

	ED_SHORTCUT("script_text_editor/find_previous", TTR("Find Previous"), KeyModifierMask::SHIFT | Key::F3);
	ED_SHORTCUT_OVERRIDE("script_text_editor/find_previous", "macos", KeyModifierMask::META | KeyModifierMask::SHIFT | Key::G);

	ED_SHORTCUT_AND_COMMAND("script_text_editor/replace", TTR("Replace..."), KeyModifierMask::CTRL | Key::R);
	ED_SHORTCUT_OVERRIDE("script_text_editor/replace", "macos", KeyModifierMask::ALT | KeyModifierMask::META | Key::F);

	ED_SHORTCUT("script_text_editor/find_in_files", TTR("Find in Files..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::F);
	ED_SHORTCUT("script_text_editor/replace_in_files", TTR("Replace in Files..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::R);

	ED_SHORTCUT("script_text_editor/contextual_help", TTR("Contextual Help"), KeyModifierMask::ALT | Key::F1);
	ED_SHORTCUT_OVERRIDE("script_text_editor/contextual_help", "macos", KeyModifierMask::ALT | KeyModifierMask::SHIFT | Key::SPACE);

	ED_SHORTCUT("script_text_editor/toggle_bookmark", TTR("Toggle Bookmark"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::B);
	ED_SHORTCUT("script_text_editor/goto_next_bookmark", TTR("Go to Next Bookmark"), KeyModifierMask::CMD_OR_CTRL | Key::B);
	ED_SHORTCUT("script_text_editor/goto_previous_bookmark", TTR("Go to Previous Bookmark"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::B);
	ED_SHORTCUT("script_text_editor/remove_all_bookmarks", TTR("Remove All Bookmarks"), Key::NONE);

	ED_SHORTCUT("script_text_editor/goto_function", TTR("Go to Function..."), KeyModifierMask::ALT | KeyModifierMask::CTRL | Key::F);
	ED_SHORTCUT_OVERRIDE("script_text_editor/goto_function", "macos", KeyModifierMask::CTRL | KeyModifierMask::META | Key::J);

	ED_SHORTCUT("script_text_editor/goto_line", TTR("Go to Line..."), KeyModifierMask::CMD_OR_CTRL | Key::L);

	ED_SHORTCUT("script_text_editor/toggle_breakpoint", TTR("Toggle Breakpoint"), Key::F9);
	ED_SHORTCUT_OVERRIDE("script_text_editor/toggle_breakpoint", "macos", KeyModifierMask::META | KeyModifierMask::SHIFT | Key::B);

	ED_SHORTCUT("script_text_editor/remove_all_breakpoints", TTR("Remove All Breakpoints"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::F9);
	ED_SHORTCUT("script_text_editor/goto_next_breakpoint", TTR("Go to Next Breakpoint"), KeyModifierMask::CMD_OR_CTRL | Key::PERIOD);
	ED_SHORTCUT("script_text_editor/goto_previous_breakpoint", TTR("Go to Previous Breakpoint"), KeyModifierMask::CMD_OR_CTRL | Key::COMMA);

	ScriptEditor::register_create_script_editor_function(create_editor);
}

void ScriptTextEditor::validate() {
	this->code_editor->validate_script();
}
