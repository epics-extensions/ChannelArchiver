#include "ArchiveException.h"
#include "BinCtrlInfo.h"

// Read CtrlInfo for Binary format
//
// Especially for the original engine,
// the CtrlInfo on disk can be damaged.
// Special case of a CtrlInfo that's "too small":
// For enumerated types, an empty info is assumed.
// For other types, the current info is maintained
// so that the reader can decide to ignore the problem.
// In other cases, the type is set to Invalid
void BinCtrlInfo::read(LowLevelIO &file, FileOffset offset)
{
    // read size field only
    unsigned short size;
    if (file.llseek(offset) != offset ||
        !file.llread(&size, sizeof size))
    {
        _infobuf.mem()->type = Invalid;
        throwDetailedArchiveException(
            ReadError, "Cannot read size of CtrlInfo");
        return;
    }
#ifdef CONVERSION_REQUIRED
    SHORTFromDisk(size);
#endif
    if (size < offsetof(CtrlInfoData, value) + sizeof(EnumeratedInfo))
    {
        if (getType() == Enumerated)
        {
            LOG_MSG("CtrlInfo too small: %d, "
                    "forcing to empty enum for compatibility\n", size);
            setEnumerated (0, 0);
            return;
        }
        // keep current values for _infobuf!
        char buffer[100];
        sprintf(buffer, "CtrlInfo too small: %d", size);
        throwDetailedArchiveException(Invalid, buffer);
        return;
    }

    _infobuf.reserve (size+1); // +1 for possible unit string hack, see below
    CtrlInfoData *info = _infobuf.mem();
    info->size = size;
    if (info->size > _infobuf.getBufferSize ())
    {
        info->type = Invalid;
        char buffer[100];
        sprintf(buffer, "CtrlInfo too big: %d", info->size);
        throwDetailedArchiveException(Invalid, buffer);
        return;
    }
    // read remainder of CtrlInfo:
    if (! file.llread (reinterpret_cast<char *>((void *)info) + sizeof size,
                       info->size - sizeof size))
    {
        info->type = Invalid;
        throwDetailedArchiveException (ReadError, "Cannot read remainder of CtrlInfo");
        return;
    }

    // convert rest from disk format
    SHORTFromDisk (info->type);
    switch (info->type)
    {
    case Numeric:
        //FloatFromDisk(info->value.analog.disp_high);
        //FloatFromDisk(info->value.analog.disp_low);
        //FloatFromDisk(info->value.analog.low_warn);
        //FloatFromDisk(info->value.analog.low_alarm);
        //FloatFromDisk(info->value.analog.high_warn);
        //FloatFromDisk(info->value.analog.high_alarm);
        LONGFromDisk (info->value.analog.prec);
        {
            // Hack: some old archives are written with nonterminated unit strings:
            int end = info->size - offsetof (CtrlInfoData, value.analog.units);
            for (int i=0; i<end; ++i)
            {
                if (info->value.analog.units[i] == '\0')
                    return; // OK, string is terminated
            }
            ++info->size; // include string terminator
            info->value.analog.units[end] = '\0';
        }
        break;
    case Enumerated:
        SHORTFromDisk (info->value.index.num_states);
        break;
    default:
        LOG_MSG("CtrlInfo::read @offset 0x%X:\n", (void *) offset);
        LOG_MSG("Invalid CtrlInfo, type %d, size %d\n",
                info->type, info->size);
        info->type = Invalid;
        throwDetailedArchiveException(Invalid, "Archive: Invalid CtrlInfo");
    }
}

// Write CtrlInfo to file.
void BinCtrlInfo::write (LowLevelIO &file, FileOffset offset) const
{   // Attention:
    // copy holds only the fixed CtrlInfo portion,  not enum strings etc.!
    const CtrlInfoData *info = _infobuf.mem();
    CtrlInfoData copy = *info;
    size_t converted;

    switch (copy.type) // convert to disk format
    {
    case Numeric:
        //FloatToDisk(copy.value.analog.disp_high);
        //FloatToDisk(copy.value.analog.disp_low);
        //FloatToDisk(copy.value.analog.low_warn);
        //FloatToDisk(copy.value.analog.low_alarm);
        //FloatToDisk(copy.value.analog.high_warn);
        //FloatToDisk(copy.value.analog.high_alarm);
        LONGToDisk (copy.value.analog.prec);
        converted = offsetof (CtrlInfoData, value) + sizeof (NumericInfo);
        break;
    case Enumerated:
        SHORTToDisk (copy.value.index.num_states);
        converted = offsetof (CtrlInfoData, value) + sizeof (EnumeratedInfo);
        break;
    default:
        throwArchiveException (Invalid);
    }
    SHORTToDisk (copy.size);
    SHORTToDisk (copy.type);

    if (file.llseek (offset) != offset ||
        !file.llwrite (&copy, converted))
        throwArchiveException (WriteError);

    // only the common, minimal CtrlInfoData portion was converted,
    // the remaining strings are written from 'this'
    if (info->size > converted &&
        !file.llwrite (reinterpret_cast<const char *>(info) + converted,
                       info->size - converted))
        throwArchiveException (WriteError);
}

