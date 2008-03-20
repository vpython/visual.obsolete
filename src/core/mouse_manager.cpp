#include "mouse_manager.hpp"
#include "display_kernel.hpp"

namespace cvisual {

/* The implementation here is rather rudimentary - a step backward in functionality.
   But it provides a very simple interface to the display drivers, which should suffice
   to implement all the features we need.  So we can get the drivers right now and worry
   about the details of event handling later. */

mouse_manager::mouse_manager( class display_kernel& display )
 : display(display), px(0), py(0), locked(false), left_down(false), left_dragging(false), left_semidrag(false)
{
	buttons[0] = buttons[1] = false;
}

// trivial properties
bool mouse_manager::is_mouse_locked() { return locked; }
int mouse_manager::get_x() { return px; }
int mouse_manager::get_y() { return py; }
mouse_t& mouse_manager::get_mouse() { return mouse; }

static void fill( int out_size, bool out[], int in_size, bool in[], bool def = false ) {
	for(int i=0; i<out_size && i<in_size; i++)
		out[i] = in[i];
	for(int i=in_size; i<out_size; i++)
		out[i] = def;
}

void mouse_manager::report_mouse_state( int physical_button_count, bool is_button_down[], 
										int cursor_client_x, int cursor_client_y,
										int shift_state_count, bool shift_state[],
										bool can_lock_mouse )
{
	// Right now we only support a 2-button mouse with shift,ctrl,alt
	bool new_buttons[2]; fill(2, new_buttons, physical_button_count, is_button_down );
	bool new_shift[3]; fill(3, new_shift, shift_state_count, shift_state);

	// If there's been more than one button change, impose an arbitrary order, so that update()
	// only sees one change at a time even if the display driver doesn't enforce that
	if (new_buttons[0] != buttons[0] && new_buttons[1] != buttons[1]) {
		new_buttons[1] = !new_buttons[1];
		update( new_buttons, cursor_client_x, cursor_client_y, new_shift, can_lock_mouse );
		new_buttons[1] = !new_buttons[1];
	}
	update( new_buttons, cursor_client_x, cursor_client_y, new_shift, can_lock_mouse );
}

void mouse_manager::update( bool new_buttons[], int new_px, int new_py, bool new_shift[], bool can_lock_mouse ) {
	// Shift states are just passed directly to mouseobject
	mouse.set_shift( new_shift[0] );
	mouse.set_ctrl( new_shift[1] );
	mouse.set_alt( new_shift[2] );

	locked = (can_lock_mouse && display.zoom_is_allowed() && new_buttons[0] && new_buttons[1]) ||
	         (can_lock_mouse && display.spin_is_allowed() && new_buttons[1]);
	if (new_buttons[1])
		display.report_camera_motion( new_px - px, new_py - py, 
									  new_buttons[0] ? display_kernel::MIDDLE : display_kernel::RIGHT );

	// left_semidrag means that we've moved the mouse and so can't get a left click, but we aren't
	// necessarily actually dragging, because the movement might have occurred with the right button down.
	if (left_down && !left_dragging && (new_px != px || new_py != py))
		left_semidrag = true;
	if (!left_down) left_semidrag = false;
	
	if (!new_buttons[1]) { //< Ignore changes in the left button state while the right button is down!
		bool b = new_buttons[0];
	
		if (b != left_down) {
			if (b) {
				if ( !buttons[0] ) //< Releasing the other button of a chord doesn't "press" the left
					mouse.push_event( press_event(1, mouse) );
				else
					b = false;
			} else if ( left_dragging ) {
				mouse.push_event( drop_event(1, mouse) );
				left_dragging = false;
			} else if ( left_semidrag ) {
				mouse.push_event( release_event(1, mouse) );
			} else if (left_down) {
				mouse.push_event( click_event(1, mouse) );
			}
		}
	
		if ( b && left_down && (new_px != px || new_py != py) && !left_dragging ) {
			mouse.push_event( drag_event(1, mouse) );
			left_dragging = true;
		}
		
		left_down = b;
	}

	// xxx Generate mouse events for right (and middle?) buttons.  Very carefully.
		
	if (!locked) {
		px = new_px;
		py = new_py;
	}
	for(int b=0; b<2; b++) buttons[b] = new_buttons[b];
}

} // namespace cvisual