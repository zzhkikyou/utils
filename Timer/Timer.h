#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>

struct Test
{
    std::string str;
};

enum class TIMER_EVENT : uint8_t {
    EVENT_ACTIVE_STOP = 1, //主动停止定时
    EVENT_TIMEOUT_STOP,    //达到定时时间，停止定时
    EVENT_REPEAT_TIMING,   //当前定时正在进行，不允许重复添加
};

//定时器用户参数  ---后续增加，减少，修改入参， 该这里
class CTimerUserArgs
{
public:
    explicit CTimerUserArgs(std::unique_ptr<Test> &upTest) : m_upTest(std::move(upTest)) {}
    ~CTimerUserArgs()= default;

    CTimerUserArgs(const CTimerUserArgs &) = delete;
    CTimerUserArgs &operator=(const CTimerUserArgs &) = delete;

public:
    std::unique_ptr<Test> m_upTest;
};


//定时器基本单元
class CTimerUnit
{
    friend class CTimer;

public:
    explicit CTimerUnit(const uint32_t uKey) : m_uKey(uKey) {}
    ~CTimerUnit()= default;

    CTimerUnit(const CTimerUnit &) = delete;
    CTimerUnit &operator=(const CTimerUnit &) = delete;

private:
    TIMER_EVENT Wait(int64_t dwMilllSecond =-1)
    {
        if(dwMilllSecond == -1)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while(!m_bNotify)
            {
                m_cond.wait(lock);
            }
            m_bNotify = false;
            return TIMER_EVENT::EVENT_ACTIVE_STOP;
        }
        else
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_cond.wait_for(lock, std::chrono::milliseconds(dwMilllSecond), [this] { return m_bNotify; }))
            {
                m_bNotify = false;
                return TIMER_EVENT::EVENT_ACTIVE_STOP;
            }
            else
            {
                return TIMER_EVENT::EVENT_TIMEOUT_STOP;
            }
        }
    }

    int32_t Notify()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_bNotify = true;
        m_cond.notify_one();
        return 0;
    }

private:
    volatile bool                    m_bNotify = false;
    uint32_t                         m_uKey;
    std::mutex                       m_mutex;
    std::condition_variable          m_cond;
private:    
    std::unique_ptr<CTimerUserArgs>  m_upTimerUserArgs; //用户参数
};

//定时器  目前仅支持同步定时
class CTimer
{
public:
    CTimer() = default;
    ~CTimer() = default;

    CTimer(const CTimer &) = delete;
    CTimer &operator=(const CTimer &) = delete;

    /**
     * 开始同步定时
     * @param uKey[IN] 定时条件唯一标识，同一标识不能同时调用此接口
     * @param iMillSecond[IN] 定时时间，单位毫秒，-1表示永久等待
     * @param upTimerUserArgs[OUT] 用户参数，由停止定时接口设置，只有主动停止时才有效
     * @return 定时器事件，如果在定时时间内调用停止定时，则返回“主动停止事件”，若超过定时时间，则返回“定时器时间到”
     * @throw Exception 内存错误
     * @note 此接口为同步等待，直到调用停止定时或者定时时间到达才返回
     * */
    TIMER_EVENT StartSyncTiming(const uint32_t uKey, const int64_t iMillSecond, std::unique_ptr<CTimerUserArgs> &upTimerUserArgs)
    {
        TIMER_EVENT event;

        std::unique_lock<std::mutex> lock(m_TimerMutex);
        std::unique_ptr<CTimerUnit> &spTimerUnit = m_TimerUnitMap[uKey];
        if (!spTimerUnit)
        {
            spTimerUnit.reset(new CTimerUnit(uKey));
            lock.unlock();
            event = spTimerUnit->Wait(iMillSecond);
            lock.lock();
            upTimerUserArgs = std::move(spTimerUnit->m_upTimerUserArgs);
            m_TimerUnitMap.erase(spTimerUnit->m_uKey);
            spTimerUnit.reset();//需要显示释放
        }
        else
        {
            event = TIMER_EVENT::EVENT_REPEAT_TIMING;
        }
        return event;
    }

    /**
     * 停止定时
     * @param uKey 定时条件唯一标识，同一标识不能同时调用此接口
     * @param upTimerUserArgs 用户参数
     * @return 0 成功，非0 失败
     * */
    int32_t StopTiming(const uint32_t uKey, std::unique_ptr<CTimerUserArgs> &&upTimerUserArgs)
    {
        int iRet = -1;
        std::unique_lock<std::mutex> lock(m_TimerMutex);
        if(m_TimerUnitMap.find(uKey) != m_TimerUnitMap.end())
        {
            std::unique_ptr<CTimerUnit> &spTimerUnit = m_TimerUnitMap[uKey];
            if(spTimerUnit)
            {
                spTimerUnit->m_upTimerUserArgs = std::move(upTimerUserArgs);
                iRet = spTimerUnit->Notify();
            }
        }

        return iRet;
    }

private:
    std::mutex m_TimerMutex;
    std::map<uint32_t, std::unique_ptr<CTimerUnit>> m_TimerUnitMap;
};

#endif