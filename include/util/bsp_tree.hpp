#ifndef VPYTHON_UTIL_BSP_TREE_HPP
#define VPYTHON_UTIL_BSP_TREE_HPP

#include <list>
#include <allocator>

#include "util/vector.hpp"

/** A helper object to define a 2D flat plane in R3. */
class plane_equation
{
 private:
 	// The tolerance level for whether or not an object lies in a particular plane.
	static const thickness_epsilon;
	// The tolerance level for whether or not an object is coplanar with another.
	static const angle_epsilon;
 
	// The plane equation is defined by: 
	// normal.x*X + normal.y*Y + normal.z*Z + d == 0
	vector normal;
	double d;
	
 public:
	// Construct a plane with normal vector n that includes a particular
	// point.
	plane_equation( const vector& normal, const vector& point);
	
	// Returns: 1 if the point is in front of the plane,
	// 	0 if the point lies in the plane
	//  -1 if the point is behind the plane.
	int classify( const vector& point) const;
};


/** An implementation of a Binary Partition Tree for transparent models in 3D
space.  This class is not complete, and may not be used at all.
*/
template <typename Polygon>
class bsp_tree
{
 private: // Private types and data.
	struct Node
	{
		typedef std::list<Polygon> container;
		// The definition of this plane
		plane_equation def;
		// All of the faces that lie on this plane.  TODO: Replace this with
		// a singly-linked list.
		container faces;
		// The Node containing objects in front of this one, as classified by
		// plane_equation::classify().
		Node* front;
		// The same, only behind this node.
		Node* back;
		// The parent of this node, NULL for the head.
		Node* parent;
		// Construct a new node from an initial Polygon.
		Node( const Polygon& p, Node* = NULL);
	};
	
	Node* head;
	// The memory pool for this container.
	std::allocator<Node> pool;

 public: // Public types
	class iterator
	{
	 private:
		const vector forward; //< The traversal criterion.
		bool reverse; //< True if the traversal order for this node is front to
		// back to fit the traversal criterion.
		Node* current; //< the current node
		// The polygon currently pointed to by this iterator, and the end point
		// criterion for the list.
		Node::container::iterator current_poly, end;
		void incriment();
	 	iterator( Node* head, const vector& forward);
	 	
	 public:
	 	iterator();
	 	iterator( const iterator& other);
	 	const iterator& operator=( const iterator& other);
	 	bool operator==( const iterator& other);
	 	
	 	inline iterator& operator++()
	 	{ incriment(); return *this; }
	 	
	 	inline iterator operator++(int)
	 	{
	 		iterator last = *this;
	 		incriment();
	 		return last;
	 	}
	 	
	 	inline Polygon* operator->();
	 	{ return &*current_poly; }
	 	
	 	inline Polygon& operator*();
		{ return *current_poly; }
	};
	
	class const_iterator
	{
	 private:
		const vector forward;
		const Node* current;
		Node::container::const_iterator current_poly, end;
		void incriment();
	 	const iterator( const Node* head, const vector& forward);
		
	 public:
	 	const_iterator();
	 	const_iterator( const const_iterator& other);
	 	const const_iterator& operator=( const const_iterator& other);
	 	
	 	bool operator==( const iterator& other);
	 	
	 	inline const_iterator& operator++();
	 	{ incriment(); return *this; }
	 	
	 	inline const_iterator operator++(int)
	 	{
	 		const_iterator last = *this;
	 		incriment();
	 		return last;	 		
	 	}
	 	
	 	inline const Polygon* operator->()
	 	{ return &*current_poly; }
	 	
	 	inline const Polygon& operator*()
		{ return *current_poly; }
	};
	
 public: // Public functions.
	// Construct a new tree with the contents of any container of Polygon.
	template <typename ForwardIterator>
	bsp_tree( ForwardIterator start, ForwardIterator stop);
	
	// Returns true iff the container is empty.
	inline bool empty() const
	{ return Head == NULL; }
	
	// Returns the number of polygons within the tree.
	size_t size() const;
	
	// Returns an iterator pointing to the beginning of the tree that traverses
	// it with a forward vector initialized to +z
	iterator begin();
	iterator begin( const vector& forward);
	const_iterator begin() const;
	const_iterator begin( const vector& forward) const;
	// Add a single polygon to the container.
	void insert( const Polygon&);
	
	// Add a collection of polygons to the container.
	template <typename ForwardIterator>
	void insert( ForwardIterator start, ForwardIterator stop);
	
	// Probably not ever used, removes a single polygon.
	void remove( iterator polygon);
	
	// Call UniaryFunctor with each Polygon held within this bsp_tree from
	// back to front in the order defined by forward.
	template <typename UniaryFunctor>
	void for_all( UniaryFunctor action, vector forward = vector(0,0,1));
	
 private:
	friend class iterator;
	friend class const_iteator;
};

#endif // !defined VPYTHON_UTIL_BSP_TREE_HPP
