// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "ToolsConfig.h"

// BinaryTree:
//
// Sorted binary tree for Items that support the "less than" and "equal" operator.

// Tree Item:
// Holds the full Item as well as left/right pointers
template<class Item>
class BinaryTreeItem
{
public:
	Item		_item;
	BinaryTreeItem	*_left;
	BinaryTreeItem	*_right;
};

template<class Item>
class BinaryTree
{
public:
	BinaryTree ()
	{	_root = 0; }

	~BinaryTree ()
	{	cleanup (_root); }

	// Item is copied into the tree
	void add (const Item &item)
	{
		BinaryTreeItem<Item> *n = new BinaryTreeItem<Item>;
		n->_item = item;
		n->_left = 0;
		n->_right = 0;

		insert (n, &_root);
	}

	// Try to find item in tree.
	// Result: true if found
	bool find (const Item &item)
	{
		BinaryTreeItem<Item> *n = _root;

		while (n)
		{
			if (item == n->_item)
				return true;
			if (item < n->_item)
				n = n->_left;
			else
				n = n->_right;
		}

		return false;
	}

	// Sorted (inorder) traverse,
	// calling visitor routine on each item
	void traverse (void (*visit) (const Item &, void *), void *arg=0)
	{	visit_inorder (visit, _root, arg); }

private:
	BinaryTreeItem<Item>	*_root;

	void insert (BinaryTreeItem<Item> *new_item, BinaryTreeItem<Item> **node)
	{
		if (*node == 0)
			*node = new_item;
		else if (new_item->_item  <  (*node)->_item)
			insert (new_item, & ((*node)->_left));
		else
			insert (new_item, & ((*node)->_right));
	}

	void visit_inorder (void (*visit) (const Item &, void *),
                        BinaryTreeItem<Item> *node, void *arg)
	{
		if (node)
		{
			visit_inorder (visit, node->_left, arg);
			visit (node->_item, arg);
			visit_inorder (visit, node->_right, arg);
		}
	}

	void cleanup (BinaryTreeItem<Item> *node)
	{
		if (node)
		{
			cleanup (node->_left);
			cleanup (node->_right);
			delete node;
		}
	}
};


