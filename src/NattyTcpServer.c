/*
 *  Author : WangBoJing , email : 1989wangbojing@gmail.com
 * 
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Author. (C) 2016
 * 
 *
 
****       *****
  ***        *
  ***        *                         *               *
  * **       *                         *               *
  * **       *                         *               *
  *  **      *                        **              **
  *  **      *                       ***             ***
  *   **     *       ******       ***********     ***********    *****    *****
  *   **     *     **     **          **              **           **      **
  *    **    *    **       **         **              **           **      *
  *    **    *    **       **         **              **            *      *
  *     **   *    **       **         **              **            **     *
  *     **   *            ***         **              **             *    *
  *      **  *       ***** **         **              **             **   *
  *      **  *     ***     **         **              **             **   *
  *       ** *    **       **         **              **              *  *
  *       ** *   **        **         **              **              ** *
  *        ***   **        **         **              **               * *
  *        ***   **        **         **     *        **     *         **
  *         **   **        **  *      **     *        **     *         **
  *         **    **     ****  *       **   *          **   *          *
*****        *     ******   ***         ****            ****           *
                                                                       *
                                                                      *
                                                                  *****
                                                                  ****


 *
 */

#include "NattyTcpServer.h"
#include "NattyThreadPool.h"
#include "NattyFilter.h"
#include "NattyHash.h"
#include "NattyHBD.h"
#include "NattyConfig.h"
#include "NattyUtils.h"
#include "NattySession.h"
#include "NattyResult.h"
#include "NattyMessage.h"

#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ev.h>
#include <errno.h>
#include <netinet/tcp.h> 

#include <time.h>


#define JEMALLOC_NO_DEMANGLE 1
#define JEMALLOC_NO_RENAME	 1
#include <jemalloc/jemalloc.h>

struct ev_loop *tcp_mainloop = NULL;


static int ntySetNonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}

static int ntySetReUseAddr(int fd) {
	int reuse = 1;
	return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
}

static int ntySetAlive(int fd) {
	int alive = 1;	// ����keepalive����
	int idle = 60;	// ���������60����û���κ���������,�����̽��
	int interval = 5;	// ̽��ʱ������ʱ����Ϊ5 ��
	int count = 2;	// ̽�Ⳣ�ԵĴ���.�����1��̽������յ���Ӧ��,���2�εĲ��ٷ�

	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&alive, sizeof(alive));
	setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&idle, sizeof(idle));
	setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void*)&interval, sizeof(interval));
	setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*)&count, sizeof(count));
}


static int ntyTcpRecv(int fd, U8 *buffer, int length, struct ev_io *watcher, struct ev_loop *loop) {
	
	int rLen = 0;

	while (1) {
#if 1
		rLen = recv(fd, buffer, length, 0);
#else
		rLen = read(fd, buffer, length);
#endif
		ntylog(" recv rLen:%d\n", rLen);
		
		if (rLen < 0) {
			if (errno == EINTR) return -1;
			if (errno == EAGAIN) {
				usleep(100);
				break;
			}
			ntylog(" ntyTcpRecv --> errno:%d\n", errno);
			break;
		} else if (rLen == 0) {

			struct sockaddr_in client_addr;
			int nSize = sizeof(struct sockaddr_in);

			extern void* ntyRBTreeInstance(void);
			extern int ntyRBTreeInterfaceDelete(void *self, C_DEVID key);
		
			getpeername(watcher->fd,(struct sockaddr*)&client_addr, &nSize); 
			
			ntylog(" %d.%d.%d.%d:%d --> Client Disconnected\n", *(unsigned char*)(&client_addr.sin_addr.s_addr), *((unsigned char*)(&client_addr.sin_addr.s_addr)+1),													
					*((unsigned char*)(&client_addr.sin_addr.s_addr)+2), *((unsigned char*)(&client_addr.sin_addr.s_addr)+3),													
					client_addr.sin_port);	
			// release client
			// search hash table for client key
#if 0	
			C_DEVID devid = ntySearchDevIdFromHashTable(&client_addr);
			if (devid == NATTY_NULL_DEVID) {
				ntylog(" DevID is not exist \n");
				//ntyReleaseClientNodeSocket(loop, watcher, watcher->fd);
				return ;
			}
	
			ntyBoardcastAllFriendsNotifyDisconnect(devid);
#endif
			break;
		} else {
			//time_t cTime = time(NULL);
			//recvLen += rLen;
			ntylog("ntyTcpRecv --> rLen : %d\n", rLen);
			return rLen;
		}
	}
	return rLen;
}

