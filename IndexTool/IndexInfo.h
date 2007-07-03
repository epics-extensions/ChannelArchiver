#ifndef INDEXINFO_H_
#define INDEXINFO_H_

// Index
#include <IndexFile.h>

/** Display info about index. */
class IndexInfo
{
public:
    IndexInfo(const stdString &index_name, int verbosity,
              const stdString &channel)
        : index_name(index_name), verbosity(verbosity)
    {
        show(channel);
    }
    
    
private:
    const stdString &index_name;
    int verbosity;
    
    void show(const stdString &channel);
    void showChannel(Index &index, const stdString &channel);
    void showBlocks(Index &index, const stdString &channel);
};


#endif /*INDEXINFO_H_*/
