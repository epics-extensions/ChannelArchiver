// -*- c++ -*-

#ifndef _FUX_H_
#define _FUX_H_

// System
#include <stdio.h>
// Tools
#include <ToolsConfig.h>

// Define this one to use the Xerces XML library
// When undefined, we default to the Expat library.
// Also check ChannelArchiver/make.cfg for the required link commands
#define FUX_XERCES

/// \ingroup Tools

/// FUX: the f.u. XML helper class.

/// "f.u." could stand for
/// - frightingly useless
/// - friendly and utilitarian
/// - ... or whatever you want.
///
/// In any case, FUX implements XML read
/// and write support.
/// The 'read' part is based on XML libraries,
/// there there's a choice between
/// - Xerces C++, see http://xml.apache.org/index.html
/// - Expat, see http://www.libexpat.org.
/// Since Xerces handles validation and Expat doesn't,
/// the former should be preferred.
class FUX
{
public:
    FUX(); ///< Constructor
    ~FUX(); ///< Destructor

    /// One element in the FUX tree.
    class Element
    {
    public:
        Element(Element *parent, const stdString &name); ///< Constructor
        ~Element();      ///< Destructor
        Element *parent; ///< Parent element or 0.
        stdString name;  ///< Name of this element.
        stdString value; ///< Value of this element.

        /// Add a child to this Element.
        void add(Element *child)
        {   children.push_back(child); }

        /// Returns the first child of given name or 0.
        Element *find(const char *name);
        
        /// List of children.
        stdList<Element *> children;
    };

    stdString DTD; ///< The DTD. Set for dump().

    /// Parse the given XML file into the FUX tree.

    /// Returns root of the FUX document or zero.
    ///
    ///
    Element *parse(const char *file_name);

    /// Dumps the FUX document.

    /// Elements with values that are pure white space are
    /// printed as empty elements. Tabs are used to indent
    /// the elements according to their hierarchical location
    /// in the document.
    void dump(FILE *f);
private:
#ifdef FUX_XERCES
    friend class FUXContentHandler;
    friend class FUXErrorHandler;
#endif
    enum State { error, idle, element };
    State state;

    Element *root;
    Element *current;
    
    static void start_tag(void *data, const char *el, const char **attr);
    static void text_handler(void *data, const char *s, int len);
    static void end_tag(void *data, const char *el);
    void dump_element(FILE *f, Element *e, int depth);
};

#endif