int ntyAddRelationMap(MessagePacket *msg) {
	int ret = NTY_RESULT_SUCCESS;
	void *map = ntyMapInstance();
	ClientSocket *value = (ClientSocket *)ntyMapSearch(map, msg->client->devId);
	if ( value == NULL ) {
		ClientSocket *nValue = (NValue*)malloc(sizeof(ClientSocket));
		if (nValue == NULL) return NTY_RESULT_ERROR;
		
	#if ENABLE_EV_WATCHER_MODE
		nValue->watcher = msg->watcher;
	#else
		nValue->sockfd = msg->watcher->fd;
	#endif
		nValue->connectType = msg->connectType;

		int nSize = sizeof(struct sockaddr_in);
		getpeername(msg->watcher->fd ,(struct sockaddr*)&nValue->addr, &nSize); 
		
		ret = ntyMapInsert(map, msg->client->devId, nValue);
		if (ret == NTY_RESULT_EXIST || ret == NTY_RESULT_FAILED) {
			ntylog( "ntyAddRelationMap ntyMapInsert exit\n" );
			ntyFree(nValue);
		} else if (ret == NTY_RESULT_PROCESS) { 
		// RBTree have process ,
		// should ret server is busy, and close socket
			ntylog( "ntyAddRelationMap ntyMapInsert busy\n" );
			ntyJsonCommonResult(msg->client->devId, NATTY_RESULT_CODE_BUSY);
			ntyFree(nValue);
		} else if (ret == NTY_RESULT_SUCCESS) {		
			//insert HashTable
			void *hash = ntyHashTableInstance();
			Payload payload;
			payload.id = msg->client->devId;
			ret = ntyHashTableUpdate(hash, msg->watcher->fd, &payload);		
		}else{}
			
	} else {
	#if ENABLE_EV_WATCHER_MODE	
		if (value->watcher != msg->watcher) 
	#else
		if (value->sockfd != msg->watcher->fd) 
	#endif
		{
			struct sockaddr_in client_addr;
			int nSize = sizeof(struct sockaddr_in);
			getpeername(value->watcher->fd, (struct sockaddr*)&client_addr, &nSize); 		
			ntylog(" IP Addr Have Changed from: %d.%d.%d.%d:%d -->", *(unsigned char*)(&client_addr.sin_addr.s_addr), *((unsigned char*)(&client_addr.sin_addr.s_addr)+1),													
				*((unsigned char*)(&client_addr.sin_addr.s_addr)+2), *((unsigned char*)(&client_addr.sin_addr.s_addr)+3),													
				client_addr.sin_port);
			getpeername(msg->watcher->fd, (struct sockaddr*)&client_addr, &nSize); 
			ntylog(" to:%d.%d.%d.%d:%d\n", *(unsigned char*)(&client_addr.sin_addr.s_addr), *((unsigned char*)(&client_addr.sin_addr.s_addr)+1),													
				*((unsigned char*)(&client_addr.sin_addr.s_addr)+2), *((unsigned char*)(&client_addr.sin_addr.s_addr)+3),													
				client_addr.sin_port);
	#if 1 //Add Repeat clientId, logout the other client.
			ntyJsonCommonResult(msg->client->devId, NATTY_RESULT_CODE_ERR_REPEAT_CLIENTID);
	#endif

	#if 1 //set msg->watcher->fd and client id
			void *hash = ntyHashTableInstance();
			Payload payload;
			payload.id = msg->client->devId;
			ret = ntyHashTableUpdate(hash, msg->watcher->fd, &payload);
	#endif
			
	#if ENABLE_EV_WATCHER_MODE
			//release before socket
			//ntyReleaseSocket(tcp_mainloop, value->watcher);
			//save new socket io
			value->watcher = msg->watcher;
	#else
			value->sockfd = msg->watcher->fd;
	#endif
			memcpy(&value->addr, &client_addr, nSize);			
		}
	}

	return ret;
}

