// Tools
#include "FUX.h"

FUX::Element::~Element()
{    
    stdList<Element *>::iterator c;
    for (c=children.begin(); c!=children.end(); ++c)
        delete *c;
}

FUX::FUX()
        : state(idle), root(0), current(0)
{}

FUX::~FUX()
{
    if (root)
        delete root;
}

void FUX::start_tag(void *data, const char *el, const char **attr)
{
    FUX *me = (FUX *)data;
    if (me->root == 0)
        me->root = me->current = new Element(0, el);
    else
    {
        me->current = new Element(me->current, el);
        me->current->parent->addChild(me->current);
    }
    me->state = element;
}

void FUX::text_handler(void *data, const char *s, int len)
{
    FUX *me = (FUX *)data;
    if (me->state == idle)
        return;
    me->current->value.append(s, len);
}

void FUX::end_tag(void *data, const char *el)
{
    FUX *me = (FUX *)data;
    if (me->current)
        me->current = me->current->parent;
    else
        fprintf(stderr, "FUX: malformed '%s'\n", el);
    me->state = idle;
}

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
        fprintf(stderr, "Couldn't allocate memory for parser\n");
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
            fprintf(stderr, "FUX: No buffer\n");
            goto parse_error;
        }
            
        int len = fread(buf, 1, sizeof(buf), f);
        if (ferror(f))
        {
            fprintf(stderr, "FUX: Read error\n");
            goto parse_error;
        }
        done = feof(f);
        if (! XML_ParseBuffer(p, len, done))
        {
            fprintf(stderr, "FUX: Error at line %d:\n%s\n",
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

inline void indent(int depth)
{
    for (int i=0; i<depth; ++i)
        printf("\t");    
}

void FUX::dump(FILE *f)
{
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
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
    indent(depth);
    if (all_white_text(e->value))
        fprintf(f, "<%s>", e->name.c_str());
    else
        fprintf(f, "<%s>%s", e->name.c_str(), e->value.c_str());
    if (e->children.size() > 0)
    {
        fprintf(f, "\n");
        stdList<Element *>::const_iterator c;
        for (c=e->children.begin(); c!=e->children.end(); ++c)
            dump_element(f, *c, depth+1);
        indent(depth);
    }
    fprintf(f, "</%s>\n", e->name.c_str());    
}


