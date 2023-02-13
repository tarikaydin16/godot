/*************************************************************************/
/*  window_wrapper.h                                                     */
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

#ifndef WINDOW_WRAPPER_H
#define WINDOW_WRAPPER_H

#include "scene/gui/margin_container.h"
#include "scene/gui/menu_button.h"

class Window;

class WindowWrapper : public MarginContainer {
	GDCLASS(WindowWrapper, MarginContainer);

	Control *wrapped_control = nullptr;
	MarginContainer *margins = nullptr;
	Window *window = nullptr;

	Panel *window_background = nullptr;

	Ref<Shortcut> enable_shortcut;

	Rect2 _get_default_window_rect() const;
	Node *_get_wrapped_control_parent() const;

protected:
	static void _bind_methods();
	void _notification(int p_what);

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

public:
	void set_wrapped_control(Control *p_control, const Ref<Shortcut> &p_enable_shortcut = Ref<Shortcut>());
	Control *get_wrapped_control() const;
	Control *release_wrapped_control();

	bool is_window_available() const;

	void set_window_enabled(bool p_visible);
	bool get_window_enabled() const;

	Rect2i get_window_rect() const;
	int get_window_screen() const;

	void restore_window(const Rect2i &p_rect, int p_screen);
	void enable_window_on_screen(int p_screen, bool p_auto_scale = false);

	void set_window_title(const String p_title);
	void set_margins_enabled(bool p_enabled);

	WindowWrapper();
};

class ScreenSelect : public MenuButton {
	GDCLASS(ScreenSelect, MenuButton);

	void _build_advanced_menu();
	void _populate_screen_list(Container *p_container);

	void _emit_screen_signal(int p_screen_idx);
	void _handle_mouse_shortcut(const Ref<InputEvent> &p_event);

protected:
	static void _bind_methods();

	void _notification(int p_what);

public:
	ScreenSelect();
};

#endif // WINDOW_WRAPPER_H