int ntyDelRelationMap(C_DEVID id) {
	int ret = NTY_RESULT_SUCCESS;
	void *map = ntyMapInstance();	
	NValue * value = (NValue *)ntyMapSearch(map, id);
	if ( value != NULL ) {
		int sockfd = value->watcher->fd;
		//delete hash table
		void *hash = ntyHashTableInstance();
		ret = ntyHashTableDelete(hash, sockfd);	
		//release client socket
		ntyReleaseSocket(tcp_mainloop, value->watcher);
		//release value
		ntyFree(value);	
		ret = ntyMapDelete(map, id);
		if (ret == NTY_RESULT_PROCESS) {
			ntylog("ntyDelRelationMap rbtree have delete ret:%d", ret);
		} else if (ret == NTY_RESULT_NOEXIST) {
			ntylog("ntyDelRelationMap rbtree no exit ret:%d", ret);
		}else{
			ntylog("ntyDelRelationMap rbtree delete success ret:%d", ret);
		}
	#if 1 
		//Delete BHeap Table		
	#endif
		return ret;	
	} else {
		ntylog("ntyDelRelationMap Have No Id:%lld\n", id);
		return NTY_RESULT_NOEXIST;
	}

	return ret;
}



static void ntyTcpServerJob(Job *job) {
	if (job == NULL) return ;
	MessagePacket *msg = (RequestPacket*)job->user_data;
	void* pFilter = ntyProtocolFilterInstance();
#if 0	
	ntyProtocolFilterProcess(pFilter, msg->buffer, msg->length, msg->client);
#else
	ntyProtocolFilterProcess(pFilter, msg->buffer, msg->length, msg);
#endif
	freeRequestPacket(msg);
	ntyFree(job);
}

static void ntyReleaseClient(Job *job) {
	if (job == NULL) return ;
	RequestPacket *req = (RequestPacket*)job->user_data;
#if 0
	struct ev_loop *loop = ntyTcpServerGetMainloop();

	req->client->devId = ntySearchDevIdFromHashTable(&req->client->addr);
	if (req->client->devId == NATTY_NULL_DEVID) {
		ntylog(" DevID is not exist \n");
		ntyReleaseClientNodeSocket(loop, req->client->watcher, req->client->sockfd);
		return ;
	} 

	ntyBoardcastAllFriendsNotifyDisconnect(req->client->devId);

	if (0 == ntyReleaseClientNodeByNode(loop, req->client)) {
		ntylog("Release Client Node Success\n");
	} else {
		ntylog("Release Client Node Failed\n");
		ntyReleaseClientNodeSocket(loop, req->client->watcher, req->client->sockfd);
	}
#endif
	freeRequestPacket(req);
	free(job);
}

int ntyReleaseSocket(struct ev_loop *loop, struct ev_io *watcher) {
	if (watcher == NULL) {
		return NTY_RESULT_FAILED;
	}
	if (watcher->fd < 0) {
		return NTY_RESULT_FAILED;
	}

	ev_io_stop(loop, watcher);
	
	close(watcher->fd);
	watcher->fd = -1;
	free(watcher);

	return NTY_RESULT_SUCCESS;
}


extern void ntySelfLogoutPacket(C_DEVID id, U8 *buffer);

