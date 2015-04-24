/*
 * CTransferManager.cpp
 *
 *  Created on: Apr 14, 2015
 *      Author: Anirudh Jayakumar
 */

#include "CTransferManager.h"
#include "Common.h"
#include <iostream>
using namespace std;

CTransferManager::CTransferManager() {
	// TODO Auto-generated constructor stub
	m_pJobQueue = NULL;
	m_pProxy = NULL;
}

CTransferManager::~CTransferManager() {
	// TODO Auto-generated destructor stub
}

int CTransferManager::Initialize(CJobQueue *pJobQueue, CCommProxy *pProxy)
{
	m_pJobQueue = pJobQueue;
	m_pProxy 	= pProxy;
	return SUCCESS;
}

int CTransferManager::SendJobsToRemote(int size)
{
	JobVec vJobs = m_pJobQueue->SliceChunkFromQueue(size);
	if(vJobs.size() > 0)
	{
		cout << "Sending "  << vJobs.size() << " jobs to remote"  << endl;
		m_pProxy->SendJobsToRemote(vJobs);
	}
	else
		cout << "CTransferManager::SendJobsToRemote: SliceChunkFromQueue returned zero jobs" << endl;
	return SUCCESS;
}

int CTransferManager::SendCompletedJobsToRemote()
{
	JobVec vJobs = m_pJobQueue->GetCompletedJobs();
	if(vJobs.size() > 0)
	{
		cout << "Sending "  << vJobs.size() << " completed jobs to remote"  << endl;
		m_pProxy->SendCompletedJobsToRemote(vJobs);
	}
	else
		cout << "CTransferManager::SendCompletedJobsToRemote: SliceChunkFromQueue returned zero jobs" << endl;

	return SUCCESS;
}

int CTransferManager::AddCompletedJobsToLocalQueue(std::vector<CJob*> &vJobs)
{
	cout << "Received "  << vJobs.size() << " jobs from remote"  << endl;
	m_pJobQueue->AddCompletedJobsToQueue(vJobs);
	m_pJobQueue->ListCompletedJobs();
	return SUCCESS;
}

int CTransferManager::RequestCompletedJobsFromRemote()
{
	m_pProxy->RequestCompletedJobsFromRemote();
	return SUCCESS;
}

int CTransferManager::RequestJobsFromRemote(int size)
{
	cout << "Requesting "  << size << " jobs from remote"  << endl;
	m_pProxy->RequestJobsFromRemote(size);
	return SUCCESS;
}


int CTransferManager::AddJobsToLocalQueue(std::vector<CJob*> &vJobs)
{
	cout << "Received "  << vJobs.size() << " jobs from remote"  << endl;
	m_pJobQueue->AddJobsToQueue(vJobs);
	return SUCCESS;
}
