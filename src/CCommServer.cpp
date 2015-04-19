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
//using namespace std;
using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

//using  boost::shared_ptr;
using  boost::shared_ptr;
using namespace  ::Comm;

class DynLBServerHandler : virtual public DynLBServerIf {
public:
  DynLBServerHandler(CCommServer *comm) {
	  m_comm = comm;
	  transferMgr = comm->GetTrsfrMgr();
	  stateMgr    = comm->GetStateMgr();
    // Your initialization goes here
  }

  void SendJobsToRemote(const int32_t size, const std::vector<std::string> & vJobs) {
    // Your implementation goes here
  	std::vector<CJob*> vJobPtr;
	for(std::vector<std::string>::const_iterator iter = vJobs.begin(); iter != vJobs.end(); ++iter)
	{
		CJob *pJob = new CJob();
		pJob->DeSerialize(iter->c_str());
		vJobPtr.push_back(pJob);
	}
	transferMgr->AddJobsToLocalQueue(vJobPtr);
	return;
  }

  void RequestJobsFromRemote(const int32_t nJobs) {
    // Your implementation goes here
    printf("RequestJobsFromRemote\n");
	transferMgr->SendJobsToRemote(nJobs);
  }

  void SendStateToRemote(const std::string& stateBlob) {
    // Your implementation goes here
    printf("SendStateToRemote\n");
	State state;
	state.DeSerialize(stateBlob.c_str());
    stateMgr->UpdateRemoteState(state);
  }

  void RequestStateFromRemote() {
    // Your implementation goes here
	stateMgr->SendStateToRemote();
    printf("RequestStateFromRemote\n");
  }

  private:
  CCommServer *m_comm;
  CTransferManager *transferMgr;
  CStateManager *stateMgr;

};

void CCommServer::Init(CTransferManager *transfer_,CStateManager *stateMgr_)
{
	transferMgr = transfer_;
	stateMgr    = stateMgr_;
	m_thread = new std::thread(&CCommServer::Start,this);
}

void CCommServer::Start()
{
    int port = 9090; //place holder
	shared_ptr<DynLBServerHandler> handler(new DynLBServerHandler(this));	
    shared_ptr<TProcessor> processor(new DynLBServerProcessor(handler));
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    server.serve();

}

int CCommServer::WaitServer()
{
	m_thread->join();
	return 0;
}


CTransferManager *CCommServer::GetTrsfrMgr() 
{
	return transferMgr;
}

CStateManager *CCommServer::GetStateMgr()
{
	return stateMgr;
}

int CCommServer::UnInit()
{
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