void ntyOnReadEvent(struct ev_loop *loop, struct ev_io *watcher, int revents) {
	U8 buffer[RECV_BUFFER_SIZE] = {0};
	int rLen = 0;
	int nSize = sizeof(struct sockaddr_in);

	if (EV_ERROR & revents) {
		ntylog("error event in read");
		return ;
	}

	memset(buffer, 0, RECV_BUFFER_SIZE);
#if 1
	rLen = recv(watcher->fd, buffer, RECV_BUFFER_SIZE, 0);
	ntylog("ntyOnReadEvent recv after rLen=%d,errno:%d\n",rLen,errno);
	//ntydbg("ntyOnReadEvent recv after rLen=%d,errno:%d\n",rLen,errno);
#else
	rLen = ntyTcpRecv(watcher->fd, buffer, RECV_BUFFER_SIZE, watcher, loop);
#endif
	if (rLen < 0) {
		
		ntylog("ntyOnReadEvent recv after rLen<0:%d,errno:%d,str:%s\n",rLen,errno,strerror(errno));
		//ntydbg("ntyOnReadEvent recv after rLen<0:%d,errno:%d\n",rLen,errno);
		if (errno == EAGAIN){	// EAGAIN /* Try again */
			ntylog( "ntyOnReadEvent rLen<0 errno==EAGAIN\n" );
			return;
		}
		struct sockaddr_in client_addr;
		getpeername(watcher->fd,(struct sockaddr*)&client_addr, &nSize); 
		ntylog("ntyOnReadEvent %d.%d.%d.%d:%d Recv Error\n", *(unsigned char*)(&client_addr.sin_addr.s_addr), *((unsigned char*)(&client_addr.sin_addr.s_addr)+1),													
				*((unsigned char*)(&client_addr.sin_addr.s_addr)+2), *((unsigned char*)(&client_addr.sin_addr.s_addr)+3),													
				client_addr.sin_port);
		void *hash = ntyHashTableInstance();
		Payload *load = ntyHashTableSearch(hash, watcher->fd);
		if (load == NULL) {
			ntylog("ntyOnReadEvent ntyHashTableSearch:%d\n", watcher->fd);
			return ;
		}
		ntylog("ntyOnReadEvent ntyHashTableSearch id:%lld, errno:%d\n", load->id, errno);
		//ETIMEDOUT:Connection timed out.	EBADF:Bad file number.
		//ECONNRESET:Connection reset by peer.	ENOTCONN:Transport endpoint is not connected
		if ( errno == ETIMEDOUT || errno == EBADF || errno == ECONNRESET || errno == ENOTCONN ) {
			ntylog("ntyOnReadEvent release client socket\n");			
			int ret = ntyDelRelationMap(load->id);
			if (ret == NTY_RESULT_NOEXIST) {
				ntyReleaseSocket(loop, watcher);
			}
			
			#if ENABLE_RBTREE_REPLACE_BPTREE
				ret = ntyOfflineClientRBTree(load->id);
			#else
				ret = ntyOfflineClientHeap(load->id);
			#endif
			if (ret = NTY_RESULT_SUCCESS) {
				ntylog("ntyOnReadEvent Release Client Success\n");
			}
		}	
		//ntylog("ntyOnReadEvent read error:%d,str:%s\n", errno, strerror(errno));
	} else if ( rLen == 0 ) {
		ntylog("ntyOnReadEvent recv after rLen==0,errno:%d,str:%s\n",errno,strerror(errno));
		struct sockaddr_in client_addr;
		void* pThreadPool = ntyThreadPoolInstance();
		getpeername(watcher->fd,(struct sockaddr*)&client_addr, &nSize); 
		ntylog("ntyOnReadEvent %d.%d.%d.%d:%d sock:%d Client Disconnected\n", *(unsigned char*)(&client_addr.sin_addr.s_addr), *((unsigned char*)(&client_addr.sin_addr.s_addr)+1),													
				*((unsigned char*)(&client_addr.sin_addr.s_addr)+2), *((unsigned char*)(&client_addr.sin_addr.s_addr)+3),													
				client_addr.sin_port, watcher->fd);	

		void *hash = ntyHashTableInstance();
		Payload *load = ntyHashTableSearch(hash, watcher->fd);
		if (load == NULL) {
			ntylog("ntyOnReadEvent Disconnected ntyHashTableSearch:%d\n", watcher->fd);
			ntyReleaseSocket(loop, watcher);
			return ;
		}
		//ntyHashTableDelete(hash, watcher->fd);
		ntylog("ntyOnReadEvent Client:%lld Disconnected\n", load->id);
		int ret = ntyDelRelationMap(load->id);
		if (ret == NTY_RESULT_NOEXIST) {
			ntyReleaseSocket(loop, watcher);
		}
		#if ENABLE_RBTREE_REPLACE_BPTREE
			ret = ntyOfflineClientRBTree(load->id);
		#else		
			ret = ntyOfflineClientHeap(load->id);
		#endif
		
		if (ret = NTY_RESULT_SUCCESS) {
			ntylog("ntyOnReadEvent Release Client Success\n");
		}

	} else {
		int i = 0;
		void* pThreadPool = ntyThreadPoolInstance();
		MessagePacket *msg = (MessagePacket*)allocRequestPacket();
		if (msg == NULL) {
			freeRequestPacket(msg);
			return ;
		}
		struct sockaddr_in client_addr;
		getpeername(watcher->fd,(struct sockaddr*)&client_addr, &nSize);
		ntylog("ntyOnReadEvent TcpRecv: %d.%d.%d.%d:%d,length:%d,sock:%d,msgid:%x,id:%lld\n", *(unsigned char*)(&client_addr.sin_addr.s_addr), *((unsigned char*)(&client_addr.sin_addr.s_addr)+1),													
				*((unsigned char*)(&client_addr.sin_addr.s_addr)+2), *((unsigned char*)(&client_addr.sin_addr.s_addr)+3),													
				client_addr.sin_port, rLen, watcher->fd, buffer[NTY_PROTO_MSGTYPE_IDX], *(C_DEVID*)(&buffer[NTY_PROTO_DEVID_IDX]));	
	#if 0
				int k=0;
				ntydbg("==========*******##########----------TCP-ntyOnReadEvent-begin==========*******##########----------\n");
				ntydbg("{");
				for (;k<rLen;k++) {
					ntydbg("0x%02x, ", buffer[k]);
				}
				ntydbg("0x00}\n");
				ntydbg("==========*******##########----------TCP-ntyOnReadEvent-end==========*******##########----------\n");
	#endif
		void *hash = ntyHashTableInstance();
		Payload *load = ntyHashTableSearch(hash, watcher->fd);
	#if 1
		if (load == NULL) {
			ntyU8ArrayToU64(&buffer[NTY_PROTO_DEVID_IDX], &msg->client->devId);
		} else {
			ntylog("ntyOnReadEvent TcpRecv : ntyHashTableSearch --> %lld\n", load->id);
			if (load->id == NATTY_NULL_DEVID) {
				ntyU8ArrayToU64(&buffer[NTY_PROTO_DEVID_IDX], &msg->client->devId);
			} else {
				msg->client->devId = load->id;
			}
		}
	#endif
		//req->client->ackNum = ntyU8ArrayToU32(&buffer[NTY_PROTO_ACKNUM_IDX])+1;
		msg->watcher = watcher;
		msg->connectType = PROTO_TYPE_TCP;
		msg->client->deviceType = buffer[NTY_PROTO_DEVTYPE_IDX];
		msg->length = (U16)rLen;
		msg->buffer = (U8*)malloc(rLen);
		if (msg->buffer == NULL) {
			ntylog("ntyOnReadEvent malloc Recv Buffer failed\n");
			ntyFree(msg->client);
			ntyFree(msg);
			return;
		}
		memset(msg->buffer, 0, rLen);
		memcpy(msg->buffer, buffer, rLen); 
	#if 0		
		ntyAddRelationMap(msg);
	#endif
		Job *job = (Job*)malloc(sizeof(*job));
		if (job == NULL) {
			ntylog("ntyOnReadEvent malloc Job failed\n");
			freeRequestPacket(msg);
			return;
		}
		job->job_function  = ntyTcpServerJob;
		job->user_data = msg;
		ntyThreadPoolPush(pThreadPool, job);
	}

	return;
}


