// -*- c++ -*-

#ifndef _FUX_H_
#define _FUX_H_

// System
#include <stdio.h>
// XMS Expat
#include <xmlparse.h>
// Tools
#include <ToolsConfig.h>

/// \ingroup Tools

/// FUX: the f.u. XML helper class.

/// "f.u." could stand for
/// - frightingly useless
/// - friendly and utilitarian
/// - ... or whatever you want.
///
/// In any case, FUX implements XML read
/// and write support, where the reading
/// is based on the expat library from
/// http://www.libexpat.org.
class FUX
{
public:
    FUX();

    ~FUX();

    /// One element in the FUX tree.
    class Element
    {
    public:
        Element(Element *parent,
                const stdString &name)
                : parent(parent), name(name)
        {}

        ~Element();
        
        Element *parent;
        stdString name, value;

        void addChild(Element *child)
        {   children.push_back(child); }
        
        stdList<Element *> children;
    };

    /// Parse the given XML file into the FUX tree.

    /// Returns root of the FUX document or zero.
    Element *parse(const char *file_name);

    /// Dumps the FUX document.

    /// Elements with values that are pure white space are
    /// printed as empty elements. Tabs are used to indent
    /// the elements according to their hierarchical location
    /// in the document.
    void dump(FILE *f);
private:
    enum State { idle, element };
    State state;

    Element *root;
    Element *current;
    
    static void start_tag(void *data, const char *el, const char **attr);
    static void text_handler(void *data, const char *s, int len);
    static void end_tag(void *data, const char *el);
    void dump_element(FILE *f, Element *e, int depth);

};

#endif
