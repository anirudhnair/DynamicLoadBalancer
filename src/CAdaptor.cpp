/*
 * CAdapter.cpp
 *
 *  Created on: Apr 14, 2015
 *      Author: Anirudh Jayakumar
 */

#include "CAdaptor.h"
#include "Common.h"
#include <cstdlib>
#include <iostream>
using namespace std;
#define JOB_DIFF 2

CAdaptor::CAdaptor() {
	// TODO Auto-generated constructor stub
}

void CAdaptor::StopThread()
{
	stopThread = true;
}

CAdaptor::~CAdaptor() {
	// TODO Auto-generated destructor stub
}


int CAdaptor::Initialize(CStateManager *pStateManager, CTransferManager *pTransferManager,configInfo *config)
{
	stopThread = false;
	ePolicy = e_SenderInitialted;
	m_pConfig = config;
	m_pStateManager = pStateManager;
	m_pTransferManager = pTransferManager;
	if ( m_pConfig->transfer_policy_type == "sender")
		ePolicy = e_SenderInitialted;
	else if (m_pConfig->transfer_policy_type == "receiver")
		ePolicy = e_ReceiverInitiated;
	else if (m_pConfig->transfer_policy_type == "symmetric")
		ePolicy = e_Symmetric;

	if ( m_pConfig->transfer_policy_algo == "jobCount")
		eAlgo = e_JobCount;
	else if (m_pConfig->transfer_policy_algo == "jobCompletion")
		eAlgo = e_JobCompletion;
	else if (m_pConfig->transfer_policy_algo == "advanced")
		eAlgo = e_Advanced;

	m_thread = new std::thread(&CAdaptor::Start, this);
	return SUCCESS;
}

void CAdaptor::Start() // thread or event timers
{
	while(!stopThread)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(m_pConfig->transfer_policy_period));
		if (m_pConfig->myNodeId == 0)
		{
			if( SUCCESS == CheckIfJobsDone())
			{
                                system("date");
				m_pTransferManager->RequestCompletedJobsFromRemote();
				stopThread = true;
			}
		}
		TransferPolicy();
	}
	return;
}

int CAdaptor::CheckIfJobsDone()
{
	State myState = m_pStateManager->GetMyState();
	State remoteState = m_pStateManager->GetRemoteState();
	int nJobsCompleted = myState.nJobsCompleted + remoteState.nJobsCompleted;

	if( nJobsCompleted >= m_pConfig->nJobs)
	{
		cout << "Detected job completion. Triggering aggregation" << endl;
		return SUCCESS;
	}
	return FAIL;
}

int CAdaptor::TransferPolicy()
{
	//get state information
	State myState = m_pStateManager->GetMyState();
	State remoteState = m_pStateManager->GetRemoteState();
	if (eAlgo == e_JobCount)
	{
		if(ePolicy == e_SenderInitialted)
		{
			int diff = myState.nJobsPending - remoteState.nJobsPending;
			if( diff > JOB_DIFF)
			{
				int nJobtoSend = diff/2; // send half of the jobs to make both sides equal
				m_pTransferManager->SendJobsToRemote(nJobtoSend);
			}
		}
		else if(ePolicy == e_ReceiverInitiated)
		{
			int diff = remoteState.nJobsPending - myState.nJobsPending;
			if( diff > JOB_DIFF)
			{
				int nJobtoReceive = diff/2; // ask for half of the jobs to make both sides equal
				m_pTransferManager->RequestJobsFromRemote(nJobtoReceive);
			}
		}
		else if(ePolicy == e_Symmetric)
		{
			// in symmetric since both parties participate, each party
			// will do half the work. i.e if there is a load imbalance
			// each node will take care of balancing it by half.
			int diff = myState.nJobsPending - remoteState.nJobsPending;
			if (diff > JOB_DIFF)
			{
				int nJobtoSend = diff/4; //divided by 4;half the work
				m_pTransferManager->SendJobsToRemote(nJobtoSend);

			}
			else if(diff < -JOB_DIFF)
			{
				int nJobtoReceive = -diff/4; //divided by 4;half the work
				m_pTransferManager->RequestJobsFromRemote(nJobtoReceive);
			}
		}
	}
	else if(eAlgo == e_JobCompletion)
	{
		// find total time taken to complete one job. adding time_to_run + wait_time
		double remoteOneJobTotalTime = remoteState.timeForOneJob +
					( (1 - remoteState.timeForOneJob*remoteState.fThrottleVal) / (1 - remoteState.fThrottleVal) );
		double localOneJobTotalTime  = myState.timeForOneJob +
				( (1 - myState.timeForOneJob*myState.fThrottleVal)/(1 - myState.fThrottleVal));

		// find total completion time of jobs
		double remoteCompletionTime = remoteState.nJobsPending*(remoteOneJobTotalTime);
		double localCompletionTime  = myState.nJobsPending*(localOneJobTotalTime);

		if(ePolicy == e_SenderInitialted)
		{
			//only when local completion time is greater than remote
			double diff = localCompletionTime - remoteCompletionTime;
			if( diff > 0)
			{
				int nExtraJobs = diff/localOneJobTotalTime; //extra jobs in local
				// the jobs we send to remote should take almost the same time as the pending jobs local.
				// Algebra gives the below.
				int nJobsToSend = (nExtraJobs*localOneJobTotalTime) / (remoteOneJobTotalTime + localOneJobTotalTime);
				m_pTransferManager->SendJobsToRemote(nJobsToSend);
			}
		}
		else if(ePolicy == e_ReceiverInitiated)
		{
			int diff = remoteCompletionTime - localCompletionTime;
			if( diff > 0)
			{
				int nExtraJobs = diff/remoteOneJobTotalTime;
				// the jobs we receive from remote should take almost the same time as the pending jobs in remote.
				// Algebra gives the below.
				int nJobtoReceive = (nExtraJobs*remoteOneJobTotalTime) / (remoteOneJobTotalTime + localOneJobTotalTime);
				m_pTransferManager->RequestJobsFromRemote(nJobtoReceive);
			}
		}
		else if(ePolicy == e_Symmetric)
		{
			// in symmetric since both parties participate, each party
			// will do half the work. i.e if there is a load imbalance
			// each node will take care of balancing it by half.
			double diff = localCompletionTime - remoteCompletionTime;
			if (diff > 0)
			{
				// same as e_SenderInitialted, but only half jobs send
				int nExtraJobs = diff/localOneJobTotalTime;
				int nJobsToSend = (nExtraJobs*localOneJobTotalTime) / (remoteOneJobTotalTime + localOneJobTotalTime);
				m_pTransferManager->SendJobsToRemote(nJobsToSend/2); // divide by 2

			}
			else if(diff < 0)
			{
				// same as e_ReceiverInitiated, but only half jobs requested
				int nExtraJobs = diff/remoteOneJobTotalTime;
				int nJobtoReceive = (nExtraJobs*remoteOneJobTotalTime) / (remoteOneJobTotalTime + localOneJobTotalTime);
				m_pTransferManager->RequestJobsFromRemote(nJobtoReceive/2); //divide by 2
			}
		}
	}
	else if(eAlgo == e_Advanced)
	{
		// no idea what goes in here. may be network related policy
	}
	return SUCCESS;
}

