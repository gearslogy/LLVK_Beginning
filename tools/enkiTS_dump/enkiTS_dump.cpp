#include <TaskScheduler.h>
#include "Timer.h"
#include "enkiTS_pinnedTaskAdvanced.hpp"
#include "enkiTs_pinnedTaskAdvancedV2.hpp"
struct SimpleTask: enki::ITaskSet {
    void ExecuteRange(  enki::TaskSetPartition range, uint32_t threadnum ) override {
        printf("SimpleTask Thread %d, start %d, end %d\n", threadnum, range.start, range.end );
    }
};



void testLambada() {
    enki::TaskScheduler g_TS;
    g_TS.Initialize();

    enki::TaskSet task( 20, []( enki::TaskSetPartition range, uint32_t threadnum  ) {
        printf("lambda Thread %d, start %d, end %d\n", threadnum, range.start, range.end );
    } );

    SimpleTask st_01;
    st_01.m_SetSize = 1024;
    g_TS.AddTaskSetToPipe(&st_01);
    g_TS.AddTaskSetToPipe( &task );
    g_TS.WaitforTask( &st_01 );
    g_TS.WaitforTask( &task );

}


struct ExampleTask : enki::ITaskSet
{
    explicit ExampleTask( uint32_t size_ ) { m_SetSize = size_; }

    void ExecuteRange( enki::TaskSetPartition range_, uint32_t threadnum_ ) override
    {
        if( m_Priority == enki::TASK_PRIORITY_LOW )
        {
            // fake slow task with timer
            Timer timer;
            timer.Start();
            double tWaittime = (double)( range_.end - range_.start ) * 100.;
            while( timer.GetTimeMS() < tWaittime )
            {
            }
            printf( "\tLOW PRIORITY TASK range complete: thread: %d, start: %d, end: %d\n",
                    threadnum_, range_.start, range_.end );
        }
        else
        {
            printf( "HIGH PRIORITY TASK range complete: thread: %d, start: %d, end: %d\n",
                    threadnum_, range_.start, range_.end );
        }
    }
};
void testPriority() {
    enki::TaskScheduler g_TS;
    g_TS.Initialize();

    ExampleTask lowPriorityTask( 10 );
    lowPriorityTask.m_Priority  = enki::TASK_PRIORITY_LOW;

    ExampleTask highPriorityTask( 1 );
    highPriorityTask.m_Priority = enki::TASK_PRIORITY_HIGH;

    g_TS.AddTaskSetToPipe( &lowPriorityTask );
    for( int task = 0; task < 10; ++task )
    {
        // run high priority tasks
        g_TS.AddTaskSetToPipe( &highPriorityTask );
        // wait for task but only run tasks of the same priority on this thread
        g_TS.WaitforTask( &highPriorityTask, highPriorityTask.m_Priority );
    }
    // wait for low priority task, run any tasks on this thread whilst waiting
    g_TS.WaitforTask( &lowPriorityTask );
}


struct PinnedTaskHelloWorld : enki::IPinnedTask
{
    PinnedTaskHelloWorld()
        : IPinnedTask(0) // set pinned thread to 0
    {}
    void Execute() override
    {
        printf("This will run on the main thread\n");
    }
};

struct ParallelTaskSet : enki::ITaskSet
{
    enki::TaskScheduler *pTs{};

    PinnedTaskHelloWorld pinnedTask;
    void ExecuteRange( enki::TaskSetPartition range_, uint32_t threadnum_ ) override
    {
        auto &g_TS = *pTs;
        g_TS.AddPinnedTask( &pinnedTask );

        printf("This could run on any thread, currently thread %d\n", threadnum_);

        g_TS.WaitforTask( &pinnedTask );
    }
};

void testPinnedTaskHelloWorld() {
    const int REPEATS   = 100;
    enki::TaskScheduler g_TS;
    g_TS.Initialize();
    for( int run = 0; run< REPEATS; ++run )
    {

        ParallelTaskSet task;
        task.pTs = &g_TS;
        g_TS.AddTaskSetToPipe(&task );

        printf("Task %d added\n", run );

        // RunPinnedTasks must be called on main thread to run any pinned tasks for that thread.
        // Tasking threads automatically do this in their task loop.
        g_TS.RunPinnedTasks();
        g_TS.WaitforTask( &task);
    }
}



int main() {
    //testLambada();
    //testPriority();
    //testPinnedTaskHelloWorld();
    //testPinnedTaskAdvanced();
    PINNED_TASK_V2::testPinnedTaskAdvancedV2();
    return 0;
}