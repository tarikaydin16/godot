/*************************************************************************/
/*  window_wrapper.cpp                                                   */
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

#include "window_wrapper.h"
#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "editor/editor_settings.h"
#include "scene/gui/box_container.h"
#include "scene/gui/panel.h"
#include "scene/main/window.h"

// WindowWrapper

// Capture all the shortcut event not handled by other nodes.
class ShortcutBin : public Node {
	GDCLASS(ShortcutBin, Node);

	virtual void _notification(int what) {
		switch (what) {
			case NOTIFICATION_READY:
				set_process_shortcut_input(true);
				break;
		}
	}

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override {
		Window *parent = Object::cast_to<Window>(get_viewport());
		ERR_FAIL_COND(!parent);

		if (!parent->is_visible()) {
			return;
		}

		// Get the window of the parent of the current window.
		Window *grandparent_window = Object::cast_to<Window>(parent->get_parent()->get_viewport());
		ERR_FAIL_COND(!grandparent_window);

		if (Object::cast_to<InputEventKey>(p_event.ptr()) || Object::cast_to<InputEventShortcut>(p_event.ptr())) {
			// HACK: Propagate the window input to the editor main window to handle global shortcuts.
			grandparent_window->push_unhandled_input(p_event);
			if (grandparent_window->is_input_handled()) {
				get_viewport()->set_input_as_handled();
			}
		}
	}
};

Rect2 WindowWrapper::_get_default_window_rect() const {
	// Assume that the control rect is the desidered one for the window.
	return wrapped_control->get_global_rect();
}

Node *WindowWrapper::_get_wrapped_control_parent() const {
	if (margins) {
		return margins;
	}
	return window;
}

void WindowWrapper::_bind_methods() {
	ADD_SIGNAL(MethodInfo("window_visibility_changed", PropertyInfo(Variant::BOOL, "visible")));
}

void WindowWrapper::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (get_window_enabled() && is_visible()) {
				window->grab_focus();
			}
		} break;
		case NOTIFICATION_READY: {
			set_process_input(true);
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			window_background->add_theme_style_override("panel", get_theme_stylebox("PanelForeground", "EditorStyles"));
		} break;
	}
}

void WindowWrapper::shortcut_input(const Ref<InputEvent> &p_event) {
	if (enable_shortcut.is_valid() && enable_shortcut->matches_event(p_event)) {
		set_window_enabled(true);
	}
}

void WindowWrapper::set_wrapped_control(Control *p_control, const Ref<Shortcut> &p_enable_shortcut) {
	ERR_FAIL_NULL(p_control);
	ERR_FAIL_COND(wrapped_control);

	wrapped_control = p_control;
	enable_shortcut = p_enable_shortcut;
	add_child(p_control);
}

Control *WindowWrapper::get_wrapped_control() const {
	return wrapped_control;
}

Control *WindowWrapper::release_wrapped_control() {
	set_window_enabled(false);
	if (wrapped_control) {
		Control *old_wrapped = wrapped_control;
		remove_child(wrapped_control);
		wrapped_control = nullptr;

		return old_wrapped;
	}
	return nullptr;
}

bool WindowWrapper::is_window_available() const {
	return window != nullptr;
}

void WindowWrapper::set_window_enabled(bool p_visible) {
	ERR_FAIL_NULL(wrapped_control);

	if (!is_window_available()) {
		return;
	}

	if (window->is_visible() == p_visible) {
		if (p_visible) {
			window->grab_focus();
		}
		return;
	}

	Node *parent = _get_wrapped_control_parent();

	window->set_visible(p_visible);
	if (wrapped_control->get_parent() != parent) {
		Rect2 control_rect = _get_default_window_rect();

		// Move the control to the window.
		remove_child(wrapped_control);
		parent->add_child(wrapped_control);

		// Set the window rect even when the window is maximized to have a good default size
		// when the user remove the maximized mode.
		window->set_size(control_rect.size);
		window->set_position(control_rect.position);

		if (EDITOR_GET("interface/multi_window/maximize_window")) {
			window->set_mode(Window::MODE_MAXIMIZED);
		}

		// HACK: Reparenting the control from the original container
		// doesn't show it, so hide and show it again.
		wrapped_control->hide();
		wrapped_control->show();

		wrapped_control->set_anchors_and_offsets_preset(PRESET_FULL_RECT);

	} else if (!p_visible) {
		// Remove control from window.
		parent->remove_child(wrapped_control);
		add_child(wrapped_control);

		// HACK: Reparenting the control from the window doesn't show it, so hide
		// and show it again.
		wrapped_control->hide();
		wrapped_control->show();
	}

	emit_signal("window_visibility_changed", p_visible);
}