void ntyOnTimeoutEvent(struct ev_loop *loop, struct ev_timer *watcher, int revents) {
	ntylog(" ntyOnTimeoutEvent --> Start\n");

	if (EV_ERROR & revents) {
		ntylog("error event in accept\n");
		return;
	}

	//ntyHeartBeatDetectTraversal(loop);	
	
	void *map = ntyMapInstance();
	
	ntyMapTraversal(map, ntyHashMapTraversal);
	
}


void ntyOnAcceptEvent(struct ev_loop *loop, struct ev_io *watcher, int revents){
	int client_fd;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	struct ev_io *ev_client = (struct ev_io *)malloc(sizeof(struct ev_io));
	if (EV_ERROR & revents) {
		ntylog("error event in accept\n");
		ntylog("revents error :%d :%s\n", errno, strerror(errno));
		return ;
	}
	
	client_fd = accept(watcher->fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd == -1) {
		ntylog("accept error :%d :%s\n", errno, strerror(errno));
		return ;
	}
	if (ntySetNonblock(client_fd) < 0) {
		ntylog("failed to set client socket to non-blocking\n");
		ntylog("ntySetNonblock error :%d :%s\n", errno, strerror(errno));
		close(client_fd);
		return ;
	}
	ntySetReUseAddr(client_fd);
	ntySetAlive(client_fd);
	
	ntylog(" %d.%d.%d.%d:%d --> New Client Connected \n", 
		*(unsigned char*)(&client_addr.sin_addr.s_addr), *((unsigned char*)(&client_addr.sin_addr.s_addr)+1),													
		*((unsigned char*)(&client_addr.sin_addr.s_addr)+2), *((unsigned char*)(&client_addr.sin_addr.s_addr)+3),													
		client_addr.sin_port);
	// insert search hash table
	#if 0 //Insert Hash table
	int ret = ntyInsertNodeToHashTable(&client_addr, 0x1);
	if (ret == 0) { 			
		ntylog("Hash Table Node Insert Success\n");
	} 
	#else
	void *hash = ntyHashTableInstance();
	Payload payload;
	payload.id = NATTY_NULL_DEVID;	
	int ret = ntyHashTableInsert(hash, client_fd, &payload);
	#endif

	ev_io_init(ev_client, ntyOnReadEvent, client_fd, EV_READ);
	ev_io_start(loop, ev_client);

	return ;
}

