// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "mouseobject.hpp"
#include "display_kernel.hpp"
#include <stdexcept>

namespace cvisual {


/* Translate a button click code into a text string.
 */ 
std::string* 
mousebase::button_name(int buttons) 
{
	switch (buttons) {
		case display_kernel::NONE:
			return 0;
		case display_kernel::LEFT:
			return new std::string("left");
		case display_kernel::RIGHT:
			return new std::string("right");
		case display_kernel::MIDDLE:
			return new std::string("middle");
		default:
			// This should *never* happen.
			throw std::invalid_argument("Button type should be left, right, or middle.");
	}
}

std::string* 
mousebase::get_buttons()
{
	return button_name( buttons);
}

/* Project the cursor's current location onto the plane specified by the normal 
 * vector 'normal' and a perpendicular distance 'dist' from the origin.
 */
vector 
mousebase::project1( vector normal, double dist)
{
	double ndc = normal.dot(cam) - dist;
	double ndr = normal.dot(ray);
	double t = -ndc / ndr;
	vector v = cam + ray*t;
	return v;
}

/* Project the cursor's current position onto the plane specified by the normal vector
 * 'normal' rooted at the position vector 'point'.
 */
vector 
mousebase::project2( vector normal, vector point)
{
	double dist = normal.dot(point);
	double ndc = normal.dot(cam) - dist;
	double ndr = normal.dot(ray);
	double t = -ndc / ndr;
	vector v = cam + ray*t;
	return v;	
}

std::string*
mousebase::get_press()
{
	if (is_press())
		return get_buttons();
	else
		return 0;
}

std::string*
mousebase::get_release()
{
	if (is_release())
		return get_buttons();
	else
		return 0;
}

std::string*
mousebase::get_drag()
{
	if (is_drag())
		return get_buttons();
	else
		return 0;
}

std::string*
mousebase::get_drop()
{
	if (is_drop())
		return get_buttons();
	else
		return 0;
}

std::string*
mousebase::get_click()
{
	if (is_click())
		return get_buttons();
	else
		return 0;
}

shared_ptr<renderable>
mousebase::get_pick()
{
	return pick;
}

/************** event implementation **************/
#if 0
shared_ptr<event> 
event::create_press( 
		shared_ptr<renderable> pick, 
		vector pickpos,
		modifiers buttonstate,
		display_kernel::mouse_button buttons)
{
	shared_ptr<event> ret( new event());
	ret->pickpos = pickpos;
	ret->pick = pick;
	if (buttonstate & ctrl)
		ret->set_ctrl( true);
	if (buttonstate & shift)
		ret->set_shift( true);
	if (buttonstate & alt)
		ret->set_alt();
	ret->set_press( true);
	ret->buttons = buttons;
	
	return ret;
}
#endif

/************** mouseObject implementation **************/


void 
mouse::clear_events( int i)
{
	if (i != 0) {
		throw std::invalid_argument( "mouse.events can only be set to zero");
	}
	events.clear();
	return;
}

int
mouse::num_events() const
{
	return events.size();
}

int
mouse::num_clicks() const
{
	return click_count;
}

shared_ptr<event>
mouse::pop_event()
{
	shared_ptr<event> ret = events.pop();
	if (ret->is_click())
		click_count--;
	return ret;
}

shared_ptr<event>
mouse::pop_click()
{
	shared_ptr<event> ret = events.pop();
	while (!ret->is_click()) {
		ret = events.pop();
	}
	click_count--;
	return ret;
}

void
mouse::push_event( shared_ptr<event> e)
{
	if (e->is_click())
		click_count++;
	events.push( e);
}




} // !namespace visual