bool WindowWrapper::get_window_enabled() const {
	return is_window_available() ? window->is_visible() : false;
}

Rect2i WindowWrapper::get_window_rect() const {
	ERR_FAIL_COND_V(!get_window_enabled(), Rect2i());
	return Rect2i(window->get_position(), window->get_size());
}

int WindowWrapper::get_window_screen() const {
	ERR_FAIL_COND_V(!get_window_enabled(), -1);
	return window->get_current_screen();
}

void WindowWrapper::restore_window(const Rect2i &p_rect, int p_screen) {
	ERR_FAIL_COND(!is_window_available());
	ERR_FAIL_INDEX(p_screen, DisplayServer::get_singleton()->get_screen_count());

	enable_window_on_screen(p_screen, false);
	window->set_position(p_rect.position);
	window->set_size(p_rect.size);
}

void WindowWrapper::enable_window_on_screen(int p_screen, bool p_auto_scale) {
	int current_screen = Object::cast_to<Window>(get_viewport())->get_current_screen();

	bool auto_scale = p_auto_scale && !EDITOR_GET("interface/multi_window/maximize_window");

	if (auto_scale && current_screen != p_screen) {
		Rect2 control_rect = _get_default_window_rect();

		Rect2i source_screen_rect = DisplayServer::get_singleton()->screen_get_usable_rect(current_screen);
		Rect2i dest_screen_rect = DisplayServer::get_singleton()->screen_get_usable_rect(p_screen);

		// Adjust the window rect size in case the resolution changes.
		Vector2 screen_ratio = Vector2(source_screen_rect.size) / Vector2(dest_screen_rect.size);

		// The screen positioning may change, so remove the original screen position.
		control_rect.position -= source_screen_rect.position;
		control_rect = Rect2i(control_rect.position * screen_ratio, control_rect.size * screen_ratio);
		control_rect.position += dest_screen_rect.position;

		restore_window(control_rect, p_screen);
	} else {
		set_window_enabled(true);
		window->set_current_screen(p_screen);
	}
}

void WindowWrapper::set_window_title(const String p_title) {
	if (!is_window_available()) {
		return;
	}
	window->set_title(p_title);
}

void WindowWrapper::set_margins_enabled(bool p_enabled) {
	ERR_FAIL_COND(get_window_enabled());
	if (bool(margins != nullptr) == p_enabled) {
		return;
	}

	if (margins) {
		margins->queue_free();
		margins = nullptr;
	} else if (p_enabled) {
		Size2 borders = Size2(4, 4) * EDSCALE;
		margins = memnew(MarginContainer);
		margins->add_theme_constant_override("margin_right", borders.width);
		margins->add_theme_constant_override("margin_top", borders.height);
		margins->add_theme_constant_override("margin_left", borders.width);
		margins->add_theme_constant_override("margin_bottom", borders.height);

		window->add_child(margins);
		margins->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	}
}

