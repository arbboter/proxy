// NatRunner.h: interface for the CNatRunner class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_RUNNER_H_
#define _NAT_RUNNER_H_


enum NatRunResult
{
    RUN_NONE,
    RUN_DO_MORE,
    RUN_DO_FAILED
};

NatRunResult NatRunResultMax(NatRunResult value1, NatRunResult value2);

class CNatRunner
{
public:
    virtual ~CNatRunner() {};
    virtual NatRunResult Run() = 0;
};

enum NatTaskResult
{    
    NAT_TASK_IDLE,
    NAT_TASK_DO_MORE,
    NAT_TASK_DONE,
};


inline NatTaskResult Nat_TaskResultMax(NatTaskResult value1, NatTaskResult value2)
{
    if (value1 < value2)
    {
        return value2;
    }
    return value1;
}

inline bool Nat_CheckTaskDoneFrom(NatTaskResult &taskResult, NatRunResult fromRunResult)
{
    taskResult = Nat_TaskResultMax(taskResult, (NatTaskResult)fromRunResult);
    return taskResult == NAT_TASK_DONE;
}

#endif//_NAT_RUNNER_H_