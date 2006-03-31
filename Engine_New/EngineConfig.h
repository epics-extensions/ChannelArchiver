#ifndef ENGINECONFIG_H_
#define ENGINECONFIG_H_

// Tools
#include <ToolsConfig.h>

/**\ingroup Engine
 *  Global engine configuration parameters.
 */
class EngineConfig
{
public:
    EngineConfig()
    {
        write_period = 30.0;
        buffer_reserve = 3;
        max_repeat_count = 100;
        ignored_future = 60.0;
    }
    
    /** @return Returns the period in seconds between 'writes' to Storage. */
    double getWritePeriod() const       { return write_period; }

    /** Set the period in seconds between 'writes' to Storage. */
    void setWritePeriod(double secs)    { write_period = secs; }


    /** Set how many times more buffer is reserved. */
    void getBufferReserve(size_t times) { buffer_reserve = times; }
    
    /** Set the max. repeat count.
     *  @see SampleMechanismGet
     */
    void setMaxRepeatCount(size_t count) { max_repeat_count = count; }

    /** @return Returns suggested buffer for given scan period. */
    size_t getSuggestedBufferSpace(double scan_period) const
    {
        if (scan_period <= 0)
            return 1;
        size_t num = (size_t)(write_period * buffer_reserve / scan_period);
        if (num < 3)
            return 3;
        return num;
    }
    
    /** @return Returns how many times more buffer is reserved. */
    size_t getBufferReserve() const     { return buffer_reserve; }
    
    /** @return Returns the max. repeat count.
     *  @see SampleMechanismGet
     */
    size_t getMaxRepeatCount() const    { return max_repeat_count; }
    
    
    /** @return Returns the seconds into the future considered too futuristic.
     */
    double getIgnoredFutureSecs() const { return ignored_future; }

protected:
    double write_period;
    size_t buffer_reserve;
    size_t max_repeat_count;
    double ignored_future;
};


/**\ingroup Engine
 *  Modifyable global engine configuration parameters.
 */
class WritableEngineConfig : public EngineConfig
{
public:    

};

#endif /*ENGINECONFIG_H_*/
