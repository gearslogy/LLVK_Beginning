#pragma once
#include <iostream>

#include "TaskScheduler.h"



struct RunPinnedTaskLoopTask : enki::IPinnedTask
{
    enki::TaskScheduler *pTs;
    void Execute() override
    {
        auto &g_TS = *pTs;
        while( !g_TS.GetIsShutdownRequested() )
        {
            g_TS.WaitForNewPinnedTasks(); // this thread will 'sleep' until there are new pinned tasks
            g_TS.RunPinnedTasks();
        }
    }
};

struct PretendDoFileIO : enki::IPinnedTask
{
    void Execute() override
    {
        // Do file IO
        std::cout << "PretendDoFileIO" << std::endl;
    }
};

void testPinnedTaskAdvanced(){

    enki::TaskScheduler g_TS;
    enki::TaskSchedulerConfig config;

    // In this example we create more threads than the hardware can run,
    // because the IO thread will spend most of it's time idle or blocked
    // and therefore not scheduled for CPU time by the OS
    config.numTaskThreadsToCreate += 1;

    g_TS.Initialize( config );

    // in this example we place our IO threads at the end
    RunPinnedTaskLoopTask runPinnedTaskLoopTasks;
    runPinnedTaskLoopTasks.pTs = &g_TS;
    runPinnedTaskLoopTasks.threadNum = g_TS.GetNumTaskThreads() - 1;
    g_TS.AddPinnedTask( &runPinnedTaskLoopTasks );

    // Send pretend file IO task to external thread FILE_IO
    PretendDoFileIO pretendDoFileIO;
    pretendDoFileIO.threadNum = runPinnedTaskLoopTasks.threadNum;
    g_TS.AddPinnedTask( &pretendDoFileIO );

    PretendDoFileIO pretendDoFileIO1;
    pretendDoFileIO1.threadNum = runPinnedTaskLoopTasks.threadNum;
    g_TS.AddPinnedTask( &pretendDoFileIO1 );


    // ensure runPinnedTaskLoopTasks complete by explicitly calling shutdown
    g_TS.WaitforAllAndShutdown();


}