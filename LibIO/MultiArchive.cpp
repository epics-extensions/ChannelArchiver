// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "MultiArchive.h"
#include "MultiChannelIterator.h"
#include "MultiValueIterator.h"
#include "BinArchive.h"
#include "ArchiveException.h"
#include "LowLevelIO.h"
#include <ASCIIParser.h>
#include <BinaryTree.h>

// Open a MultiArchive for the given master file
MultiArchive::MultiArchive(const stdString &master_file)
{
    _queriedAllArchives = false;
    if (! parseMasterFile(master_file))
        throwDetailedArchiveException(OpenError, master_file);
}

ChannelIteratorI *MultiArchive::newChannelIterator() const
{   return new MultiChannelIterator((MultiArchive *)this); }

ValueIteratorI *MultiArchive::newValueIterator() const
{   return new MultiValueIterator(); }

ValueI *MultiArchive::newValue(DbrType type, DbrCount count)
{   return 0; }

bool MultiArchive::parseMasterFile(const stdString &master_file)
{
    // Carefully check if "master_file" starts with the magic line:
    LowLevelIO file;
    if (! file.llopen (master_file.c_str()))
    {
        LOG_MSG("Cannot open master file '" << master_file << "'\n");
        return false;
    }
    char line[14]; // master_version  : 14 chars
    if (! file.llread(line, 14))
    {
        LOG_MSG("Invalid master file '" << master_file << "'\n");
        return false; // too small to be anything
    }
    file.llclose();
    if (strncmp(line, "master_version", 14) !=0)
    {
        // Could be Binary file:
        _archives.push_back(master_file);
        return true;
    }

    // ------------------------------------------------
    // OK, looks like a master file.
    // Now read it again as ASCII:
    ASCIIParser parser;
    if (! parser.open(master_file))
        return false;

    stdString parameter, value;
    if (parser.nextLine()                      &&
        parser.getParameter(parameter, value)  &&
        parameter == "master_version"          &&
        value == "1")
    {
        while (parser.nextLine())
            _archives.push_back (parser.getLine());
    }
    else
    {
        LOG_MSG("Invalid master file '" << master_file
                << "', maybe wrong version\n");
        return false;
    }
#ifdef DEBUG_MULTIARCHIVE
    LOG_MSG("MultiArchive::parseMasterFile\n");
#endif

    return true;
}

void MultiArchive::log() const
{
    LOG_MSG("MultiArchive:\n");
    stdList<stdString>::const_iterator archs = _archives.begin();
    
    while (archs != _archives.end())
    {
        LOG_MSG("Archive file: " << *archs << "\n");
        ++archs;
    }

    LOG_MSG("Channels:\n");
    stdVector<ChannelInfo>::const_iterator chans = _channels.begin();
    while (chans != _channels.end())
    {
        LOG_MSG(chans->_name << ": "
                << chans->_first_time << ", "
                << chans->_last_time << "\n");
        ++chans;
    }
}

// Get/insert index of ChannelInfo for given name
size_t MultiArchive::getChannelInfoIndex(const stdString &name)
{
    size_t i;
    for (i=0; i < _channels.size(); ++i)
        if (_channels[i]._name == name)   // found
            return i;
    ChannelInfo new_info;
    new_info._name = name;
    _channels.push_back(new_info);
    return _channels.size()-1;
}

// Fill _channels from _archives
void MultiArchive::queryAllArchives()
{
    if (_queriedAllArchives)
        return;

#ifdef DEBUG_MULTIARCHIVE
    LOG_MSG("MultiArchive::queryAllArchives\n");
#endif
    
    stdString name;
    size_t i;
    ChannelInfo *info;
    osiTime time;

    stdList<stdString>::iterator archs = _archives.begin();
    for (/**/; archs != _archives.end(); ++archs)
    {
        Archive archive (new BinArchive (*archs));
        ChannelIterator channel (archive);
        archive.findFirstChannel (channel);
        while (channel)
        {
            name = channel->getName();
            i = getChannelInfoIndex(name);
            info = &_channels[i];
            // check bounds
            time = channel->getFirstTime();
            if (isValidTime(time) &&
                (!isValidTime(info->_first_time) ||
                 info->_first_time > time))
                info->_first_time = time;
            time = channel->getLastTime();
            if (isValidTime(time) &&
                info->_last_time < time)
                info->_last_time = time;
            ++ channel;
        }
    }
    _queriedAllArchives = true;
}

bool MultiArchive::findFirstChannel(ChannelIteratorI *channel)
{
    MultiChannelIterator *multi_channel_iter =
        dynamic_cast<MultiChannelIterator *>(channel);
    queryAllArchives();
    return multi_channel_iter->assign(this, 0);
}

bool MultiArchive::findChannelByName(const stdString &name,
                                     ChannelIteratorI *channel)
{
    MultiChannelIterator *multi_channel_iter =
        dynamic_cast<MultiChannelIterator *>(channel);
    size_t i;

    if (_queriedAllArchives)
    {
        for (i=0; i < _channels.size(); ++i)
            if (_channels[i]._name == name)
                return multi_channel_iter->assign(this, i);
    }
    else
    {
        stdList<stdString>::iterator archs = _archives.begin();
        for (/**/; archs != _archives.end(); ++archs)
        {
            Archive archive (new BinArchive(*archs));
            ChannelIterator channel(archive);
            if (archive.findChannelByName(name, channel))
            {
                i = getChannelInfoIndex(name);
                return multi_channel_iter->assign(this, i);
            }
        }         
    }
    
    multi_channel_iter->clear();
    return false;
}

