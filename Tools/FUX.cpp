// Tools
#include "MsgLogger.h"
#include "FUX.h"

// XML library
#ifdef FUX_XERCES
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
XERCES_CPP_NAMESPACE_USE
#else
#include <xmlparse.h>
#endif

// TODO: Catch the name of the DTD so that
//       we can print it out in dump()?
//       Unclear how to do that in a SAX
//       environment. he Xerces entity resolver
//       sees the systemID of the <!DOCTYPE ... >
//       entry, but it's not obvious that
//       the parser is in a <!DOCTYPE> tag at that point.
//       Could also be <!ENTITY ...>.

FUX::Element::Element(FUX::Element *parent, const stdString &name)
        : parent(parent), name(name)
{}

FUX::Element::~Element()
{    
    stdList<Element *>::iterator c;
    for (c=children.begin(); c!=children.end(); ++c)
        delete *c;
}

FUX::Element *FUX::Element::find(const char *name)
{    
    stdList<Element *>::iterator c;
    for (c=children.begin(); c!=children.end(); ++c)
        if ((*c)->name == name)
            return *c;
    return 0;
}

FUX::FUX()
        : state(idle), root(0), current(0)
{}

FUX::~FUX()
{
    if (root)
        delete root;
}

void FUX::setDoc(Element *doc)
{
    if (root)
        delete root;
    root = doc;
}

void FUX::start_tag(void *data, const char *el, const char **attr)
{
    FUX *me = (FUX *)data;
    if (me->state == error)
        return;
    if (me->root == 0)
        me->root = me->current = new Element(0, el);
    else
    {
        me->current = new Element(me->current, el);
        me->current->parent->add(me->current);
    }
    me->state = element;
}

void FUX::text_handler(void *data, const char *s, int len)
{
    FUX *me = (FUX *)data;
    if (me->state != element)
        return;
    me->current->value.append(s, len);
}

void FUX::end_tag(void *data, const char *el)
{
    FUX *me = (FUX *)data;
    if (me->state == error)
        return;
    if (me->current)
        me->current = me->current->parent;
    else
        fprintf(stderr, "FUX: malformed '%s'\n", el);
    me->state = idle;
}

inline void indent(FILE *f, int depth)
{
    for (int i=0; i<depth; ++i)
        fprintf(f, "\t");
}

void FUX::dump(FILE *f)
{
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    if (root && DTD.length() > 0)
        fprintf(f, "<!DOCTYPE %s SYSTEM \"%s\">\n",
                root->name.c_str(), DTD.c_str());
    dump_element(f, root, 0);
}

inline bool all_white_text(const stdString &text)
{
    const char *c = text.c_str();
    while (*c)
    {
        if (strchr(" \t\n\r", *c)==0)
            return false;
        ++c;
    }
    return true;
}

void FUX::dump_element(FILE *f, Element *e, int depth)
{
    if (!e)
        return;
    indent(f, depth);
    if (all_white_text(e->value))
    {
        if (e->children.empty())
        {
            fprintf(f, "<%s/>\n", e->name.c_str());
            return;
        }
        fprintf(f, "<%s>", e->name.c_str());
    }
    else
        fprintf(f, "<%s>%s", e->name.c_str(), e->value.c_str());
    if (!e->children.empty())
    {
        fprintf(f, "\n");
        stdList<Element *>::const_iterator c;
        for (c=e->children.begin(); c!=e->children.end(); ++c)
            dump_element(f, *c, depth+1);
        indent(f, depth);
    }
    fprintf(f, "</%s>\n", e->name.c_str());    
}

#ifdef FUX_XERCES
// Xerces implementation ---------------------------------------------------

#if 0
class FUXEntityResolver : public EntityResolver
{
public:
    InputSource *resolveEntity(const XMLCh *const publicId,
                               const XMLCh *const systemId);
};

InputSource *FUXEntityResolver::resolveEntity(const XMLCh *const publicId,
                                              const XMLCh *const systemId)
{
    char *s;
    s = XMLString::transcode(publicId);
    printf("Entity, publicId '%s'\n", s);
    XMLString::release(&s);
    s = XMLString::transcode(systemId);
    printf("Entity, systemId '%s'\n", s);
    XMLString::release(&s);
    return 0;
}
#endif

class FUXContentHandler : public DefaultHandler
{
public:
    FUXContentHandler(FUX *fux) : fux(fux) {}
    void startElement(const XMLCh* const uri, const XMLCh* const localname,
                      const XMLCh* const qname,const Attributes& attrs);
    void characters(const XMLCh *const chars, const unsigned int length);
    void endElement(const XMLCh* const uri, const XMLCh* const localname,
                    const XMLCh* const qname);
private:
    FUX *fux;
};

void FUXContentHandler::startElement(const XMLCh* const uri,
                                     const XMLCh* const localname,
                                     const XMLCh* const name,
                                     const Attributes& attrs)
{
    char *s = XMLString::transcode(name);
    fux->start_tag(fux, s, 0);
    XMLString::release(&s);
}

