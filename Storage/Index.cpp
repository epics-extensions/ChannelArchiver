// Tools
#include <Filename.h>
// Local
#include "Index.h"

Index::Index()
{}

Index::~Index()
{}

void Index::setFilename(const stdString &full_name)
{
    // Split filename    
    Filename::getDirname(full_name, dirname);
    Filename::getBasename(full_name, filename);
    // and reassemble to get canonical form
    Filename::build(dirname, filename, fullname);
}

void Index::clearFilename()
{
    filename.assign(0, 0);
    dirname.assign(0, 0);
}

Index::Result::Result(RTree *tree, const stdString &directory)
    : tree(tree), directory(directory)
{
    LOG_ASSERT(tree != 0);
}

Index::Result::~Result()
{
    delete tree;
    tree = 0;
}

Index::NameIterator::~NameIterator()
{}