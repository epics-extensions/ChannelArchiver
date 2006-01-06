// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
// Storage
#include <DataWriter.h>
#include <DataFile.h>

TEST_CASE data_writer_test()
{
    stdString index_name = "test/data_writer.index";

    TEST_DELETE_FILE(index_name.c_str());
    TEST_DELETE_FILE("test/data_writer.data");

    stdString channel_name = "fred";
    size_t samples = 10000;

    try
    {
        IndexFile index(50);
        index.open(index_name.c_str(), false);
    
        CtrlInfo info;
        info.setNumeric (2, "socks",
                         0.0, 10.0,
                         0.0, 1.0, 9.0, 10.0);
    
        DbrType dbr_type = DBR_TIME_DOUBLE;
        DbrCount dbr_count = 1;
        DataWriter::file_size_limit = 10*1024*1024;
        AutoPtr<DataWriter> writer(new DataWriter(index,
                                                  channel_name, info,
                                                  dbr_type, dbr_count, 2.0,
                                                  samples));
        writer->setDataFileNameBase("data_writer.data");
        RawValueAutoPtr data(RawValue::allocate(dbr_type, dbr_count, 1));
        RawValue::setStatus(data, 0, 0);
        size_t i;
        for (i=0; i<samples; ++i)
        {
            data->value = (double) i;
            RawValue::setTime(data, epicsTime::getCurrent());
            if (!writer->add(data))
            {
                fprintf(stderr, "Write error with value %d/%d\n",
                        i, samples);
                break;
            }   
        }
        writer = 0;
        DataFile::close_all();
        index.close();
    }
    catch (GenericException &e)
    {
        printf("Exception:\n%s\n", e.what());
        FAIL("DataWriter test failed");
    }
    TEST_MSG(1, "DataWriter test passed");
    TEST_OK;
}

#if 0
{
    ErrorInfo error_info;
    IndexFile index(50);

    if (!index.open(index_name.c_str(), true, error_info))
    {
        fprintf(stderr, "%s\n", error_info.info.c_str());
        return 0;
    }
    size_t samples = 0;
    DataReader *reader = new RawDataReader(index);
    const RawValue::Data *data =
        reader->find(channel_name, 0, error_info);
    while (data)
    {
        ++samples;
        data = reader->next(error_info);    
    }
    delete reader;
    DataFile::close_all();
    index.close();
    return samples;
}

#endif

