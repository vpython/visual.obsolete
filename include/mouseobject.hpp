#ifndef VPYTHON_MOUSEOBJECT_HPP
#define VPYTHON_MOUSEOBJECT_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include "util/atomic_queue.hpp"

#include <queue>
#include <utility>
#include <bitset>

namespace cvisual {

/** This common base class implements common functionality for event and mouse.
 * It should never be used directly.
 */
class mousebase
{
 protected:
 	std::string* button_name( int);
 	std::bitset<3> modifiers;
	std::bitset<5> eventtype;
	int buttons;

 public:
	mousebase() : buttons(0) {}
	// The position of the mouse, either currently, or when the even happened.
	vector position;
	// The position of the camera in the scene.
	vector cam;
	// A vector pointing from the camera into the scene colinear with the mouse.
	vector ray;
	// The object nearest to the cursor when this event happened.
	shared_ptr<renderable> pick;
	// The position on the object that intersects with ray.
	vector pickpos;
	
	/* 'buttonstate' contains the following state flags as defined by 'button'.
	 */
	enum modifiers_t { ctrl, alt, shift };
	
	/* 'eventtype' contains state flags as defined by 'event'.
	 */
	enum event_t { press, release, click, drag, drop };


	inline bool is_press() { return eventtype.test( press); }
	inline bool is_release() { return eventtype.test( release); }
	inline bool is_click() { return eventtype.test( click); }
	inline bool is_drag() { return eventtype.test( drag); }
	inline bool is_drop() { return eventtype.test( drop); }
	std::string* get_buttons();
	inline bool is_alt() { return modifiers.test( alt); }
	inline bool is_shift() { return modifiers.test( shift); }
	inline bool is_ctrl(){ return modifiers.test( ctrl); }
	inline vector get_pos() { return position; }
	inline vector get_camera() { return cam; }
	inline vector get_ray(){ return ray; }
	inline vector get_pickpos() { return pickpos; }
	shared_ptr<renderable> get_pick();
	
	inline void set_shift( bool _shift) { modifiers.set( shift, _shift); }
	inline void set_ctrl( bool _ctrl) { modifiers.set( ctrl, _ctrl); }
	inline void set_alt( bool _alt) { modifiers.set( alt,  _alt); }
	inline void set_press( bool _press) { eventtype.set( press, _press); }
	inline void set_release( bool _release) { eventtype.set( release, _release); }
	inline void set_click( bool _click) { eventtype.set( click, _click); }
	inline void set_drag( bool _drag) { eventtype.set( drag, _drag); }
	inline void set_drop( bool _drop) { eventtype.set( drop, _drop); }
	
	vector project1( vector normal, double dist);
	vector project2( vector normal, vector point = vector(0,0,0));

	// These functions will return an object constructed from std::string, or None.
	std::string* get_press();
	std::string* get_release();
	std::string* get_click();
	std::string* get_drag();
	std::string* get_drop();
};

/* Objects of this class represent the state of the mouse at a distinct event:
 * 	either press, release, click, drag, or drop.
 */
class event: public mousebase 
{
 public:
	event(){}
};

/* A class exported to python as the single object display.mouse.
 * All of the python access for data within this class get the present value of
 * the data.
 */
class mouse_t : public mousebase
{
 private:
	// The bool tells whether or not the click was a left click or not.
	atomic_queue<shared_ptr<event> > events;
	int click_count; // number of queued events which are left clicks
	
 public:
	mutex mtx;
	
	// The following member functions are synchronized - no additional locking
	// is requred.
	int num_events() const;
	void clear_events(int);
	int num_clicks() const;
	shared_ptr<event> pop_event();
	shared_ptr<event> pop_click();
	
	/** Push a new event onto the queue.  This function not exposed to Python.
	 */
	void push_event( shared_ptr<event>);
};

} // !namespace cvisual

#endif // !VPYTHON_MOUSEOBJECT_HPP