bool MultiArchive::findChannelByPattern(const stdString &regular_expression,
                                        ChannelIteratorI *channel)
{
    MultiChannelIterator *multi_channel =
        dynamic_cast<MultiChannelIterator *>(channel);
    queryAllArchives();
    return findFirstChannel(channel) &&
        multi_channel->moveToMatchingChannel(regular_expression);
}

bool MultiArchive::addChannel(const stdString &name,
                              ChannelIteratorI *channel)
{
    throwDetailedArchiveException(Invalid,
                                  "Cannot write, MultiArchive is read-only");
    return false;
}

// See comment in header file
bool MultiArchive::getValueAtOrAfterTime(
    MultiChannelIterator &channel_iterator,
    const osiTime &time, bool exact_time_ok,
    MultiValueIterator &value_iterator) const
{
    if (channel_iterator._channel_index >= _channels.size())
        return false;

    const ChannelInfo &info = _channels[channel_iterator._channel_index];
    stdList<stdString>::const_iterator archs = _archives.begin();
    for (/**/; archs != _archives.end(); ++archs)
    {
        Archive archive (new BinArchive(*archs));
        ChannelIterator channel(archive);
        if (archive.findChannelByName(info._name, channel))
        {
            // getValueAfterTime() could succeed for '=='.
            // Do we allow '=='?
            if (!exact_time_ok && channel->getLastTime() <= time)
                continue;
            ValueIterator value(archive);
            // Does this archive have values after "time"?
            if (! channel->getValueAfterTime(time, value))
                continue;
            // position() will delete ref's to previous
            // ArchiveI, ChannelIteratorI, ...
            // -> remove ref. to values first, then channeliterator/archive
            value_iterator.position(&channel_iterator, value.getI());
            channel_iterator.position(archive.getI(), channel.getI());

            value.detach();    // Now ref'd by MultiValueIterator
            archive.detach();   // Now ref'd by MultiChannelIterator
            channel.detach();   // dito
            return value_iterator.isValid();
        }                    
    }
    return false;
}

bool MultiArchive::getValueAtOrBeforeTime(
    MultiChannelIterator &channel_iterator,
    const osiTime &time, bool exact_time_ok,
    MultiValueIterator &value_iterator) const
{
    if (channel_iterator._channel_index >= _channels.size())
        return false;

    const ChannelInfo &info = _channels[channel_iterator._channel_index];
    // Note: searches archives in reverse order
    // to be more predictable when going back and forth
    stdList<stdString>::const_reverse_iterator archs = _archives.rbegin();
    for (/**/; archs != _archives.rend(); ++archs)
    {
        Archive archive (new BinArchive(*archs));
        ChannelIterator channel(archive);
        if (archive.findChannelByName(info._name, channel))
        {
            // getValueBeforeTime() could succeed for '=='.
            // Do we allow '=='?
            if (!exact_time_ok && channel->getFirstTime() >= time)
                continue;
            ValueIterator value(archive);
            // Does this archive have values before "time"?
            if (! channel->getValueBeforeTime(time, value))
                continue;
            // position() will delete ref's to previous
            // ArchiveI, ChannelIteratorI, ...
            // -> remove ref. to values first, then channeliterator/archive
            value_iterator.position(&channel_iterator, value.getI());
            channel_iterator.position(archive.getI(), channel.getI());

            value.detach();    // Now ref'd by MultiValueIterator
            archive.detach();   // Now ref'd by MultiChannelIterator
            channel.detach();   // dito
            return value_iterator.isValid();
        }                    
    }
    return false;
}

bool MultiArchive::getValueNearTime(
    MultiChannelIterator &channel_iterator,
    const osiTime &time, MultiValueIterator &value_iterator) const
{
    if (channel_iterator._channel_index >= _channels.size())
        return false;

    // Query all archives in MultiArchive for value nearest "time"
    double t = double (time);
    double best_bet = -1.0; // negative == invalid

    const ChannelInfo &info = _channels[channel_iterator._channel_index];
    stdList<stdString>::const_iterator archs = _archives.begin();
    for (/**/; archs != _archives.end(); ++archs)
    {
        Archive archive (new BinArchive(*archs));
        ChannelIterator channel(archive);
        if (archive.findChannelByName(info._name, channel))
        {
            ValueIterator value(archive);
            if (! channel->getValueNearTime(time, value))
                continue;

            double distance = fabs(double(value->getTime()) - t);  
            if (best_bet >= 0  &&  best_bet <= distance)
                continue; // worse than what we found before

            best_bet = distance;
            value_iterator.position(&channel_iterator, value.getI());
            channel_iterator.position(archive.getI(), channel.getI());
            value.detach ();    // Now ref'd by MultiValueIterator
            archive.detach();   // Now ref'd by MultiChannelIterator
            channel.detach();   // dito
        }                    
    }

    if (best_bet >= 0)
        return value_iterator.isValid ();
    return false;
}



