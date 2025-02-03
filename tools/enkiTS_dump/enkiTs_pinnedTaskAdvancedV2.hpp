//
// Created by liuya on 2/2/2025.
//

#pragma once

#include "TaskScheduler.h"

#include <stdio.h>

using namespace enki;

namespace PINNED_TASK_V2 {

static TaskScheduler adv_v2_g_TS;
static std::atomic<int32_t> g_Run;

enum class IOThreadId
{
    FILE_IO,            // more than one file io thread may be useful if handling lots of small files
    NETWORK_IO_0,
    NETWORK_IO_1,
    NETWORK_IO_2,
    NUM,
};

struct ParallelTaskSet : ITaskSet
{
    ParallelTaskSet() { m_SetSize = 10; }

    void ExecuteRange( TaskSetPartition range_, uint32_t threadnum_ ) override
    {
        bool bIsIOThread = threadnum_ >= adv_v2_g_TS.GetNumTaskThreads() - static_cast<uint32_t>(IOThreadId::NUM);
        if( bIsIOThread )
        {
            assert( false ); //for this example this is an error - but external threads can run tasksets in general
            printf(" Run %d: ParallelTaskSet on thread %d which is an IO thread\n", g_Run.load(), threadnum_);
        }
        else
        {
            printf(" Run %d: ParallelTaskSet on thread %d which is not an IO thread\n", g_Run.load(), threadnum_);
        }
    }
};

struct RunPinnedTaskLoopTask : IPinnedTask
{
    void Execute() override
    {
        while( !adv_v2_g_TS.GetIsShutdownRequested() )
        {
            adv_v2_g_TS.WaitForNewPinnedTasks(); // this thread will 'sleep' until there are new pinned tasks
            adv_v2_g_TS.RunPinnedTasks();
        }
    }
};

struct PretendDoFileIO : IPinnedTask
{
    void Execute() override
    {
        // sleep used as a 'pretend' blocking workload
        std::this_thread::sleep_for( std::chrono::milliseconds( 5000 ) );
        printf(" Run %d: PretendDoFileIO on thread %d\n", g_Run.load(), threadNum );
    }
};

struct PretendDoNetworkIO : IPinnedTask
{
    void Execute() override
    {
        // sleep used as a 'pretend' blocking workload
        std::this_thread::sleep_for( std::chrono::milliseconds( 60 ) );
        printf(" Run %d: PretendDoNetworkIO on thread %d\n", g_Run.load(), threadNum );

    }
};

static constexpr int      REPEATS            = 2;

inline void testPinnedTaskAdvancedV2()
{
    enki::TaskSchedulerConfig config;
    std::cout << "config.numTaskThreadsToCreate:"<<config.numTaskThreadsToCreate << std::endl; // 7950X 测试有：31
    // In this example we create more threads than the hardware can run,
    // because the IO threads will spend most of their time idle or blocked
    // and therefore not scheduled for CPU time by the OS
    config.numTaskThreadsToCreate += (uint32_t)IOThreadId::NUM;
    std::cout << "(uint32_t)IOThreadId::NUM ->config.numTaskThreadsToCreate:"<<config.numTaskThreadsToCreate << std::endl; // 现在有numTaskThreadsToCreate:35个线程
    adv_v2_g_TS.Initialize( config );
    std::cout << "adv_v2_g_TS.GetNumTaskThreads():"<<adv_v2_g_TS.GetNumTaskThreads() << std::endl;   // 36个，竟然比config多一个
    // in this example we place our IO threads at the end
    uint32_t theadNumIOStart = adv_v2_g_TS.GetNumTaskThreads() - (uint32_t)IOThreadId::NUM;
    std::cout << "thread num io start: " << theadNumIOStart << std::endl; // 36 - 4 = 32

    // 创建了4个持久线程
    RunPinnedTaskLoopTask runPinnedTaskLoopTasks[ (uint32_t)IOThreadId::NUM ];
    for( uint32_t ioThreadID = 0; ioThreadID < (uint32_t)IOThreadId::NUM; ++ioThreadID ) // 4个RunPinnedTaskLoopTask
    {
        runPinnedTaskLoopTasks[ioThreadID].threadNum = ioThreadID + theadNumIOStart; // 每个线程 ID 是: 32 33 34 35
        adv_v2_g_TS.AddPinnedTask( &runPinnedTaskLoopTasks[ioThreadID] );
    }


    for( g_Run = 0; g_Run< REPEATS; ++g_Run )
    {
        printf("Run %d\n", g_Run.load() );

        // set of a ParallelTaskSet
        // to demonstrate can perform work on enkiTS worker threads but WaitForNewPinnedTasks will
        // not perform work. This can be used to fully subscribe machine with enkiTS worker threads but
        // have extra IO bound threads to handle blocking tasks
        ParallelTaskSet parallelTaskSet;
        adv_v2_g_TS.AddTaskSetToPipe( &parallelTaskSet );

        // Send pretend file IO task to external thread FILE_IO
        PretendDoFileIO pretendDoFileIO;
        pretendDoFileIO.threadNum = (uint32_t)IOThreadId::FILE_IO + theadNumIOStart;
        adv_v2_g_TS.AddPinnedTask( &pretendDoFileIO );

        // Send pretend network IO tasks to external thread  NETWORK_IO_0 ... NUMEXTERNALTHREADS
        PretendDoNetworkIO pretendDoNetworkIO[ (uint32_t)IOThreadId::NUM - (uint32_t)IOThreadId::NETWORK_IO_0 ];
        for( uint32_t ioThreadID = (uint32_t)IOThreadId::NETWORK_IO_0; ioThreadID < (uint32_t)IOThreadId::NUM; ++ioThreadID )
        {
            pretendDoNetworkIO[ioThreadID-(uint32_t)IOThreadId::NETWORK_IO_0].threadNum = ioThreadID + theadNumIOStart;
            adv_v2_g_TS.AddPinnedTask( &pretendDoNetworkIO[ ioThreadID - (uint32_t)IOThreadId::NETWORK_IO_0 ] );
        }

        adv_v2_g_TS.WaitforTask( &parallelTaskSet );

        // in this example  we need to wait for IO tasks to complete before running next loop
        adv_v2_g_TS.WaitforTask( &pretendDoFileIO  );
        for( uint32_t ioThreadID = (uint32_t)IOThreadId::NETWORK_IO_0; ioThreadID < (uint32_t)IOThreadId::NUM; ++ioThreadID )
        {
            adv_v2_g_TS.WaitforTask( &pretendDoNetworkIO[ ioThreadID - (uint32_t)IOThreadId::NETWORK_IO_0 ] );
        }
    }

    // ensure runPinnedTaskLoopTasks complete by explicitly calling WaitforAllAndShutdown
    adv_v2_g_TS.WaitforAllAndShutdown();
}

}