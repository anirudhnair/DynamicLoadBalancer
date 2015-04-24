// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "CCommServer.h"
#include "DynLBServer.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "CTransferManager.h"
#include "CStateManager.h"
#include <iostream>
//using namespace std;
using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

//using  boost::shared_ptr;
using boost::shared_ptr;
using namespace ::Comm;

class DynLBServerHandler: virtual public DynLBServerIf {
public:
	DynLBServerHandler(CCommServer *comm) {
		m_comm = comm;
		transferMgr = comm->GetTrsfrMgr();
		stateMgr = comm->GetStateMgr();
		// Your initialization goes here
	}

	void SendJobsToRemote(const int32_t size,
			const std::vector<std::string> & vJobs) {
		// Your implementation goes here
		std::cout << "Receive jobs from remote to server"  << std::endl; 
		std::vector<CJob*> vJobPtr;
		for (std::vector<std::string>::const_iterator iter = vJobs.begin();
				iter != vJobs.end(); ++iter) {
			CJob *pJob = new CJob();
			pJob->DeSerialize(iter->c_str());
			vJobPtr.push_back(pJob);
		}
		transferMgr->AddJobsToLocalQueue(vJobPtr);
		return;
	}

	void RequestJobsFromRemote(const int32_t nJobs) {
		// remote node requests for jobs. This request is forwarded
		// to transfer manager
		std::cout << "Received request to send " << nJobs << " to remote"
				<< std::endl;
		transferMgr->SendJobsToRemote(nJobs);
	}

	void SendStateToRemote(const std::string& stateBlob) {
		// server receives state from remote
		std::cout << "Received state from remote" << std::endl;
		State state;
		state.DeSerialize(stateBlob.c_str());
		stateMgr->UpdateRemoteState(state);
	}

	void RequestStateFromRemote() {
		std::cout << "Received request to send state to remote" << std::endl;
		stateMgr->SendStateToRemote();
	}

	void RequestCompletedJobsFromRemote()
	{
		transferMgr->SendCompletedJobsToRemote();
	}

        void GetStateInfo(UIState& _return) {
  		State my_state 	= stateMgr->GetMyState();
	        _return.cpu_util = my_state.dCPUUtil;
		_return.nJobsPending = my_state.nJobsPending;
		_return.nJobsCompleted = my_state.nJobsCompleted;
		_return.fThrottling =  my_state.fThrottleVal;
		// network later		
	}

  	void SetThrottling(const double throttling) {
  		stateMgr->SetThrottling(throttling);		
	}


	void SendCompletedJobsToRemote(const int32_t size,
		const std::vector<std::string> & vJobs) {
		// Your implementation goes here
		std::vector<CJob*> vJobPtr;
		for (std::vector<std::string>::const_iterator iter = vJobs.begin();
				iter != vJobs.end(); ++iter) {
			CJob *pJob = new CJob();
			pJob->DeSerialize(iter->c_str());
			vJobPtr.push_back(pJob);
		}
		transferMgr->AddCompletedJobsToLocalQueue(vJobPtr);
		return;
	}

private:
	CCommServer *m_comm;
	CTransferManager *transferMgr;
	CStateManager *stateMgr;

};

CCommServer::~CCommServer()
{
	delete m_thread;
}


void CCommServer::Init(configInfo *pConfig, CTransferManager *transfer_,
		CStateManager *stateMgr_) {
	m_pConfig = pConfig;
	transferMgr = transfer_;
	stateMgr = stateMgr_;
	m_thread = new std::thread(&CCommServer::Start, this);
}

void CCommServer::Start() {
	int port = m_pConfig->nodeInfo[m_pConfig->myNodeId].port; //place holder
	shared_ptr<DynLBServerHandler> handler(new DynLBServerHandler(this));
	shared_ptr<TProcessor> processor(new DynLBServerProcessor(handler));
	shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
	shared_ptr<TTransportFactory> transportFactory(
			new TBufferedTransportFactory());
	shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

	TSimpleServer server(processor, serverTransport, transportFactory,
			protocolFactory);
	std::cout << "Server binding on " << port << std::endl;
	server.serve();

}

int CCommServer::WaitServer() {
	m_thread->join();
	return 0;
}

CTransferManager *CCommServer::GetTrsfrMgr() {
	return transferMgr;
}

CStateManager *CCommServer::GetStateMgr() {
	return stateMgr;
}

int CCommServer::UnInit() {
	delete m_thread;
	return 0;
}
/*
 int main(int argc, char **argv) {
 int port = 9090;
 shared_ptr<DynLBServerHandler> handler(new DynLBServerHandler());
 shared_ptr<TProcessor> processor(new DynLBServerProcessor(handler));
 shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
 shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
 shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

 TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
 server.serve();
 return 0;
 }
 */
