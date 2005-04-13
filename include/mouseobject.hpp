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
 	std::string button_name();
 	std::bitset<3> modifiers;
	std::bitset<5> eventtype;
	std::bitset<3> buttons;

 public:
	mousebase() {}
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
	
	enum button_t { left, middle, right };


	inline bool is_press() const { return eventtype.test( press); }
	inline bool is_release() const { return eventtype.test( release); }
	inline bool is_click() const { return eventtype.test( click); }
	inline bool is_drag() const { return eventtype.test( drag); }
	inline bool is_drop() const { return eventtype.test( drop); }
	std::string* get_buttons() const;
	inline bool is_alt() const { return modifiers.test( alt); }
	inline bool is_shift() const { return modifiers.test( shift); }
	inline bool is_ctrl() const { return modifiers.test( ctrl); }
	inline vector get_pos() const { return position; }
	inline vector get_camera() const { return cam; }
	inline vector get_ray() const { return ray; }
	inline vector get_pickpos() const { return pickpos; }
	shared_ptr<renderable> get_pick();
	
	inline void set_shift( bool _shift) { modifiers.set( shift, _shift); }
	inline void set_ctrl( bool _ctrl) { modifiers.set( ctrl, _ctrl); }
	inline void set_alt( bool _alt) { modifiers.set( alt,  _alt); }
	
	inline void set_press( bool _press) { eventtype.set( press, _press); }
	inline void set_release( bool _release) { eventtype.set( release, _release); }
	inline void set_click( bool _click) { eventtype.set( click, _click); }
	inline void set_drag( bool _drag) { eventtype.set( drag, _drag); }
	inline void set_drop( bool _drop) { eventtype.set( drop, _drop); }
	
	inline void set_leftdown( bool _ld) { buttons.set( left, _ld); }
	inline void set_rightdown( bool _rd) { buttons.set( right, _rd); }
	inline void set_middledown( bool _md) { buttons.set( middle, _md); }
	
	vector project1( vector normal, double dist);
	vector project2( vector normal, vector point = vector(0,0,0));

	// These functions will return an object constructed from std::string, or None.
	std::string get_press();
	std::string get_release();
	std::string get_click();
	std::string get_drag();
	std::string get_drop();
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
    // Exposed as the function display.mouse.getevent()
	shared_ptr<event> pop_event();
    // Exposed as the function mouse.getclick()
	shared_ptr<event> pop_click();
	
	/** Push a new event onto the queue.  This function is not exposed to Python.
	 */
	void push_event( shared_ptr<event>);
};

// Convenience functions for creating event objects.
// which represents which mouse button is involved:
// 1 for left
// 2 for middle
// 3 for right
// no other number is valid.
shared_ptr<event> click_event( int which, const mouse_t& mouse);
shared_ptr<event> drop_event( int which, const mouse_t& mouse);
shared_ptr<event> press_event( int which, const mouse_t& mouse);
shared_ptr<event> drag_event( int which, const mouse_t& mouse);

} // !namespace cvisual

#endif // !VPYTHON_MOUSEOBJECT_HPP
