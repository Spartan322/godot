/*************************************************************************/
/*  rich_text_canvas.h                                                 */
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

#ifndef RICH_TEXT_CANVAS_H
#define RICH_TEXT_CANVAS_H

#include "core/object/ref_counted.h"
#include "scene/gui/rich_text_label.h"

class RichTextCanvas : public RefCounted {
	GDCLASS(RichTextCanvas, RefCounted);

	friend RichTextLabel;

	RichTextLabel *label = nullptr;

protected:
	void _call_input(Ref<InputEvent> p_event);
	void _call_process(double p_delta);
	void _call_draw();
	Size2 _call_get_size(Size2 p_requested);

	virtual void input(Ref<InputEvent> p_event);
	virtual void process(double p_delta);
	virtual void draw();
	virtual Size2 get_size(Size2 p_requested);

	GDVIRTUAL1(_input, Ref<InputEvent>);
	GDVIRTUAL1(_process, double);
	GDVIRTUAL0(_draw);
	GDVIRTUAL1R(Vector2, _get_size, Vector2);

	static void _bind_methods();

public:
	RichTextLabel *get_label() const;

	RichTextCanvas(RichTextLabel *p_label);
	~RichTextCanvas();
};

#endif // RICH_TEXT_CANVAS_H