void *ntyTcpServerInitialize(TcpServer *server) {
	server->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->sockfd < 0) {
		ntylog("natty tcp server listen failed\n");
		return NULL;
	}

	memset(&server->addr, 0, sizeof(server->addr));
	server->addr.sin_family = AF_INET;
	server->addr.sin_addr.s_addr = INADDR_ANY;
	server->addr.sin_port = htons(NATTY_TCP_SERVER_PORT);
	if (bind(server->sockfd, (struct sockaddr*)&server->addr, sizeof(server->addr)) < 0) {
		ntydbg("natty tcp server bind failed,exit the program.\n");
		ntylog("natty tcp server bind failed,exit the program.\n");
		exit(1);
		return NULL;
	}

	return server;
}

void *ntyTcpServerRelease(TcpServer *server) {
	memset(&server->addr, 0, sizeof(server->addr));
	close(server->sockfd);
	return server;
}



void *ntyTcpServerGetMainloop(void) {
	return tcp_mainloop;
}


int ntyTcpServerProcess(const void *_self) {
	const TcpServer *server = _self;
	int reuseaddr_on;
	struct ev_io socket_accept;
	struct ev_timer hb_timer; 
	struct ev_loop *loop = ev_default_loop(0);
	
	if (listen(server->sockfd, NATTY_CONNECTION_BACKLOG) < 0) {
		ntylog("natty tcp server listen failed\n");
		return -1;
	}

	if (ntySetNonblock(server->sockfd) < 0) {
		ntylog("natty tcp set nonblock failed\n");
		return -2;
	}
	
	ntySetReUseAddr(server->sockfd);
	
	ev_io_init(&socket_accept, ntyOnAcceptEvent, server->sockfd, EV_READ);
	ev_io_start(loop, &socket_accept);
	
#if (ENABLE_NATTY_TIME_STAMP&&ENABLE_NATTY_HEARTBEAT_DETECTER)
	ev_timer_init(&hb_timer, ntyOnTimeoutEvent, NATTY_DURATION_EVENT, NATTY_DURATION_EVENT);
	ev_timer_start(loop, &hb_timer);
#else
	ev_init(&hb_timer, ntyOnTimeoutEvent);
	ev_timer_set(&hb_timer, NATTY_DURATION_EVENT, NATTY_DURATION_EVENT); 
	ev_timer_start(loop, &hb_timer);
#endif

	tcp_mainloop = loop;

#if 1
	while (1) {
		ev_loop(loop, 0);
	}
#else
	ev_run(loop, 0);
#endif
	return 0;
}


void *ntyTcpServerCtor(void *self, va_list *params) {
	return ntyTcpServerInitialize(self);
}

void *ntyTcpServerDtor(void *self) {
	return ntyTcpServerRelease(self);
}

static const TcpServerHandle ntyTcpServer = {
	sizeof(UdpServer),
	ntyTcpServerCtor,
	ntyTcpServerDtor,
	ntyTcpServerProcess,
};

static const void *pNtyTcpServer = &ntyTcpServer;
static void *pTcpServer = NULL;

int ntyTcpServerRun(const void *arg) {
	const TcpServerHandle * const *pServerConf = arg;
	
	if (arg && (*pServerConf)->process && (*pServerConf)) {
		(*pServerConf)->process(arg);
	}
	return 0;
}

void* ntyTcpServerInstance(void) {
	if (pTcpServer == NULL) {
		void *pServer = New(pNtyTcpServer);
		if ((unsigned long)NULL != cmpxchg((void*)(&pTcpServer), (unsigned long)NULL, (unsigned long)pServer, WORD_WIDTH)) {
			Delete(pServer);
		}
	}
	return pTcpServer;
}