WindowWrapper::WindowWrapper() {
	if (SceneTree::get_singleton()->get_root()->is_embedding_subwindows() && EDITOR_GET("interface/multi_window/enable")) {
		return;
	}

	window = memnew(Window);
	window->set_wrap_controls(true);
	window->set_transient(false);

	add_child(window);
	window->hide();

	window->connect("close_requested", callable_mp(this, &WindowWrapper::set_window_enabled).bind(false));

	ShortcutBin *capturer = memnew(ShortcutBin);
	window->add_child(capturer);

	window_background = memnew(Panel);
	window_background->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	window->add_child(window_background);

	set_process_shortcut_input(true);

	EDITOR_DEF_RST("interface/multi_window/enable", true);
	EDITOR_DEF("interface/multi_window/restore_windows_on_load", true);
	EDITOR_DEF("interface/multi_window/maximize_window", false);
}

// ScreenSelect

void ScreenSelect::_build_advanced_menu() {
	const real_t popup_height = real_t(get_theme_font_size("font_size")) * 2.0;
	const Size2 borders = Size2(4, 4) * EDSCALE;

	get_popup()->set_min_size(Size2(0, popup_height * 3));

	Panel *background = memnew(Panel);
	background->add_theme_style_override("panel", get_theme_stylebox("PanelForeground", "EditorStyles"));
	get_popup()->add_child(background);
	background->set_anchors_and_offsets_preset(PRESET_FULL_RECT);

	MarginContainer *popup_root = memnew(MarginContainer);
	popup_root->add_theme_constant_override("margin_right", borders.width);
	popup_root->add_theme_constant_override("margin_top", borders.height);
	popup_root->add_theme_constant_override("margin_left", borders.width);
	popup_root->add_theme_constant_override("margin_bottom", borders.height);
	get_popup()->add_child(popup_root);

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	popup_root->add_child(vb);

	Label *description = memnew(Label(TTR("Screens")));
	description->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	vb->add_child(description);

	HBoxContainer *screen_list = memnew(HBoxContainer);
	screen_list->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	vb->add_child(screen_list);

	popup_root->set_anchors_and_offsets_preset(PRESET_FULL_RECT);

	_populate_screen_list(screen_list);
}

void ScreenSelect::_populate_screen_list(Container *p_container) {
	const real_t height = real_t(get_theme_font_size("font_size")) * 1.5;

	int current_screen = Object::cast_to<Window>(get_viewport())->get_current_screen();
	for (int i = 0; i < DisplayServer::get_singleton()->get_screen_count(); i++) {
		Button *button = memnew(Button);

		Size2 screen_size = Size2(DisplayServer::get_singleton()->screen_get_size(i));
		Size2 button_size = Size2(height * (screen_size.x / screen_size.y), height);
		button->set_custom_minimum_size(button_size);
		p_container->add_child(button);

		if (i == current_screen) {
			button->set_icon(get_theme_icon("Favorites", "EditorIcons"));
			button->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		} else {
			button->set_text(itos(i));
			button->set_text_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		}
		button->connect("pressed", callable_mp(this, &ScreenSelect::_emit_screen_signal).bind(i));
		button->connect("pressed", callable_mp(static_cast<BaseButton *>(this), &ScreenSelect::set_pressed).bind(false));
	}
}

void ScreenSelect::_emit_screen_signal(int p_screen_idx) {
	emit_signal("request_open_in_screen", p_screen_idx);
}

void ScreenSelect::_bind_methods() {
	ADD_SIGNAL(MethodInfo("request_open_in_screen", PropertyInfo(Variant::INT, "screen")));
}

void ScreenSelect::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			connect("about_to_popup", callable_mp(this, &ScreenSelect::_build_advanced_menu));
			connect("gui_input", callable_mp(this, &ScreenSelect::_handle_mouse_shortcut));
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			set_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon("MakeFloating", "EditorIcons"));
		} break;
	}
}

void ScreenSelect::_handle_mouse_shortcut(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_valid()) {
		if (mouse_button->is_pressed() && mouse_button->get_button_index() == MouseButton::LEFT) {
			_emit_screen_signal(Object::cast_to<Window>(get_viewport())->get_current_screen());
			accept_event();
		}
	}
}

ScreenSelect::ScreenSelect() {
	set_text(TTR("Make Floating"));
	set_button_mask(mouse_button_to_mask(MouseButton::RIGHT));
}
