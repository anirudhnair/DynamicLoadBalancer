/*
 * CWorker.cpp
 *
 *  Created on: Apr 14, 2015
 *      Author: Anirudh Jayakumar
 */

#include "CWorker.h"
#include "Common.h"
#include <sys/time.h>
#include <iostream>
using namespace std;
CWorker::CWorker() {
    // TODO Auto-generated constructor stub
    stopThread = false;
}

void CWorker::StopThread()
{
    stopThread = true;
}

CWorker::~CWorker() {
    // TODO Auto-generated destructor stub
}

int CWorker::Initialize(CJobQueue 	*jobQueue, CHWMonitor 	*monitor, configInfo * config)
{
    pJobQueue = jobQueue;
    pMonitor = monitor;

    for (int i = 0; i < config->worker; ++i)
    {
        cout << "Worker " << i << " created" << endl;
        p_thread[i] = new std::thread(&CWorker::Start, this);
    }
    return SUCCESS;
}

int CWorker::Start()
{
    //execute two times and calculate average
    int count = 0;
    long totaltime = 0;
    double avgTime = 0;
    struct timeval start, end;
    long mtime, seconds, useconds;
    double sleep_time = 0;
    while (count < 2)
    {
        CJob *pJob = pJobQueue->GetNextJob();
        if(pJob == NULL)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        gettimeofday(&start, NULL);
        pJob->ExecJob();
        gettimeofday(&end, NULL);
        seconds  = end.tv_sec  - start.tv_sec;
        useconds = end.tv_usec - start.tv_usec;
        mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
        totaltime+=mtime;
        pJobQueue->AddNewJobTime(double(mtime));
        pJobQueue->AddCompletedJob(pJob);
        count++;
    }
    avgTime = (double)totaltime/2;
    pJobQueue->SetTimeForOneJob(avgTime);
    cout <<  "Jobs to calculate avg time done. Avg time=" << avgTime << endl;
    while(!stopThread) {
        CJob *pJob = pJobQueue->GetNextJob();
        if(pJob == NULL)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        gettimeofday(&start, NULL);
        pJob->ExecJob();
        sleep_time = ((1/pMonitor->GetThrottlingValue()) - 1) * avgTime;
        //cout << "sleep:" << sleep_time << endl;
        std::this_thread::sleep_for(std::chrono::milliseconds((long)sleep_time));
        gettimeofday(&end, NULL);
        seconds  = end.tv_sec  - start.tv_sec;
        useconds = end.tv_usec - start.tv_usec;
        mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
        pJobQueue->AddNewJobTime(double(mtime));
        pJobQueue->AddCompletedJob(pJob);
        if(pJob->GetID() % 10 == 0)
            cout << "Finished processing job " << pJob->GetID() << endl;
    }
    //async call
    return SUCCESS;
}



int CWorker::UnInitialize()
{
    return SUCCESS;
}

