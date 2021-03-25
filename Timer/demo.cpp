#include "Timer.h"
#include <bits/stdc++.h>
using namespace std;

CTimer g_Timer;


void func()
{
    cout << "Wait for Enter" << endl;
    getchar();

    unique_ptr<Test> upTest(new Test);
    upTest->str = "Hello World";
    unique_ptr<CTimerUserArgs> upTimerUserArgs(new CTimerUserArgs(upTest));

    g_Timer.StopTiming(1, std::move(upTimerUserArgs));
}

int main()
{

    thread thread_func = thread(func);

    std::unique_ptr<CTimerUserArgs> upTimerUserArgs;
    TIMER_EVENT event = g_Timer.StartSyncTiming(1, 5000, upTimerUserArgs);

    switch (event)
    {
        case TIMER_EVENT::EVENT_ACTIVE_STOP:
            cout << "Stop" << endl;
            if (upTimerUserArgs)
            {
                cout << "UserArgs: " << upTimerUserArgs->m_upTest->str << endl;
            }
            break;
        case TIMER_EVENT::EVENT_REPEAT_TIMING:
            cout << "repeat timing" << endl;
            break;
        case TIMER_EVENT::EVENT_TIMEOUT_STOP:
            cout << "Time out" << endl;
            break;
    }
    

    thread_func.join();
}