void FUXContentHandler::characters(const XMLCh *const chars,
                                   const unsigned int length)
{
    char buf[500]; // TODO: Loop over chars in case length > sizeof(buf)
    int len = length;
    if (len >= (int)sizeof(buf))
        len = sizeof(buf) - 1;
    if (!XMLString::transcode(chars, buf, len))
    {
        LOG_MSG("FUXContentHandler: Transcode error\n");
        fux->state = FUX::error;
    }
    fux->text_handler(fux, buf, len);
}

void FUXContentHandler::endElement(const XMLCh* const uri,
                                   const XMLCh* const localname,
                                   const XMLCh* const name)
{
    char *s = XMLString::transcode(name);
    fux->end_tag(fux, s);
    XMLString::release(&s);
}

class FUXErrorHandler : public DefaultHandler
{
public:
    FUXErrorHandler(FUX *fux) : fux(fux) {}    
    void warning(const SAXParseException&);
    void error(const SAXParseException&);
    void fatalError(const SAXParseException&);
private:
    FUX *fux;
};

void FUXErrorHandler::warning(const SAXParseException& exception)
{
    char* message = XMLString::transcode(exception.getMessage());
    LOG_MSG("XML Warning, Line %d: %s\n", exception.getLineNumber(), message);
    XMLString::release(&message);
}

void FUXErrorHandler::error(const SAXParseException& exception)
{
    fux->state = FUX::error;
    char* message = XMLString::transcode(exception.getMessage());
    LOG_MSG("XML Error, Line %d: %s\n", exception.getLineNumber(), message);
    XMLString::release(&message);
}

void FUXErrorHandler::fatalError(const SAXParseException& exception)
{
    fux->state = FUX::error;
    char* message = XMLString::transcode(exception.getMessage());
    LOG_MSG("XML Error (fatal), Line %d: %s\n",
            exception.getLineNumber(), message);
    XMLString::release(&message);
}

FUX::Element *FUX::parse(const char *file_name)
{
    try
    {
        XMLPlatformUtils::Initialize();
        //FUXEntityResolver entity_resolver;
        FUXContentHandler content_handler(this);
        FUXErrorHandler   error_handler(this);
        SAX2XMLReader *parser = XMLReaderFactory::createXMLReader();
        if (!parser)
        {
            LOG_MSG("Couldn't allocate parser\n");
            return 0;
        }
        // Very strange notation, but these XMLUni constants
        // are simply the XMLCh strings shown in the following comments.
        // Those URLs matche the SAX2XMLReader documentation,
        // but it's very hard to find the XMLUni members
        // for the URLs without looking at XMLUni.cpp.
        // "http://xml.org/sax/features/validation"
        parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
        // "http://xml.org/sax/features/namespaces"
        parser->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
        // "http://xml.org/sax/features/validation"
        parser->setFeature(XMLUni::fgXercesDynamic, true);
        // "http://apache.org/xml/features/validation-error-as-fatal"
        parser->setFeature(XMLUni::fgXercesValidationErrorAsFatal, true);
        //parser->setEntityResolver(&entity_resolver);
        parser->setContentHandler(&content_handler);
        parser->setErrorHandler(&error_handler);
        parser->parse(file_name);
        delete parser;
        XMLPlatformUtils::Terminate();
        if (state == error)
            return 0;
    }
    catch (const XMLException &toCatch)
    {
        char* message = XMLString::transcode(toCatch.getMessage());
        LOG_MSG("Xerces exception: %s\n", message);
        XMLString::release(&message);
        return 0;
    }
    catch (const SAXParseException &toCatch)
    {
        char* message = XMLString::transcode(toCatch.getMessage());
        LOG_MSG("Xerces exception: %s\n", message);
        XMLString::release(&message);
        return 0;
    }
    catch (...)
    {
        printf("Xerces error\n");
        return 0;
    }  
    return root;
}
// End of Xerces implementation --------------------------------------------
#else
// Expat implementation ----------------------------------------------------
FUX::Element *FUX::parse(const char *file_name)
{
    bool done = false;
    FILE *f = 0;
    f = fopen(file_name, "rt");
    if (!f)
        return 0;
    XML_Parser p = XML_ParserCreate(NULL);
    if (! p)
    {
        LOG_MSG("Couldn't allocate memory for parser\n");
        goto parse_error;
    }
    XML_SetUserData(p, this);
    XML_SetElementHandler(p, start_tag, end_tag);
    XML_SetCharacterDataHandler(p, text_handler);
    while (!done)
    {
        void *buf = XML_GetBuffer(p, 1000);
        if (!buf)
        {
            LOG_MSG("FUX: No buffer\n");
            goto parse_error;
        }
            
        int len = fread(buf, 1, sizeof(buf), f);
        if (ferror(f))
        {
            LOG_MSG("FUX: Read error\n");
            goto parse_error;
        }
        done = feof(f);
        if (! XML_ParseBuffer(p, len, done))
        {
            LOG_MSG("FUX: Error at line %d: %s\n",
                    XML_GetCurrentLineNumber(p),
                    XML_ErrorString(XML_GetErrorCode(p)));
            goto parse_error;
        }
    }
    XML_ParserFree(p);
    fclose(f);
    return root;
  parse_error:
    XML_ParserFree(p);
    fclose(f);
    return 0;
}
// End of Expat implementation ----------------------------------------------
#endif
