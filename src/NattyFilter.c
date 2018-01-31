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


#include "NattyRBTree.h"
#include "NattyFilter.h"
#include "NattySession.h"
#include "NattyDaveMQ.h"
#include "NattyConfig.h"
#include "NattyHash.h"
#include "NattyNodeAgent.h"
#include "NattyDBOperator.h"
#include "NattyUtils.h"
#include "NattyMulticast.h"
#include "NattyHBD.h"
#include "NattyVector.h"
#include "NattyJson.h"

#include "NattyServAction.h"
#include "NattyMessage.h"
#include "NattyDaveMQ.h"
#include "NattyUdpServer.h"
#include "NattyPush.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <ev.h>

#define JEMALLOC_NO_DEMANGLE 1
#define JEMALLOC_NO_RENAME	 1
#include <jemalloc/jemalloc.h>


#if ENABLE_NATTY_TIME_STAMP
#include <pthread.h>
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t loop_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

extern void *ntyTcpServerGetMainloop(void);
extern int ntyReleaseSocket(struct ev_loop *loop, struct ev_io *watcher);


static void *ntyClientListCtor(void *_self, va_list *params) {
	SingleList *self = _self;
	self->head = (Node*)calloc(1, sizeof(Node));
	self->head->next = NULL;
	self->count = 0;
	
	return self;
}

static void *ntyClientListDtor(void *_self) {
	SingleList *self = _self;
	Node **p = &self->head;

	while ((*p) != NULL) {
		Node *node = *p;
		*p = node->next;
		free(node);
	}
	return self;
}


static void ntyClientListInsert(void *_self, int id) {
	SingleList *self = _self;
	Node *node = (Node*)calloc(1, sizeof(Node));
	Node **p = (Node**)&self->head;
#if 0
	for (; (*p) != NULL; p = &(*p)->next) ;

	node->data = data;
	node->next = p;
	*p = node;
#else
	self->count ++; //client list count
	node->clientId = id;
	node->next = (*p)->next;
	(*p)->next = node;
#endif
}


static int ntyClientListRemove(void *_self, int id) {
	SingleList *self = _self;
	Node **p = (Node **)&self->head;

	while ((*p) != NULL) {
		Node *node = *p;
		if (node->clientId == id) {
			*p = node->next;
			self->count --; //client list count
			free(node);

			return 0;
		} else {
			p = &(*p)->next;
		}
	}
	return 1;
}

static C_DEVID* ntyClientListIterator(const void *_self) {
	const SingleList *self = _self;
	Node **p = &self->head->next;
	C_DEVID* pClientList = (C_DEVID*)malloc(self->count * sizeof(C_DEVID));

	for (; (*p) != NULL; p = &(*p)->next) {		
		*pClientList = (*p)->clientId;
	}
	return pClientList;
}

static void ntyClientListPrint(const void *_self) {
	const SingleList *self = _self;
	Node **p = &self->head->next;

	while ((*p) != NULL) {
		//print_fn((*p)->data);
		//ntylog(" %ld ", (*p)->clientId);
		p = &(*p)->next;
	}
	ntylog("\n");
}

static const List ntyClientList = {
	sizeof(SingleList),
	ntyClientListCtor,
	ntyClientListDtor,
	ntyClientListInsert,
	ntyClientListRemove,
	ntyClientListIterator,
	ntyClientListPrint,
};

const void *pNtyClientList = &ntyClientList;

void Insert(void *self, int Id) {
	List **pListOpera = self;
	if (self && (*pListOpera) && (*pListOpera)->insert) {
		(*pListOpera)->insert(self, Id);
	}
}

int Remove(void *self, int Id) {
	List **pListOpera = self;
	if (self && (*pListOpera) && (*pListOpera)->remove) {
		return (*pListOpera)->remove(self, Id);
	}
	return -1;
}

U64* Iterator(void *self) {
	List **pListOpera = self;
	if (self && (*pListOpera) && (*pListOpera)->iterator) {
		return (*pListOpera)->iterator(self);
	}
	return NULL;
}



void *ntyPacketCtor(void *_self, va_list *params) {
	Packet *self = _self;
	return self;
}


void *ntyPacketDtor(void *_self) {
	Packet *self = _self;
	self->succ = NULL;
	
	return self;
}

void ntyPacketSetSuccessor(void *_self, void *_succ) {
	Packet *self = _self;
	self->succ = _succ;
}

void* ntyPacketGetSuccessor(const void *_self) {
	const Packet *self = _self;
	return self->succ;
}

static int ntyUpdateClientListIpAddr(UdpClient *client, C_DEVID key, U32 ackNum) {
	int i = 0;
	UdpClient *pClient = client;
	void *pRBTree = ntyRBTreeInstance();

	C_DEVID* friendsList = Iterator(pClient->friends);
	int count = ((SingleList*)pClient->friends)->count;
	for (i = 0;i < count;i ++) {
#if 1
		UdpClient *cv = ntyRBTreeInterfaceSearch(pRBTree, *(friendsList+i));
		if (cv != NULL) {
			
			U8 ackNotify[NTY_P2P_NOTIFY_ACK_LENGTH] = {0};
			//*(C_DEVID*)(&ackNotify[NTY_PROTO_P2P_NOTIFY_DEVID_IDX]) = key;
			//*(U32*)(&ackNotify[NTY_PROTO_P2P_NOTIFY_ACKNUM_IDX]) = ackNum;
#if 0			
			*(U32*)(&ackNotify[NTY_PROTO_P2P_NOTIFY_IPADDR_IDX]) = client->addr.sin_addr.s_addr;
			*(U16*)(&ackNotify[NTY_PROTO_P2P_NOTIFY_IPPORT_IDX]) = client->addr.sin_port;
#endif
			//ntyP2PNotifyClient(cv, ackNotify);
		}
#else
		U8 ackNotify[NTY_P2P_NOTIFY_ACK_LENGTH] = {0};
		*(U32*)(&ackNotify[NTY_PROTO_P2P_NOTIFY_DEVID_IDX]) = key;
		*(U32*)(&ackNotify[NTY_PROTO_P2P_NOTIFY_ACKNUM_IDX]) = ackNum;

		ntyP2PNotifyClient(cv, ackNotify);
#endif
	}
	free(friendsList);
}



/*
 * add rbtree <key, value> -- <UserId, sockfd>
 * add B+tree <key, value> -- <UserId, UserInfo>
 */
Client* ntyAddClientHeap(const void * obj, int *result) {
	const Client *client = obj;
	int ret = -1;

	//pClient->
	BPTreeHeap *heap = ntyBHeapInstance();
	if ( heap == NULL ){
		ntylog( "ntyAddClientHeap heap==NULL\n" );
		return NULL;
	}
	ntylog("ntyAddClientHeap heap->count:%d\n", heap->count);
	NRecord *record = ntyBHeapSelect( heap, client->devId );
	if ( record == NULL ){
		Client *pClient = (Client*)malloc( sizeof(Client) );
		if ( pClient == NULL ) {
			*result = NTY_RESULT_ERROR;
			ntylog( "ntyAddClientHeap pClient malloc failed\n" );
			return NULL;
		}
		memset(pClient, 0, sizeof(Client));		
		memcpy(pClient, client, sizeof(Client));
		ntylog("ntyAddClientHeap is not exist %lld\n", client->devId);
		
		//insert bheap
		ret = ntyBHeapInsert(heap, client->devId, pClient);
#if 0
		if (ret == NTY_RESULT_EXIST || ret == NTY_RESULT_FAILED) {
			ntylog("ret : %d\n", ret);
			free(pClient);
			
			*result = ret;
			return NULL;
		} else if (ret == NTY_RESULT_BUSY) {
			ntylog("ret : %d\n", ret);
			free(pClient);

			*result = ret;
			return NULL;
		}
	#else
		if ( ret != NTY_RESULT_SUCCESS ) {
			ntylog("ntyAddClientHeap ret:%d\n", ret);
			ntyFree(pClient);
			*result = ret;
			return NULL;
		}
		//select for test
		NRecord *recordTmp = ntyBHeapSelect( heap, pClient->devId );
		if ( recordTmp != NULL ){
			Client *clientTmp = (Client *)recordTmp->value;
			ntylog( "ntyAddClientHeap select:%lld\n", clientTmp->devId );
		}
		//ntyPrintTree(heap->root);  //easy error,just for debug
	#endif
		pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
		memcpy(&pClient->bMutex, &blank_mutex, sizeof(pClient->bMutex));

		
		pClient->online = 1;
		if (pClient->deviceType == NTY_PROTO_CLIENT_WATCH) {
			ntyExecuteChangeDeviceOnlineStatusHandle(pClient->devId, 1);
		}
		
		pClient->rLength = 0;
		pClient->recvBuffer = malloc( PACKET_BUFFER_SIZE );
		if ( pClient->recvBuffer == NULL ) {
			ntyFree(pClient);
			return NULL;
		}
		memset(pClient->recvBuffer, 0, PACKET_BUFFER_SIZE);

		//voice buffer
		pClient->rBuffer = NULL;
		pClient->sBuffer = NULL;

#if ENABLE_NATTY_TIME_STAMP //TIME Stamp 	
		pClient->stamp = ntyTimeStampGenrator();
#endif
		
		ntylog(" ntyAddClientHeap --> ntyVectorCreator \n");
		if (pClient->friends == NULL) {
			pClient->friends = ntyVectorCreator();
			if (pClient->friends == NULL) {
				*result = NTY_RESULT_ERROR;
				return pClient;
			}

			ntylog("ntyAddClientHeap --> friend addr:%llx\n", (C_DEVID)pClient->friends);
#if ENABLE_CONNECTION_POOL
			if (pClient->deviceType == NTY_PROTO_CLIENT_ANDROID 
				|| pClient->deviceType == NTY_PROTO_CLIENT_IOS 
				|| pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) { //App
				if(-1 == ntyQueryWatchIDListSelectHandle(pClient->devId, pClient->friends)) {
					ntylog(" ntyQueryWatchIDListSelectHandle Failed \n");
					
				}
			} else if (pClient->deviceType == NTY_PROTO_CLIENT_WATCH) { //Device
				if (-1 == ntyQueryAppIDListSelectHandle(pClient->devId, pClient->friends)) {
					ntylog(" ntyQueryAppIDListSelectHandle Failed \n");
					
				}
			} else {
				ntylog(" Protocol Device Type is Error : %c\n", pClient->deviceType);
				//free(pClient);
				//return ;
			}
#endif
		}

		if (pClient->group == NULL) {
#if 1 //Add Groups
#endif
		}
#if 0 //cancel timer
		//start timer,
		NWTimer* nwTimer = ntyTimerInstance();
		unsigned long addr = (unsigned long)pClient;
		void* timer = ntyTimerAdd(nwTimer, 60, ntyCheckOnlineAlarmNotify, (void*)&addr, sizeof(unsigned long));
		pClient->hbdTimer = timer;
#endif
		*result = NTY_RESULT_SUCCESS;
		return pClient;
	} else {
	
		ntylog("ntyAddClientHeap exist %lld\n", client->devId);
	
		Client *pClient = record->value;
		if (pClient == NULL) {
			ntylog(" ntyAddClientHeap pClient is not Exist : %lld\n", client->devId);

			*result = NTY_RESULT_NOEXIST;
			return NULL;
		}
#if ENABLE_NATTY_TIME_STAMP //TIME Stamp 	
		pClient->stamp = ntyTimeStampGenrator();
#endif
		if (pClient->online == 0 && pClient->deviceType == NTY_PROTO_CLIENT_WATCH) {
			ntyExecuteChangeDeviceOnlineStatusHandle(pClient->devId, 1);
		}
		pClient->online = 1;

		return pClient;
	}

	return NULL;
}

Client* ntyAddClientRBTree(const void * obj, int *result) {
	const Client *client = obj;
	int ret = -1;

	void *map = ntyRBTreeMapInstance();
	Client *ptrClient = (Client *)ntyMapSearch( map, client->devId );
	if ( ptrClient == NULL ){
		ntylog("ntyAddClientRBTree not exist:%lld\n", client->devId);
		Client *pClient = (Client*)malloc( sizeof(Client) );
		if ( pClient == NULL ) {
			*result = NTY_RESULT_ERROR;
			ntylog( "ntyAddClientRBTree pClient malloc failed\n" );
			return NULL;
		}
		memset( pClient, 0, sizeof(Client) );		
		memcpy( pClient, client, sizeof(Client) );
		
		//insert rbtree
		ret = ntyMapInsert( map, client->devId, pClient );
		if (ret == NTY_RESULT_EXIST || ret == NTY_RESULT_FAILED) {
			ntylog( "ntyAddClientRBTree ntyMapInsert exit\n" );
			ntyFree( pClient );
			*result = ret;
			return NULL;
		} else if (ret == NTY_RESULT_PROCESS) { 
			// RBTree have process ,
			ntylog( "ntyAddClientRBTree ntyMapInsert busy\n" );
			ntyFree( pClient );
			*result = ret;
			return NULL;
		} else if (ret == NTY_RESULT_SUCCESS) {
			ntylog( "ntyAddClientRBTree ntyMapInsert success\n" );
		}
		else{}

		pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
		memcpy( &pClient->bMutex, &blank_mutex, sizeof(pClient->bMutex) );
		
		if ( pClient->online==0 && pClient->deviceType == NTY_PROTO_CLIENT_WATCH ) {
			ntyExecuteChangeDeviceOnlineStatusHandle( pClient->devId, 1 );
		}
		pClient->online = 1;
		pClient->rLength = 0;
		pClient->recvBuffer = malloc( PACKET_BUFFER_SIZE );
		if ( pClient->recvBuffer == NULL ) {
			ntyFree( pClient );
			return NULL;
		}
		memset( pClient->recvBuffer, 0, PACKET_BUFFER_SIZE );
		//voice buffer
		pClient->rBuffer = NULL;
		pClient->sBuffer = NULL;

	#if ENABLE_NATTY_TIME_STAMP //TIME Stamp 	
		pClient->stamp = ntyTimeStampGenrator();
	#endif
		if ( pClient->friends == NULL ) {
			pClient->friends = ntyVectorCreator();
			if ( pClient->friends == NULL ) {
				*result = NTY_RESULT_ERROR;
				return pClient;
			}
			//ntylog("ntyAddClientHeap --> friend addr:%llx\n", (C_DEVID)pClient->friends);
		#if ENABLE_CONNECTION_POOL
			if (pClient->deviceType == NTY_PROTO_CLIENT_ANDROID 
				|| pClient->deviceType == NTY_PROTO_CLIENT_IOS 
				|| pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) { //App
				if(-1 == ntyQueryWatchIDListSelectHandle(pClient->devId, pClient->friends)) {
					ntylog("ntyAddClientRBTree ntyQueryWatchIDListSelectHandle Failed\n");		
				}
			} else if (pClient->deviceType == NTY_PROTO_CLIENT_WATCH) { //Device
				if (-1 == ntyQueryAppIDListSelectHandle(pClient->devId, pClient->friends)) {
					ntylog("ntyAddClientRBTree ntyQueryAppIDListSelectHandle Failed\n");				
				}
			} else {
				ntylog("Protocol Device Type is Error:%c\n", pClient->deviceType);
			}
		#endif
		}
		if ( pClient->group == NULL ) {
	#if 1 //Add Groups
	#endif
		}

		*result = NTY_RESULT_SUCCESS;
		return pClient;
	} else {
		ntylog("ntyAddClientHeap exist:%lld\n", client->devId);
	#if ENABLE_NATTY_TIME_STAMP //TIME Stamp 	
		ptrClient->stamp = ntyTimeStampGenrator();
	#endif
		if ( ptrClient->online==0 && ptrClient->deviceType == NTY_PROTO_CLIENT_WATCH ) {
			ntyExecuteChangeDeviceOnlineStatusHandle(ptrClient->devId, 1);
		}
		ptrClient->online = 1;

		return ptrClient;
	}

	return NULL;
}


int ntyDelClientHeap(C_DEVID clientId) {
	int ret = -1;

	void *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, clientId);
	if (record != NULL) {
		Client *pClient = record->value;
		if (pClient == NULL) {
			ntylog("ntyDelClientHeap pClient == NULL : %lld\n", clientId);
			return NTY_RESULT_NOEXIST;
		}

		pClient->rLength = 0;
		if (pClient->recvBuffer != NULL) {
			free(pClient->recvBuffer);
		}
#if 0 //cancel timer
		NWTimer* nwTimer = ntyTimerInstance();
		if (pClient->hbdTimer != NULL) {
			ntyTimerDel(nwTimer, pClient->hbdTimer);
			pClient->hbdTimer = NULL;
		}
#endif
		if (pClient->friends != NULL) {
			ntyVectorDestory(pClient->friends);
		}
#if 0 //release  groups
		pClient->group
#endif

		ret = ntyBHeapDelete(heap, clientId);
		if (ret == NTY_RESULT_FAILED) {
			ntylog("ntyDelClientHeap Delete Error\n");
		} else if (ret == NTY_RESULT_NOEXIST) {
			ntylog("ntyDelClientHeap Delete Error\n");
		}
		free(pClient);
	} else {
		return NTY_RESULT_NOEXIST;
	}

	return NTY_RESULT_SUCCESS;
}

int ntyDelClientRBTree(C_DEVID clientId) {
	int ret = -1;

	void *map = ntyRBTreeMapInstance();
	Client *pClient = (Client *)ntyRBTreeSearch( map, clientId );
	if ( pClient == NULL ) {
			ntylog("ntyDelClientRBTree pClient==NULL:%lld\n", clientId);
			return NTY_RESULT_NOEXIST;
	}
	pClient->rLength = 0;
	if ( pClient->recvBuffer != NULL ) {
		free(pClient->recvBuffer);
	}
	if ( pClient->friends != NULL ) {
		ntyVectorDestory(pClient->friends);
	}
	#if 0 //release  groups
		pClient->group
	#endif

	ret = ntyMapDelete( map, clientId );
	if ( ret == NTY_RESULT_FAILED) {
		ntylog("ntyDelClientRBTree Delete Error\n");
	} else if (ret == NTY_RESULT_NOEXIST) {
		ntylog("ntyDelClientRBTree Delete Error\n");
	}else{}
	
	free( pClient );
	return NTY_RESULT_SUCCESS;
}


/*
 * Release Client
 * 1. HashTable	Hash
 * 2. HashMap  	RBTree
 * 3. BHeap 	BPlusTree
 */

int ntyClientCleanup(ClientSocket *client) { //
	if (client == NULL) return NTY_RESULT_ERROR;
	if (client->watcher == NULL) return NTY_RESULT_ERROR;
	int sockfd = client->watcher->fd;

	void *hash = ntyHashTableInstance();
	Payload *load = ntyHashTableSearch(hash, sockfd);
	if (load == NULL) return NTY_RESULT_NOEXIST;

	C_DEVID Id = load->id;
	ntylog("ntyClientCleanup id:%lld\n", Id);
	if (load->id == NATTY_NULL_DEVID) return NTY_RESULT_NOEXIST;
	
	//delete hash key 
	int ret = ntyHashTableDelete(hash, sockfd);
	if (ret == NTY_RESULT_SUCCESS) {
		ntylog("ntyClientCleanup ntyHashTableDelete success ret:%d\n", ret);
		//release client socket map
		ret = ntyDelRelationMap(Id);			
		//release HashMap
	#if 0 //no delete from BHeap
		ret = ntyDelClientHeap(Id);
		ntylog(" ntyMapDelete --> ret : %d\n", ret);
	#endif
	
	#if ENABLE_RBTREE_REPLACE_BPTREE
		ntyOfflineClientRBTree( Id );	
	#else
		ntyOfflineClientHeap(Id);
	#endif
	} else {
		ntylog("ntyClientCleanup ntyHashTableDelete failed ret:%d\n", ret);
	}

	return ret;
}


int ntyOfflineClientHeap(C_DEVID clientId) {
	BPTreeHeap *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, clientId);

	if (record == NULL) {
		ntylog("Error OfflineClientHeap is not Exist %lld\n", clientId);
		ntyExecuteChangeDeviceOnlineStatusHandle( clientId, 0 );	//offline status	
		//ntyPrintTree(heap->root);  //easy error,just for debug
		return NTY_RESULT_NOEXIST;
	}

	Client* pClient = (Client*)record->value;
	if (pClient == NULL) {
		ntylog("Error OfflineClientHeap record->value == NULL %lld\n", clientId);
		//ntyPrintTree(heap->root);  //easy error,just for debug
		return NTY_RESULT_NOEXIST;
	}

	pClient->online = 0;
	if (pClient->deviceType == NTY_PROTO_CLIENT_WATCH) {
		ntyExecuteChangeDeviceOnlineStatusHandle(pClient->devId, 0);
	}
#if 0	
	if (pClient->deviceType == NTY_PROTO_CLIENT_IOS || pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
		if (pClient->token != NULL) {
			free(pClient->token);
			pClient->token = NULL;
		}
	}
#endif	
	return NTY_RESULT_SUCCESS;
}

int ntyOfflineClientRBTree(C_DEVID clientId) {
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (Client *)ntyRBTreeSearch( map, clientId );

	if ( pClient == NULL ) {
		ntylog("ntyOfflineClientRBTree pClient==NULL:%lld\n", clientId);
		ntyExecuteChangeDeviceOnlineStatusHandle( clientId, 0 );	//offline status
		return NTY_RESULT_NOEXIST;
	}

	pClient->online = 0;	//offline flag
	if ( pClient->deviceType == NTY_PROTO_CLIENT_WATCH ) {
		ntyExecuteChangeDeviceOnlineStatusHandle(pClient->devId, 0);
	}
	#if 0	
	if (pClient->deviceType == NTY_PROTO_CLIENT_IOS || pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
		if (pClient->token != NULL) {
			free(pClient->token);
			pClient->token = NULL;
		}
	}
	#endif	
	
	return NTY_RESULT_SUCCESS;
}


int ntyOnlineClientHeap(C_DEVID clientId) {
	BPTreeHeap *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, clientId);

	if (record == NULL) {
		ntylog(" Error ntyOnlineClientHeap is not Exist : %lld\n", clientId);
		ntyExecuteChangeDeviceOnlineStatusHandle( clientId, 1 );	//online status	
		//ntyPrintTree(heap->root);  //easy error,just for debug
		return NTY_RESULT_NOEXIST;
	}

	Client* pClient = (Client*)record->value;
	if (pClient == NULL) {
		ntylog(" Error ntyOnlineClientHeap record->value == NULL %lld\n", clientId);
		//ntyPrintTree(heap->root);  //easy error,just for debug
		return NTY_RESULT_NOEXIST;
	}
	
#if ENABLE_NATTY_TIME_STAMP //TIME Stamp 	
	pClient->stamp = ntyTimeStampGenrator();
#endif
	if (pClient->online == 0 && pClient->deviceType == NTY_PROTO_CLIENT_WATCH) {
		ntyExecuteChangeDeviceOnlineStatusHandle(pClient->devId, 1);
	}
	pClient->online = 1;

	return NTY_RESULT_SUCCESS;
}

int ntyOnlineClientRBTree(C_DEVID clientId) {
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (Client *)ntyRBTreeSearch( map, clientId );
	if ( pClient == NULL ) {
		ntylog("ntyOnlineClientRBTree pClient==NULL:%lld\n", clientId);
		ntyExecuteChangeDeviceOnlineStatusHandle( clientId, 1 );	//online status	
		return NTY_RESULT_NOEXIST;
	}
	
#if ENABLE_NATTY_TIME_STAMP //TIME Stamp 	
	pClient->stamp = ntyTimeStampGenrator();
#endif
	if ( pClient->online == 0 && pClient->deviceType == NTY_PROTO_CLIENT_WATCH ) {
		ntyExecuteChangeDeviceOnlineStatusHandle(pClient->devId, 1);
	}
	pClient->online = 1;

	return NTY_RESULT_SUCCESS;
}



int ntyLogoutOfflineClientHeap(C_DEVID clientId) {
	BPTreeHeap *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, clientId);

	if (record == NULL) {
		ntylog("Error ntyLogoutOfflineClientHeap is not Exist:%lld\n", clientId);
		ntyExecuteChangeDeviceOnlineStatusHandle( clientId, 0 );	//offline status
		//ntyPrintTree(heap->root); //easy error,just for debug
		return NTY_RESULT_NOEXIST;
	}

	Client* pClient = (Client*)record->value;
	if (pClient == NULL) {
		ntylog("Error ntyLogoutOfflineClientHeap record->value == NULL %lld\n", clientId);
		//ntyPrintTree(heap->root); //easy error,just for debug
		return NTY_RESULT_NOEXIST;
	}

	pClient->online = 0;
	if (pClient->deviceType == NTY_PROTO_CLIENT_WATCH) {
		ntyExecuteChangeDeviceOnlineStatusHandle(pClient->devId, 0);
	}

	if (pClient->deviceType == NTY_PROTO_CLIENT_IOS || pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
		if (pClient->token != NULL) {
			free(pClient->token);
			pClient->token = NULL;
		}
	}
	
	return NTY_RESULT_SUCCESS;
}

int ntyLogoutOfflineClientRBTree(C_DEVID clientId) {
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (Client *)ntyRBTreeSearch( map, clientId );
	if ( pClient == NULL ) {
		ntylog("ntyLogoutOfflineClientRBTree pClient==NULL:%lld\n", clientId);
		ntyExecuteChangeDeviceOnlineStatusHandle( clientId, 0 );	//offline status
		return NTY_RESULT_NOEXIST;
	}

	pClient->online = 0;
	if ( pClient->deviceType == NTY_PROTO_CLIENT_WATCH ) {
		ntyExecuteChangeDeviceOnlineStatusHandle(pClient->devId, 0);
	}

	if (pClient->deviceType == NTY_PROTO_CLIENT_IOS || pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
		if (pClient->token != NULL) {
			free(pClient->token);
			pClient->token = NULL;
		}
	}
	
	return NTY_RESULT_SUCCESS;
}


//int ntyUpdateClientInfo()

/*
 * Login Packet
 * Login Packet, HeartBeatPacket, LogoutPacket etc. should use templete designer pattern
 */
void ntyLoginPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_LOGIN_REQ) {		
		ntylog("====================begin ntyLoginPacketHandleRequest action ==========================\n");
	#if 0
		int i = 0;
		void *pRBTree = ntyRBTreeInstance();
		
		ntyAddClientNodeToRBTree(buffer, length, obj);
		Client *pClient = (Client*)ntyRBTreeInterfaceSearch(pRBTree, client->devId);
		if (pClient != NULL) {
			ntySendFriendsTreeIpAddr(pClient, 1);

			ntylog(" Buffer Version : %x\n", buffer[NEY_PROTO_VERSION_IDX]);
			if (buffer[NEY_PROTO_VERSION_IDX] == NTY_PROTO_DEVICE_VERSION)
				ntySendDeviceTimeCheckAck(pClient->devId, client->ackNum+1);
		}
	#else
		const MessagePacket *msg = (const MessagePacket*)obj;
		if ( msg == NULL ) return ;
		const Client *client = msg->client;
		MessagePacket *pMsg = (MessagePacket *)malloc(sizeof(MessagePacket));
		if ( pMsg == NULL ) return ;
		memset( pMsg, 0, sizeof(MessagePacket) );		
		memcpy( pMsg, msg, sizeof(MessagePacket) );
		ntyAddRelationMap( pMsg );
		ntyFree( pMsg );

		int ret = NTY_RESULT_SUCCESS;
		#if ENABLE_RBTREE_REPLACE_BPTREE
			Client *pClient = ntyAddClientRBTree(client, &ret);
		#else
			Client *pClient = ntyAddClientHeap( client, &ret );
		#endif
		
		if ( pClient != NULL ) {
			//ntySendFriendsTreeIpAddr(pClient, 1);
			pClient->deviceType = client->deviceType; //Client from Android switch IOS 
			if ( pClient->deviceType == NTY_PROTO_CLIENT_IOS && buffer[NTY_PROTO_VERSION_IDX] == NTY_PROTO_VERSION_B ) {
				pClient->deviceType = NTY_PROTO_CLIENT_IOS_APP_B;
			} else if ( pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH && buffer[NTY_PROTO_VERSION_IDX] == NTY_PROTO_VERSION_B ) {
				pClient->deviceType = NTY_PROTO_CLIENT_IOS_APP_B_PUBLISH;
			}else{}	
			if ( pClient->deviceType == NTY_PROTO_CLIENT_WATCH ) {
			#if 0
				ntySendDeviceTimeCheckAck(pClient, client->ackNum+1);
			#elif 0
				ntySendDeviceTimeCheckAck(pClient, 1);
			#else	
				#if 0
				ntyExecuteChangeDeviceOnlineStatusHandle(pClient->devId);
				#endif
				//ntySendLoginAckResult(pClient->devId, "", 0, 200);
				ntySendDeviceTimeCheckAck(pClient->devId, 1);
				#if 0 //move to OfflineMsge
				VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
				if (tag == NULL) return ;

				memset(tag, 0, sizeof(VALUE_TYPE));
				tag->fromId = pClient->devId;
				tag->Type = MSG_TYPE_OFFLINE_MSG_REQ_HANDLE;
				tag->cb = ntyDeviceOfflineMsgReqHandle;

				ntyDaveMqPushMessage(tag);
				#endif 
			#endif
			} else {

				if (pClient->deviceType == NTY_PROTO_CLIENT_IOS || pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH
					|| pClient->deviceType == NTY_PROTO_CLIENT_IOS_APP_B || pClient->deviceType == NTY_PROTO_CLIENT_IOS_APP_B_PUBLISH) {

					U16 tokenLen = *(U16*)(buffer+NTY_PROTO_LOGIN_REQ_JSON_LENGTH_IDX);	
					U8 *token = buffer+NTY_PROTO_LOGIN_REQ_JSON_CONTENT_IDX;
					if ( tokenLen == TOKEN_SIZE*2 ) {				
						if ( pClient->token == NULL ) {
							pClient->token = malloc(tokenLen + 1);
						}else{
							memset( pClient->token, 0, tokenLen + 1) ;
							memcpy( pClient->token, token, tokenLen );
							ntylog("ntyLoginPacketHandleRequest LOGIN:%s\n", pClient->token);
						}
					}
				}
			#if 0 //Update By WangBoJing 
				VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
				if (tag == NULL) return ;

				memset(tag, 0, sizeof(VALUE_TYPE));
				tag->fromId = pClient->devId;
				tag->Type = MSG_TYPE_OFFLINE_MSG_REQ_HANDLE;
				tag->cb = ntyOfflineMsgReqHandle;
				ntyDaveMqPushMessage(tag);
			#else 
				ntySendLoginAckResult(pClient->devId, "", 0, 200);
			#endif
			}
		} else {	
			ntylog("ntyLoginPacketHandleRequest BHeap in the Processs\n");
			if ( ret == NTY_RESULT_BUSY ) {
				ntyJsonCommonResult(client->devId, NATTY_RESULT_CODE_BUSY);
			}
		}
	#endif
		//ntylog("Login deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);		
		//send login ack	
		ntylog("====================end ntyLoginPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}


static const ProtocolFilter ntyLoginFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyLoginPacketHandleRequest,
};

/*
 * HeartBeatPacket
 */
void ntyHeartBeatPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_HEARTBEAT_REQ) {
		ntylog("====================begin ntyHeartBeatPacketHandleRequest action ==========================\n");
		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		MessagePacket *pMsg = (MessagePacket *)malloc(sizeof(MessagePacket));
		if (pMsg == NULL) {
			ntylog(" %s --> malloc failed MessagePacket. \n", __func__);
			return;
		}
		memset(pMsg, 0, sizeof(MessagePacket));
		memcpy(pMsg, msg, sizeof(MessagePacket));
		
		ntyAddRelationMap(pMsg);
		ntyFree(pMsg);
		
		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_HEARTBEAT_REQ_DEVID_IDX);
#if 1
		int ret = NTY_RESULT_SUCCESS;
	#if ENABLE_RBTREE_REPLACE_BPTREE
		Client *pClient = ntyAddClientRBTree(client, &ret);
	#else
		Client *pClient = ntyAddClientHeap(client, &ret);
	#endif
		if (pClient == NULL) {
			ntylog(" ntyHeartBeatPacketHandleRequest --> Error return\n");
			return;
		}
#else
		ntyOnlineClientHeap(client->devId);
#endif
		//if ( pClient->deviceType != NTY_PROTO_CLIENT_WATCH ){	//add by Rain 2017-10-31,do not reply all the clients.
		//	ntySendHeartBeatResult(fromId);	
		//}
		ntylog("====================end ntyHeartBeatPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}


static const ProtocolFilter ntyHeartBeatFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyHeartBeatPacketHandleRequest,
};


/*
 * Logout Packet
 */
void ntyLogoutPacketHandleRequest(const void *_self, U8 *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_LOGOUT_REQ) {
		ntylog("====================begin ntyLogoutPacketHandleRequest action ==========================\n");
		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
#if 0 //offline
		ntyOfflineClientHeap(client->devId);
#else
	#if ENABLE_RBTREE_REPLACE_BPTREE
		ntyLogoutOfflineClientRBTree(client->devId);
	#else
		ntyLogoutOfflineClientHeap(client->devId);
	#endif
	
#endif
		ntylog("====================end ntyLogoutPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyLogoutFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyLogoutPacketHandleRequest,
};


void ntyTimeCheckHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_TIME_CHECK_REQ) {
		ntylog("====================begin ntyTimeCheckHandleRequest action ==========================\n");
		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
#if 0
		C_DEVID key = *(C_DEVID*)(buffer+NTY_PROTO_LOGIN_REQ_DEVID_IDX);
		U32 ackNum = *(U32*)(buffer+NTY_PROTO_LOGIN_REQ_ACKNUM_IDX)+1;
		ntySendDeviceTimeCheckAck(client, ackNum);
#else

		ntySendDeviceTimeCheckAck(client->devId, 1);
#endif
		ntylog("====================end ntyTimeCheckHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}

}


static const ProtocolFilter ntyTimeCheckFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyTimeCheckHandleRequest,
};


void ntyICCIDReqPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_ICCID_REQ) {
		ntylog("====================begin ntyICCIDReqPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_ICCID_REQ_DEVID_IDX);

		U16 jsonlen = 0;
		memcpy(&jsonlen, buffer+NTY_PROTO_ICCID_REQ_JSON_LENGTH_IDX, NTY_JSON_COUNT_LENGTH);
		char *jsonstring = malloc(jsonlen+1);
		if (jsonstring == NULL) {
			ntylog(" %s --> malloc failed jsonstring. \n", __func__);
			return;
		}
		
		memset(jsonstring, 0, jsonlen+1);
		memcpy(jsonstring, buffer+NTY_PROTO_ICCID_REQ_JSON_CONTENT_IDX, jsonlen);

		JSON_Value *json = ntyMallocJsonValue(jsonstring);
		if (json == NULL) {
			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_ERR_JSON_FORMAT);
		} else {
			size_t len_ActionParam = sizeof(ActionParam);
			ActionParam *pActionParam = malloc(len_ActionParam);
			if (pActionParam == NULL) {
				ntylog(" %s --> malloc failed ActionParam. \n", __func__);
				return;
			}
			memset(pActionParam, 0, len_ActionParam);

			/*
			VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
			if (tag == NULL) {
				ntylog(" %s --> malloc failed VALUE_TYPE. \n", __func__);
				return;
			}
			memset(tag, 0, sizeof(VALUE_TYPE));
			tag->fromId = fromId;
			tag->toId = fromId;
			tag->Type = MSG_TYPE_OFFLINE_MSG_REQ_HANDLE;
			tag->cb = ntyDeviceOfflineMsgReqHandle;

			ntyDaveMqPushMessage(tag);
			*/	
			
			pActionParam->fromId = fromId;
			pActionParam->toId = fromId;
			pActionParam->json = json;
			pActionParam->jsonstring = jsonstring;
			pActionParam->jsonlen = jsonlen;
			pActionParam->index = 0;
			ntyJsonICCIDAction(pActionParam);
			
			free(pActionParam);
		}
		ntyFreeJsonValue(json);
		free(jsonstring);

		ntylog("====================end ntyICCIDReqPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}


static const ProtocolFilter ntyICCIDReqFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyICCIDReqPacketHandleRequest,
};

void ntyVoiceReqPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_VOICE_REQ) {
		ntylog("====================begin ntyVoiceReqPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
		
		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_VOICE_REQ_DEVID_IDX);
		U32 msgId = *(U32*)(buffer+NTY_PROTO_VOICE_REQ_MSGID_IDX);

		ntylog(" ntyVoiceReqPacketHandleRequest -->fromId:%lld,msgId:%d\n", fromId, msgId);
#if 0
		ntyVoiceReqAction(fromId, msgId);
#else
		
		VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
		if (tag == NULL) return ;

		memset(tag, 0, sizeof(VALUE_TYPE));
		tag->fromId = fromId;
		tag->arg = msgId;
		tag->Type = MSG_TYPE_VOICE_REQ_HANDLE;
		tag->cb = ntyVoiceReqHandle;

		ntyDaveMqPushMessage(tag);
#endif
		ntylog("====================end ntyVoiceReqPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyVoiceReqFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyVoiceReqPacketHandleRequest,
};


void ntyVoiceAckPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_VOICE_ACK) {
		ntylog("====================begin ntyVoiceAckPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_VOICE_ACK_DEVID_IDX);
		U32 msgId = *(U32*)(buffer+NTY_PROTO_VOICE_ACK_MSGID_IDX);

		ntylog(" ntyVoiceAckPacketHandleRequest -->fromId:%lld,msgId:%d\n", fromId, msgId);

		ntyVoiceAckAction(fromId, msgId);
		ntylog("====================end ntyVoiceAckPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyVoiceAckFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyVoiceAckPacketHandleRequest,
};


void ntyCommonReqPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_COMMON_REQ) {
		ntylog("====================begin ntyCommonReqPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_COMMON_REQ_DEVID_IDX);
		C_DEVID toId = *(C_DEVID*)(buffer+NTY_PROTO_COMMON_REQ_RECVID_IDX);

		U16 jsonlen = 0;
		memcpy(&jsonlen, buffer+NTY_PROTO_COMMON_REQ_JSON_LENGTH_IDX, NTY_JSON_COUNT_LENGTH);
		char *jsonstring = malloc(jsonlen+1);
		if (jsonstring == NULL) {
			ntylog(" %s --> malloc failed jsonstring\n", __func__);
			
			return ;
		}
		
		memset(jsonstring, 0, jsonlen+1);
		memcpy(jsonstring, buffer+NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX, jsonlen);

		ntylog("ntyCommonReqPacketHandleRequest --> fromId:%lld,toId:%lld,json:%s,len:%d\n", fromId, toId, jsonstring, jsonlen);


		VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
		if (tag == NULL) return ;

		memset(tag, 0, sizeof(VALUE_TYPE));
		tag->fromId = fromId;
		tag->toId = toId;
		tag->Tag = jsonstring;
		tag->length = jsonlen;
		tag->arg = 0;
		
		tag->Type = MSG_TYPE_COMMON_REQ_HANDLE;
		tag->cb = ntyCommonReqHandle;

		ntyDaveMqPushMessage(tag);

#if 0	//move to MQ	
		JSON_Value *json = ntyMallocJsonValue(jsonstring);
		if (json == NULL) { //JSON Error and send Code to FromId Device
			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_ERR_JSON_FORMAT);
		} else {

			size_t len_ActionParam = sizeof(ActionParam);
			ActionParam *pActionParam = malloc(len_ActionParam);
			if (pActionParam == NULL) {
				ntylog(" %s --> malloc failed ActionParam", __func__);
				free(jsonstring);

				return ;
			}
			memset(pActionParam, 0, len_ActionParam);
			 
			pActionParam->fromId = fromId;
			pActionParam->toId = toId;
			pActionParam->json = json;
			pActionParam->jsonstring = jsonstring;
			pActionParam->jsonlen = jsonlen;
			pActionParam->index = 0;
			
			ntyCommonReqAction(pActionParam);
			free(pActionParam);

		}
		free(jsonstring);
		ntyFreeJsonValue(json);
#endif		
		ntylog("====================end ntyCommonReqPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
	
}

static const ProtocolFilter ntyCommonReqFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyCommonReqPacketHandleRequest,
};

void ntyCommonAckPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_COMMON_ACK) {
		ntylog("====================begin ntyCommonAckPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_COMMON_ACK_DEVID_IDX);
		U32 msgId = *(U32*)(buffer+NTY_PROTO_COMMON_ACK_MSGID_IDX);
		ntylog("ntyCommonAckPacketHandleRequest fromId:%lld, msgId:%d\n", fromId, msgId);
#if 0

		int ret = ntyExecuteCommonOfflineMsgDeleteHandle(msgId, fromId);
		if (ret == NTY_RESULT_FAILED) {
			ntylog("ntyExecuteCommonOfflineMsgDeleteHandle DB Error \n");
		}
#else	
		VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
		if (tag == NULL) {
			ntylog(" %s --> malloc failed VALUE_TYPE. \n", __func__);
			return;
		}
		
		memset(tag, 0, sizeof(VALUE_TYPE));

		tag->fromId = fromId;
		tag->arg = msgId;
		if (client->deviceType == NTY_PROTO_CLIENT_WATCH) {
			tag->Type = MSG_TYPE_DEVICE_COMMON_ACK_HANDLE;
		} else {
			tag->Type = MSG_TYPE_APP_COMMON_ACK_HANDLE;
		}
		tag->cb = ntyCommonAckHandle;

		ntyDaveMqPushMessage(tag);
#endif
		ntylog("====================end ntyCommonAckPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyCommonAckFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyCommonAckPacketHandleRequest,
};


#if 1
U8 u8VoicePacket[NTY_VOICEREQ_COUNT_LENGTH*NTY_VOICEREQ_PACKET_LENGTH] = {0};
#endif

void ntyVoiceDataReqPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_VOICE_DATA_REQ) {
		ntylog("====================begin ntyVoiceDataReqPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		int i = 0;
		
		U16 index = *(U16*)(buffer+NTY_PROTO_VOICE_DATA_REQ_PKTINDEX_IDX);
		U16 Count = *(U16*)(&buffer[NTY_PROTO_VOICE_DATA_REQ_PKTTOTLE_IDX]);
		U32 pktLength = *(U32*)(buffer+NTY_PROTO_VOICE_DATA_REQ_PKTLENGTH_IDX);

		ntylog("ntyVoiceDataReqPacketHandleRequest Count:%d, index:%d, pktLength:%d, length:%d, pktLength%d\n", 
			Count, index, pktLength, length, NTY_PROTO_VOICE_DATA_REQ_PKTLENGTH_IDX);
	#if ENABLE_RBTREE_REPLACE_BPTREE
		void *map = ntyRBTreeMapInstance();
		Client *pClient = (Client *)ntyMapSearch( map, client->devId );
		if ( pClient== NULL ){
			ntylog( "ntyVoiceDataReqPacketHandleRequest not exit:%lld,return\n",client->devId );
			return;
		}
	#else
		void *heap = ntyBHeapInstance();
		NRecord *record = ntyBHeapSelect(heap, client->devId);
		if (record == NULL) {
			ntylog(" ntyBHeapSelect is not exist %lld\n", client->devId);
			return ;
		}
		Client *pClient = (Client*)record->value;
		if ( pClient== NULL ){
			ntylog( "ntyVoiceDataReqPacketHandleRequest not exit:%lld,return\n",client->devId );
			return;
		}
	#endif

		if (pClient->rBuffer == NULL) {
			pClient->rBuffer = malloc(NTY_VOICEREQ_COUNT_LENGTH*NTY_VOICEREQ_PACKET_LENGTH);
			
		}
		if (index == 0) { //start voice packet
			memset(pClient->rBuffer, 0, NTY_VOICEREQ_COUNT_LENGTH*NTY_VOICEREQ_PACKET_LENGTH);
		}

		for (i = 0;i < pktLength;i ++) {
			pClient->rBuffer[index * NTY_VOICEREQ_DATA_LENGTH + i] = buffer[NTY_VOICEREQ_HEADER_LENGTH + i];
		}
		if (index == Count-1) { //save voice data
			long stamp = 0;
			C_DEVID gId = 0, fromId = 0;
			U8 filename[NTY_VOICE_FILENAME_LENGTH] = {0};
			
			ntyU8ArrayToU64(buffer+NTY_PROTO_VOICE_DATA_REQ_DEVID_IDX, &fromId);
			ntyU8ArrayToU64(buffer+NTY_PROTO_VOICE_DATA_REQ_GROUP_IDX, &gId);
			
#if ENABLE_NATTY_TIME_STAMP //TIME Stamp 	
			stamp = ntyTimeStampGenrator();
#endif
			sprintf(filename, NTY_VOICE_FILENAME_FORMAT, gId, fromId, stamp);

			U32 dataLength = NTY_VOICEREQ_DATA_LENGTH*(Count-1) + pktLength;
			ntylog("ntyVoiceDataReqPacketHandleRequest Voice FileName:%s,len:%d,gId:%lld,fromId:%lld\n", filename, dataLength, gId, fromId );
			ntyWriteDat(filename, pClient->rBuffer, dataLength);

			//release rBuffer
			ntyFree(pClient->rBuffer);
			pClient->rBuffer = NULL;

/* enter to SrvAction
 * VoiceBroadCast to gId
 * 0. send data result to from Id 
 * 1. save to DB
 * 2. prepare to offline voice data
 * 
 */			
 			ntyVoiceDataReqAction(fromId, gId, filename);

		}

		ntylog("====================end ntyVoiceDataReqPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyVoiceDataReqFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyVoiceDataReqPacketHandleRequest,
};

void ntyVoiceDataAckPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_VOICE_ACK) {
		ntylog("====================begin ntyVoiceDataAckPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

#if 0
		C_DEVID toId = 0, fromId = 0;
		ntyU8ArrayToU64(buffer+NTY_PROTO_VOICEACK_SELFID_IDX, &fromId);
		ntyU8ArrayToU64(buffer+NTY_PROTO_VOICEACK_DESTID_IDX, &toId);

		ntylog(" ntyVoiceAckPacketHandleRequest --> toId:%lld, fromId:%lld\n", toId, fromId);

#if 0
		void *pRBTree = ntyRBTreeInstance();
		Client *toClient = (Client*)ntyRBTreeInterfaceSearch(pRBTree, toId);
#else
		void *map = ntyMapInstance();
		ClientSocket *toClient = ntyMapSearch(map, toId);
#endif
		if (toClient == NULL) { //no Exist
			return ;
		} 

		ntySendBuffer(toClient, buffer, length);
#else
		ntylog(" ntyVoiceDataAckPacketHandleRequest is dispatch\n");
#endif
		ntylog("====================end ntyVoiceDataAckPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyVoiceDataAckFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyVoiceDataAckPacketHandleRequest,
};

void ntyOfflineMsgReqPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_OFFLINE_MSG_REQ) {
		ntylog("====================begin ntyOfflineMsgReqPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_OFFLINE_MSG_REQ_DEVICEID_IDX);
		ntylog("ntyOfflineMsgReqPacketHandleRequest --> fromId:%lld\n", fromId);

		VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
		if (tag == NULL) return ;

		memset(tag, 0, sizeof(VALUE_TYPE));
		tag->fromId = fromId;
		tag->Type = MSG_TYPE_OFFLINE_MSG_REQ_HANDLE;
		tag->cb = ntyOfflineMsgReqHandle;
		ntyDaveMqPushMessage(tag);
		
		ntylog("====================end ntyOfflineMsgReqPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyOfflineMsgReqFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyOfflineMsgReqPacketHandleRequest,
};

void ntyOfflineMsgAckPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_OFFLINE_MSG_ACK) {
		ntylog("====================begin ntyOfflineMsgAckPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
		
		ntylog("====================end ntyOfflineMsgAckPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyOfflineMsgAckFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyOfflineMsgAckPacketHandleRequest,
};


void ntyUnBindDevicePacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_VERSION_IDX] == NTY_PROTO_VERSION_A && buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_UNBIND_REQ) {
		ntylog("====================begin ntyUnBindDevicePacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
		
		C_DEVID AppId = *(C_DEVID*)(buffer+NTY_PROTO_UNBIND_APPID_IDX);
		C_DEVID DeviceId = *(C_DEVID*)(buffer+NTY_PROTO_UNBIND_DEVICEID_IDX);

		
		int contactsTempId = 0;
#if ENABLE_CONNECTION_POOL
	#if 0
		int ret = ntyExecuteDevAppRelationDeleteHandle(AppId, DeviceId);
	#else
		int ret = ntyExecuteDevAppGroupDeleteHandle(AppId, DeviceId, &contactsTempId);
	#endif
		if (ret == -1) {
			ntylog(" ntyUnBindDevicePacketHandleRequest --> DB Exception\n");
		} else if (ret == 0) {
	#if 0
			void *pRBTree = ntyRBTreeInstance();
			Client *aclient = (Client*)ntyRBTreeInterfaceSearch(pRBTree, AppId);
			if (aclient != NULL) {
				if (aclient->friends != NULL) {
					ntyFriendsTreeDelete(aclient->friends, DeviceId);
				}
			}

			Client *dclient = (Client*)ntyRBTreeInterfaceSearch(pRBTree, DeviceId);
			if (dclient != NULL) {
				if (dclient->friends != NULL) {
					ntyFriendsTreeDelete(dclient->friends, AppId);
				}
			}
	#else
		int retTemp = 0;
	
		#if ENABLE_RBTREE_REPLACE_BPTREE
			void *map = ntyRBTreeMapInstance();
			Client *aclient = (Client *)ntyMapSearch( map, AppId );
			if ( aclient != NULL ){
				ntylog("ntyUnBindDevicePacketHandleRequest rbtree exit:%lld\n",AppId);
				retTemp = ntyVectorDelete(aclient->friends, &DeviceId);
				ntylog("ntyVectorDelete AppId:%lld ret:%d\n", AppId, retTemp);
			}else{
				ntylog("ntyUnBindDevicePacketHandleRequest rbtree not exit:%lld\n",AppId);
			}
			
			Client *dclient = (Client *)ntyMapSearch( map, DeviceId );
			if ( dclient != NULL ){
				ntylog("ntyUnBindDevicePacketHandleRequest rbtree exit:%lld\n",DeviceId);	
				retTemp = ntyVectorDelete(dclient->friends, &AppId);
				ntylog("ntyVectorDelete DeviceId:%lld ret:%d\n", DeviceId, retTemp);
			}else{
				ntylog("ntyUnBindDevicePacketHandleRequest rbtree not exit:%lld\n",DeviceId);
			}
		#else
			void *heap = ntyBHeapInstance();
			NRecord *record = ntyBHeapSelect(heap, AppId);
			if (record != NULL) {
				Client *aclient = (Client *)record->value;
				if ( aclient != NULL ){
					ntylog("ntyUnBindDevicePacketHandleRequest rbtree exit:%lld\n",AppId);
					retTemp = ntyVectorDelete(aclient->friends, &DeviceId);
					ntylog("ntyVectorDelete AppId:%lld ret:%d\n", AppId, retTemp);
				}else{
					ntylog("ntyUnBindDevicePacketHandleRequest rbtree not exit:%lld\n",AppId);
				}

			}

			record = ntyBHeapSelect(heap, DeviceId);
			if (record != NULL) {
				if ( dclient != NULL ){
					ntylog("ntyUnBindDevicePacketHandleRequest rbtree exit:%lld\n",DeviceId);	
					retTemp = ntyVectorDelete(dclient->friends, &AppId);
					ntylog("ntyVectorDelete DeviceId:%lld ret:%d\n", DeviceId, retTemp);
				}else{
					ntylog("ntyUnBindDevicePacketHandleRequest rbtree not exit:%lld\n",DeviceId);
				}

			}

		#endif
		
			DeviceDelContactsAck *pDeviceDelContactsAck = malloc(sizeof(DeviceDelContactsAck));
			if (pDeviceDelContactsAck == NULL) { 
				return;
			}
			memset(pDeviceDelContactsAck, 0, sizeof(DeviceDelContactsAck));

			char bufIMEI[64] = {0};
			sprintf(bufIMEI, "%llx", DeviceId);
			char contactsId[16] = {0};
			sprintf(contactsId, "%d", contactsTempId);
			char del[16] = {0};
			char category[16] = {0};
			strcat(del, "Delete");
			strcat(category, "Contacts");
			pDeviceDelContactsAck->msg = contactsId;
			pDeviceDelContactsAck->IMEI = bufIMEI;
			pDeviceDelContactsAck->category = category;
			pDeviceDelContactsAck->action = del;
			pDeviceDelContactsAck->id = contactsId;

			char *jsonresult = ntyJsonWriteDeviceDelContacts(pDeviceDelContactsAck);
			ntylog(" ntyUnBindDevicePacketHandleRequest json:%s\n", jsonresult);
			if ( jsonresult != NULL ){
				retTemp = ntySendRecodeJsonPacket(AppId, DeviceId, jsonresult, (int)strlen(jsonresult));
			}
			if (retTemp < 0) {
				ntylog(" ntyUnBindDevicePacketHandleRequest --> SendCommonReq Exception\n");
			}

			ntyJsonFree(jsonresult);
			ntyFree(pDeviceDelContactsAck);
	#endif		
			ret = NTY_RESULT_SUCCESS;
		}else{}
		ntyProtoUnBindAck(AppId, DeviceId, ret);
#endif		
		
		ntylog("====================end ntyUnBindDevicePacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}

}


static const ProtocolFilter ntyUnBindDeviceFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyUnBindDevicePacketHandleRequest,
};

void ntyBindConfirmReqPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_BIND_CONFIRM_REQ) {
		ntylog("====================begin ntyBindConfirmReqPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
		
		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_BIND_CONFIRM_REQ_ADMIN_SELFID_IDX);
		C_DEVID AppId = *(C_DEVID*)(buffer+NTY_PROTO_BIND_CONFIRM_REQ_PROPOSER_IDX);
		C_DEVID DeviceId = *(C_DEVID*)(buffer+NTY_PROTO_BIND_CONFIRM_REQ_DEVICEID_IDX);

		U32 msgId = *(U32*)(buffer+NTY_PROTO_BIND_CONFIRM_REQ_MSGID_IDX);
		U8 *jsonstring = buffer+NTY_PROTO_BIND_CONFIRM_REQ_JSON_CONTENT_IDX;
		//U8 *jsonstring = buffer+NTY_PROTO_BIND_CONFIRM_REQ_JSON_CONTENT_IDX-2;
		U16 jsonLen = *(U16*)(buffer+NTY_PROTO_BIND_CONFIRM_REQ_JSON_LENGTH_IDX);

		ntylog("ntyBindConfirmReqPacketHandleRequest --> json:%s,len:%d\n", jsonstring, jsonLen);

		JSON_Value *json = ntyMallocJsonValue( jsonstring );
		if (json == NULL) { //JSON Error and send Code to FromId Device
			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_ERR_JSON_FORMAT);
		} else {
			BindConfirmReq *pBindConfirmReq = (BindConfirmReq*)malloc(sizeof(BindConfirmReq));
			if (pBindConfirmReq == NULL) {
				ntylog(" %s --> malloc failed BindConfirmReq. \n", __func__);
				return ;
			}
			memset( pBindConfirmReq, 0, sizeof(BindConfirmReq) );
			ntyJsonBindConfirmReq( json, pBindConfirmReq );

			if ( strcmp(pBindConfirmReq->category, NATTY_USER_PROTOCOL_BINDCONFIRMREQ) == 0 ) {
				VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
				if (tag == NULL) {
					ntyFree( pBindConfirmReq );
					return ;
				}
				memset( tag, 0, sizeof(VALUE_TYPE) );
				
				tag->fromId = fromId;
				tag->toId = AppId;
				tag->gId = DeviceId;			
				tag->arg = msgId;
				tag->Type = MSG_TYPE_BIND_CONFIRM_REQ_HANDLE;
				tag->cb = ntyBindConfirmReqHandle;
				
				if (strcmp(pBindConfirmReq->answer, NATTY_USER_PROTOCOL_AGREE) == 0) {
					tag->u8LocationType = 1;
				} else if (strcmp(pBindConfirmReq->answer, NATTY_USER_PROTOCOL_REJECT) == 0) {
					tag->u8LocationType = 0;
				} else {
					ntylog("ntyBindConfirmReqPacketHandleRequest Can't find answer with: %s\n", pBindConfirmReq->answer);
				}
				
				ntylog("ntyBindConfirmReqPacketHandleRequest tag->answer:%s\n", pBindConfirmReq->answer);
				ntylog("ntyBindConfirmReqPacketHandleRequest tag->u8LocationType:%d\n", tag->u8LocationType);
				
				ntyDaveMqPushMessage( tag );
				ntyFree( pBindConfirmReq );
			
			}

			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_SUCCESS);
		}

		ntyFreeJsonValue(json);
		
#if ENABLE_CONNECTION_POOL

#if 1 //Need to Recode

#if 0 //juge agree and reject
#define NTY_TOKEN_AGREE			"Agree"
#define NTY_TOKEN_AGREE_LENGTH	5

#define NTY_TOKEN_REJECT		"Reject"
#define NTY_TOKEN_REJECT_LENGTH	6
		U32 match[6] = {0};

		VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));

		if (ntyKMP(jsonstring, jsonLen, NTY_TOKEN_AGREE, NTY_TOKEN_AGREE_LENGTH, match)) {
			tag->arg = 1;
		} else if (ntyKMP(jsonstring, jsonLen, NTY_TOKEN_REJECT, NTY_TOKEN_REJECT_LENGTH, match)) {
			tag->arg = 0;
		} else {
			tag->arg = 1;
		}
#endif
		
#endif
#endif

		ntylog("====================end ntyBindConfirmReqPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyBindConfirmReqPacketFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyBindConfirmReqPacketHandleRequest,
};


void ntyBindDevicePacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_VERSION_IDX] == NTY_PROTO_VERSION_A && buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_BIND_REQ) {
		ntylog("====================begin ntyBindDevicePacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
		
		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_BIND_APPID_IDX);
		C_DEVID toId =  *(C_DEVID*)(buffer+NTY_PROTO_BIND_DEVICEID_IDX);
		
#if 1	//New Version need implement
		U16 jsonlen = *(U16*)(buffer+NTY_PROTO_BIND_JSON_LENGTH_IDX);
		//memcpy(&jsonlen, buffer+NTY_PROTO_BIND_JSON_LENGTH_IDX, NTY_JSON_COUNT_LENGTH);
		char *jsonstring = malloc(jsonlen+1);
		if (jsonstring == NULL) {
			ntylog(" %s --> malloc failed jsonstring. \n", __func__);
			return;
		}
		memset(jsonstring, 0, jsonlen+1);
		memcpy(jsonstring, buffer+NTY_PROTO_BIND_JSON_CONTENT_IDX, jsonlen);

		ntylog("ntyBindDevicePacketHandleRequest --> json : %s  %d\n", jsonstring, jsonlen);
#if 0		
		JSON_Value *json = ntyMallocJsonValue(jsonstring);
		if (json == NULL) { //JSON Error and send Code to FromId Device
			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_ERR_JSON_FORMAT);
		} else {

			ActionParam *pActionParam = malloc(sizeof(ActionParam));
			if (pActionParam == NULL) {
				ntylog(" %s --> malloc failed ActionParam. \n", __func__);
				free(jsonstring);
				return;
			}
			pActionParam->fromId = fromId;
			pActionParam->toId = toId;
			pActionParam->json = json;
			pActionParam->jsonstring= jsonstring;
			pActionParam->jsonlen = jsonlen;
			pActionParam->index = 0;
			
			ntyBindReqAction(pActionParam);
			
			free(pActionParam);

		}
		free(jsonstring);
		ntyFreeJsonValue(json);
#else
		JSON_Value *json = ntyMallocJsonValue(jsonstring);
		if (json == NULL) { //JSON Error and send Code to FromId Device
			return ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_ERR_JSON_FORMAT);
		}

		VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
		if (tag == NULL) {
			ntylog(" %s --> malloc VALUE_TYPE error. \n", __func__);
			free(jsonstring);
			return;
		}
		
		tag->fromId = fromId;
		tag->gId = toId;
		
		tag->Tag = (U8*)json;		
		tag->length = jsonlen;
		
		tag->cb = ntyBindDeviceCheckStatusReqHandle;
		tag->Type = MSG_TYPE_BIND_CONFIRM_REQ_HANDLE;
		ntyDaveMqPushMessage(tag);
		
		free(jsonstring);
#endif



#else
		ntyProtoBindAck(fromId, toId, 5);
#endif		
		ntylog("====================end ntyBindDevicePacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}

}


static const ProtocolFilter ntyBindDeviceFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyBindDevicePacketHandleRequest,
};

void ntyBindDevicePacketHandleVersionBRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_VERSION_IDX] == NTY_PROTO_VERSION_B && buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_BIND_REQ) {
		ntylog("====================begin ntyBindDevicePacketHandleRequest_VersionB action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
		
		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_BIND_APPID_IDX);
		C_DEVID toId =  *(C_DEVID*)(buffer+NTY_PROTO_BIND_DEVICEID_IDX);
		
		U16 jsonLength = *(U16*)( buffer+NTY_PROTO_BIND_JSON_LENGTH_IDX );
		
		U8 *jsonstring = (U8*)malloc( jsonLength+1 );
		if ( jsonstring == NULL ) { 
			ntylog( "ntyBindDevicePacketHandleVersionBRequest --> malloc json buffer failed\n" );
			return ;
		}
		memset( jsonstring, 0, jsonLength+1 );
		memcpy( jsonstring, buffer+NTY_PROTO_BIND_JSON_CONTENT_IDX, jsonLength );
	
		ntylog( "ntyBindDevicePacketHandleVersionBRequest --> fromId:%lld,toId:%lld,json:%s,jsonLength:%d\n", fromId,toId,jsonstring,jsonLength );

		JSON_Value *json = ntyMallocJsonValue( jsonstring );
		if ( json == NULL ) { //JSON Error and send Code to FromId Device
			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_ERR_JSON_FORMAT);
			free(jsonstring);
			return;
		}
		ntyJsonCommonResult( fromId, NATTY_RESULT_CODE_SUCCESS );  //receive data and then send to fromId 200
		
		ClientActionParam *clientActionParamVal = (ClientActionParam*)malloc( sizeof(ClientActionParam) );
		if ( clientActionParamVal == NULL ){
			ntylog(" %s --> malloc failed clientActionParamVal", __func__);
			ntyFreeJsonValue(json);
			free(jsonstring);
			return;
		}
		memset( clientActionParamVal, 0, sizeof(ClientActionParam) );		
		clientActionParamVal->fromId = fromId;
		clientActionParamVal->toId = toId;
		clientActionParamVal->jsonString = jsonstring;
		clientActionParamVal->jsonObj = json;
		int nRet = 0;

		LocatorBindReq *ptrLocatorBindReq = (LocatorBindReq*)malloc( sizeof(LocatorBindReq) );
		if ( ptrLocatorBindReq == NULL ) {
			ntylog("ntyBindDevicePacketHandleVersionBRequest --> malloc failed ptrLocatorBindReq\n");
			return ;
		}
		memset( ptrLocatorBindReq, 0, sizeof(LocatorBindReq) );
		//parse json object from sender to save to struct object ptrLocatorBindReq
		nRet = ntyLocatorBindReqJsonParse( clientActionParamVal->jsonObj, ptrLocatorBindReq ); 
		if ( nRet == -1 ){
			goto EXIT;
		}

		if( (nRet=strcmp(ptrLocatorBindReq->Category,NATTY_USER_PROTOCOL_BINGREQ)) == 0 ){
			nRet = ntyLocatorBindReqAction( clientActionParamVal, ptrLocatorBindReq );
			ntylog( "*********nRet=ntyLocatorBindReqAction():nRet=%d\n",nRet );
			if ( nRet == -1 ){	//error
				//nRet = NTY_BIND_ACK_ERROR;
				nRet = NTY_BIND_ACK_REJECT;
			}
			ntyProtoBindAck( fromId, toId, nRet );
		}		
		
		EXIT:
			ntyFreeJsonValue(json);
			free(jsonstring);		
		ntylog("====================end ntyBindDevicePacketHandleRequest_VersionB action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}

}

static const ProtocolFilter ntyBindDeviceVersionBFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyBindDevicePacketHandleVersionBRequest,
};


void ntyUnBindDevicePacketHandleVersionBRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_VERSION_IDX] == NTY_PROTO_VERSION_B && buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_UNBIND_REQ) {
		ntylog("====================begin ntyUnBindDevicePacketHandleRequest_VersionB action ==========================\n");
	
		C_DEVID fromId = *(C_DEVID*)( buffer+NTY_PROTO_UNBIND_APPID_IDX );
		C_DEVID toId =  *(C_DEVID*)( buffer+NTY_PROTO_UNBIND_DEVICEID_IDX );
		
		ntylog( "ntyUnBindDevicePacketHandleVersionBRequest --> fromId:%lld,toId:%lld\n", fromId,toId );
		ntyJsonCommonResult( fromId, NATTY_RESULT_CODE_SUCCESS );  //receive data and then send to fromId 200
		int nRet = ntyLocatorUnBindReqAction( fromId, toId );
		if ( nRet < 0 || nRet > 0 ){
			nRet = NTY_RESULT_FAILED;
		}
		ntyProtoUnBindAck( fromId, toId, nRet );
		ntylog("====================end ntyUnBindDevicePacketHandleRequest_VersionB action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}

}


static const ProtocolFilter ntyUnBindDeviceVersionBFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyUnBindDevicePacketHandleVersionBRequest,
};



void ntyMulticastReqPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_MULTICAST_REQ) {
		ntylog("====================begin ntyMulticastReqPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
		
		C_DEVID toId = 0;
		ntyU8ArrayToU64(buffer+NTY_PROTO_DEST_DEVID_IDX, &toId);

		void *pRBTree = ntyRBTreeInstance();
		Client *toClient = (Client*)ntyRBTreeInterfaceSearch(pRBTree, toId);
		if (toClient == NULL) { //no Exist
			return;
		}

		buffer[NTY_PROTO_MULTICAST_TYPE_IDX] = NTY_PROTO_DATAPACKET_REQ;
		//ntySendBuffer(toClient, buffer, length);
		//ntyMulticastSend();

		ntylog("====================end ntyMulticastReqPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}


static const ProtocolFilter ntyMutlcastReqFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyMulticastReqPacketHandleRequest,
};


void ntyMulticastAckPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_MULTICAST_ACK) {
		ntylog("====================begin ntyMulticastAckPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;
		
		C_DEVID fromId = 0;
			
		ntyU8ArrayToU64(buffer+NTY_PROTO_DEST_DEVID_IDX, &fromId);

		ntylog("====================end ntyMulticastAckPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}


static const ProtocolFilter ntyMutlcastAckFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyMulticastAckPacketHandleRequest,
};

void ntyLocationAsyncReqPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_LOCATION_ASYNCREQ) {
		ntylog("====================begin ntyLocationAsyncReqPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		U16 jsonlen = 0;
		memcpy(&jsonlen, buffer+NTY_PROTO_LOCATION_ASYNC_REQ_JSON_LENGTH_IDX, NTY_JSON_COUNT_LENGTH);
		char *jsonstring = malloc(jsonlen+1);
		if (jsonstring == NULL) {
			ntylog(" %s --> malloc failed jsonstring. \n", __func__);
			return;
		}
		memset(jsonstring, 0, jsonlen+1);
		memcpy(jsonstring, buffer+NTY_PROTO_LOCATION_ASYNC_REQ_JSON_CONTENT_IDX, jsonlen);

		ntylog(" ntyLocationAsyncReqPacketHandleRequest --> json: %s, %d\n", jsonstring, jsonlen);
		C_DEVID deviceId = *(C_DEVID*)(buffer+NTY_PROTO_LOCATION_ASYNC_REQ_DEVID_IDX);

		JSON_Value *json = ntyMallocJsonValue(jsonstring);
		if (json == NULL) {
			ntyJsonCommonResult(deviceId, NATTY_RESULT_CODE_ERR_JSON_FORMAT);
		} else {
			//size_t len_ActionParam = sizeof(ActionParam);
			ActionParam *pActionParam = malloc(sizeof(ActionParam));
			if (pActionParam == NULL) {
				ntylog(" %s --> malloc failed ActionParam. \n", __func__);
				goto exit;
			}
			memset(pActionParam, 0, sizeof(ActionParam));

			pActionParam->fromId = client->devId;
			pActionParam->toId = deviceId;
			pActionParam->json = json;
			pActionParam->jsonstring= jsonstring;
			pActionParam->jsonlen = jsonlen;
			pActionParam->index = 0;
			const char *category = ntyJsonAppCategory(json);
			ntylog("ntyLocationAsyncReqPacketHandleRequest --> category:%s\n", category);
			if (strcmp(category, NATTY_USER_PROTOCOL_WIFI) == 0) {
				ntyJsonLocationWIFIAction(pActionParam);
			} else if (strcmp(category, NATTY_USER_PROTOCOL_LAB) == 0) {
				ntyJsonLocationLabAction(pActionParam);
			} else {
				ntylog("Can't find category with: %s\n", category);
			}
			free(pActionParam);
		}
		
		ntyFreeJsonValue(json);

exit:
		free(jsonstring);
		ntylog("====================end ntyLocationAsyncReqPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyLocationAsyncReqFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyLocationAsyncReqPacketHandleRequest,
};


void ntyWeatherAsyncReqPacketHandleRequest(const void *_self, unsigned char *buffer, int length, const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_WEATHER_ASYNCREQ) {
		ntylog("====================begin ntyWeatherAsyncReqPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		U16 jsonlen = 0;
		memcpy(&jsonlen, buffer+NTY_PROTO_WEATHER_ASYNC_REQ_JSON_LENGTH_IDX, NTY_JSON_COUNT_LENGTH);
		char *jsonstring = malloc(jsonlen+1);
		if (jsonstring == NULL) {
			ntylog(" %s --> malloc failed jsonstring. \n", __func__);
			return;
		}

		memset(jsonstring, 0, jsonlen+1);
		memcpy(jsonstring, buffer+NTY_PROTO_WEATHER_ASYNC_REQ_JSON_CONTENT_IDX, jsonlen);
		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_WEATHER_ASYNC_REQ_DEVID_IDX);

		ntylog("ntyWeatherAsyncReqPacketHandleRequest --> json : %s  %d\n", jsonstring, jsonlen);
		
		JSON_Value *json = ntyMallocJsonValue(jsonstring);
		if (json == NULL) {
			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_ERR_JSON_FORMAT);
		} else {
			size_t len_ActionParam = sizeof(ActionParam);
			ActionParam *pActionParam = malloc(len_ActionParam);
			if (pActionParam == NULL) {
				ntylog(" %s --> malloc failed ActionParam. \n", __func__);
				goto exit;
			}
			memset(pActionParam, 0, len_ActionParam);
			
			pActionParam->fromId = fromId;
			pActionParam->toId = client->devId;
			pActionParam->json = json;
			pActionParam->jsonstring= jsonstring;
			pActionParam->jsonlen = jsonlen;
			pActionParam->index = 0;
			ntyJsonWeatherAction(pActionParam);
			free(pActionParam);
		}
		ntyFreeJsonValue(json);

exit:
		free(jsonstring);
		ntylog("====================end ntyWeatherAsyncReqPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}

static const ProtocolFilter ntyWeatherAsyncReqFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyWeatherAsyncReqPacketHandleRequest,
};


/*
 * Server Proxy Data Transport
 * VERSION					1			 BYTE
 * MESSAGE TYPE			 	1			 BYTE (req, ack)
 * TYPE				 		1			 BYTE 
 * DEVID					8			 BYTE (self devid)
 * ACKNUM					4			 BYTE (Network Module Set Value)
 * DEST DEVID				8			 BYTE (friend devid)
 * CONTENT COUNT			2			 BYTE 
 * CONTENT					*(CONTENT COUNT)	 BYTE 
 * CRC 				 		4			 BYTE (Network Module Set Value)
 * 
 * send to server addr, proxy to send one client
 * 
 */
void ntyRoutePacketHandleRequest(const void *_self, unsigned char *buffer, int length,const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_DATA_ROUTE) {
		ntylog("====================begin ntyRoutePacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_DATA_ROUTE_DEVID_IDX);
		C_DEVID toId = *(C_DEVID*)(buffer+NTY_PROTO_DATA_ROUTE_RECVID_IDX);

		U16 jsonLen = *(U16*)(buffer+NTY_PROTO_DATA_ROUTE_JSON_LENGTH_IDX);
		ntylog("ntyRoutePacketHandleRequest --> fromId:%lld, toId:%lld, length:%d, jsonLen:%d\n", fromId, toId, length, jsonLen);
		
		int len = ntySendDataRoute(toId, (U8*)buffer, length);
		if (len >= 0) {
			ntylog("ntySendDataRoute success \n");
			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_SUCCESS);
		} else {
			ntylog("ntySendDataRoute no exist \n");
			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_ERR_DEVICE_NOTONLINE);
		}
		ntylog("====================end ntyRoutePacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}


static const ProtocolFilter ntyRoutePacketFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyRoutePacketHandleRequest,
};


void ntyUserDataPacketReqHandleRequest(const void *_self, unsigned char *buffer, int length,const void* obj) {
	const UdpClient *client = obj;
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_DATAPACKET_REQ) {
		ntylog("====================begin ntyUserDataPacketReqHandleRequest action ==========================\n");
#if 1
		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		const Client *client = msg->client;

		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_USERDATA_PACKET_REQ_DEVICEID_IDX);
		U16 jsonLength = *(U16*)(buffer+NTY_PROTO_USERDATA_PACKET_REQ_JSON_LENGTH_IDX);
		U8 *jsonstring = (U8*)malloc(jsonLength+1);
		if (jsonstring == NULL) { 
			ntylog("ntyUserDataPacketReqHandleRequest --> malloc failed json buffer\n");
			return ;
		}
		memset(jsonstring, 0, jsonLength+1);
		memcpy(jsonstring, buffer+NTY_PROTO_USERDATA_PACKET_REQ_JSON_CONTENT_IDX, jsonLength);
	
		ntylog("ntyUserDataPacketReqHandleRequest --> fromId:%lld,json:%s,len:%d\n", fromId, jsonstring, jsonLength);

		JSON_Value *json = ntyMallocJsonValue(jsonstring);
		if (json == NULL) { //JSON Error and send Code to FromId Device
			ntyJsonCommonResult(fromId, NATTY_RESULT_CODE_ERR_JSON_FORMAT);
			free(jsonstring);
			return;
		}

		const char *app_action = ntyJsonAction(json);
		if (app_action == NULL) {
			ntylog("Can't find action, because action is null\n");
			free(jsonstring);
			return;
		}
		if (strcmp(app_action, NATTY_USER_PROTOCOL_ACTION_SELECT) == 0) {
			//add by luoyb. parse json and search from DB,and send back. add begin 
			//JSON_Value *jsonObj = ntyMallocJsonValue( jsonstring ); //json to json Object
			ClientActionParam *clientActionParamVal = (ClientActionParam*)malloc(sizeof(ClientActionParam));
			if ( clientActionParamVal == NULL ){
				ntylog(" %s --> malloc failed clientActionParamVal", __func__);
				ntyFreeJsonValue(json);
				free(jsonstring);
				return;
			}
			memset( clientActionParamVal, 0, sizeof(ClientActionParam) );		
			clientActionParamVal->fromId = fromId;	//NTY_PROTO_DATAPACKET_REQ this protocol is the device select user data. 
			clientActionParamVal->toId = fromId;
			clientActionParamVal->jsonString = jsonstring;
			clientActionParamVal->jsonObj = json;

			ClientSelectReq *ptrclientSelectReq = (ClientSelectReq*)malloc(sizeof(ClientSelectReq));
			if ( ptrclientSelectReq == NULL ) {
				ntylog("ntyClientSeleteScheduleReqAction --> malloc failed ClientSelectReq\n");
				return ;
			}
			memset( ptrclientSelectReq, 0, sizeof(ClientSelectReq) );
			//parse json object from sender to save to struct object ptrclientSelectReq
			ntyClientReqJsonParse( clientActionParamVal->jsonObj, ptrclientSelectReq ); 
			
			int nVal = 0;
			if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_SCHEDULE)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:Schedule,process\n");
				ntyClientSelectScheduleReqAction( clientActionParamVal, ptrclientSelectReq );
			}else if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_CONTACTS)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:Contacts,process\n");
				ntyClientSelectContactsReqAction( clientActionParamVal, ptrclientSelectReq );
			}else if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_EFENCE)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:Efence,process\n");
				ntyClientSelectEfenceReqAction( clientActionParamVal, ptrclientSelectReq );
			}else if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_TURN)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:Turn,process\n");
				ntyClientSelectTurnReqAction( clientActionParamVal, ptrclientSelectReq );
			}else if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_RUNTIME)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:RunTime,process\n");
				ntyClientSelectRunTimeReqAction( clientActionParamVal, ptrclientSelectReq );
			}else if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_TIMETABLES)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:TimeTables,process\n");
				ntyClientSelectTimeTablesReqAction( clientActionParamVal, ptrclientSelectReq );
			}else if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_LOCATION)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:Location,process\n");
				ntyClientSelectLocationReqAction( clientActionParamVal, ptrclientSelectReq );
			}else if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_URL)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:URL,process\n");
				ntyClientSelectURLReqAction( clientActionParamVal, ptrclientSelectReq );
			}else if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_SERVICE)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:Service,process\n");
				ntyClientSelectServiceReqAction( clientActionParamVal, ptrclientSelectReq );
			}else if( (nVal=strcmp(ptrclientSelectReq->Category,NATTY_USER_PROTOCOL_INIT)) == 0 ){
				ntylog("receive user data  ptrclientSelectReq->Category:Init,process\n");
				ntyClientSelectInitReqAction( clientActionParamVal, ptrclientSelectReq );
			}else{}
		} else {
			size_t len_ActionParam = sizeof(ActionParam);
			ActionParam *pActionParam = malloc(len_ActionParam);
			if (pActionParam == NULL) {
				ntylog(" %s --> malloc failed ActionParam", __func__);
				ntyFreeJsonValue(json);
				free(jsonstring);
				return;
			}
			memset(pActionParam, 0, len_ActionParam); 
			pActionParam->fromId = fromId;
			pActionParam->toId = 0;
			pActionParam->json = json;
			pActionParam->jsonstring = jsonstring;
			pActionParam->jsonlen = jsonLength;
			pActionParam->index = 0;
			ntyUserDataReqAction(pActionParam);
			free(pActionParam);
		}
	//add end
		ntyFreeJsonValue(json);
		free(jsonstring);
#endif
		ntylog("====================end ntyUserDataPacketReqHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}

}


static const ProtocolFilter ntyUserDataPacketReqFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyUserDataPacketReqHandleRequest,
};



void ntyPerformancePacketHandleRequest(const void *_self, unsigned char *buffer, int length,const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_PERFORMANCE_REQ) {
		ntylog("====================begin ntyPerformancePacketHandleRequest action ==========================\n");

		
		ntylog("====================end ntyPerformancePacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}


static const ProtocolFilter ntyPerformancePacketFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyPerformancePacketHandleRequest,
};

void ntyMessagePushPacketHandleRequest(const void *_self, unsigned char *buffer, int length,const void* obj) {
	
	if (buffer[NTY_PROTO_MSGTYPE_IDX] == NTY_PROTO_MSG_PUSH_REQ) {
		ntylog("====================begin ntyMessagePushPacketHandleRequest action ==========================\n");

		const MessagePacket *msg = (const MessagePacket*)obj;
		if ( msg == NULL ) return ;

		C_DEVID fromId = *(C_DEVID*)(buffer+NTY_PROTO_MSG_PUSH_DEVID_IDX);
		C_DEVID toId = *(C_DEVID*)(buffer+NTY_PROTO_MSG_PUSH_RECVID_IDX);
		int nRet = 0;
		
		U16 jsonLength = *(U16*)( buffer+NTY_PROTO_MSG_PUSH_JSON_LENGTH_IDX );
		U8 *jsonstring = (U8*)malloc( jsonLength+1 );
		if ( jsonstring == NULL ) { 
			ntylog("ntyMessagePushPacketHandleRequest --> malloc failed jsonstring buffer\n");
			return ;
		}
		memset( jsonstring, 0, jsonLength+1 );
		memcpy( jsonstring, buffer+NTY_PROTO_MSG_PUSH_JSON_CONTENT_IDX, jsonLength );		
		ntylog( "ntyMessagePushPacketHandleRequest --> fromId:%lld, toId:%lld, json:%s, jsonlength:%d\n", fromId, toId, jsonstring, jsonLength );

	#if ENABLE_RBTREE_REPLACE_BPTREE
		void *map = ntyRBTreeMapInstance();
		Client *pClient = (Client *)ntyMapSearch( map, toId );
		if ( pClient == NULL ){
			ntylog( "ntyMessagePushPacketHandleRequest not exit:%lld\n", toId );
			return;
		}
	#else
		void *heap = ntyBHeapInstance();
		NRecord *record = ntyBHeapSelect( heap, toId );
		if ( record == NULL ) return ;
		Client *pClient = (Client *)record->value;
	#endif
		
		if ( pClient->deviceType != NTY_PROTO_CLIENT_IOS && pClient->deviceType != NTY_PROTO_CLIENT_IOS_PUBLISH 
			&& pClient->deviceType != NTY_PROTO_CLIENT_IOS_APP_B && pClient->deviceType != NTY_PROTO_CLIENT_IOS_APP_B_PUBLISH) {
			nRet = ntySendDataMessagePush( toId, (U8*)buffer, length );
			if ( nRet >= 0 ) {
				ntylog( "ntyMessagePushPacketHandleRequest send to app which is not ios app success\n" );
				ntyJsonCommonResult( fromId, NATTY_RESULT_CODE_SUCCESS );
			} else {
				ntylog( "ntyMessagePushPacketHandleRequest send to app which is not ios app failed\n" );
				ntyJsonCommonResult( fromId, NATTY_RESULT_CODE_ERR_DEVICE_NOTONLINE );
			}			
		} else{
			nRet = ntySendPushNotifyIos( toId, fromId, NTY_PUSH_POLICE_MSG_CONTEXT, 0 );
			if ( nRet = 0 ){
				ntylog( "ntyMessagePushPacketHandleRequest send to app which is ios app success\n" );
				ntyJsonCommonResult( fromId, NATTY_RESULT_CODE_SUCCESS );				
			}else {
				ntylog( "ntyMessagePushPacketHandleRequest send to app which is not ios app failed\n" );
				ntyJsonCommonResult( fromId, NATTY_RESULT_CODE_ERR_DEVICE_NOTONLINE );				
			}	
		}

		
		ntylog("====================end ntyMessagePushPacketHandleRequest action ==========================\n");
	} else if (ntyPacketGetSuccessor(_self) != NULL) {
		const ProtocolFilter * const *succ = ntyPacketGetSuccessor(_self);
		(*succ)->handleRequest(succ, buffer, length, obj);
	} else {
		ntylog("Can't deal with: %d\n", buffer[NTY_PROTO_MSGTYPE_IDX]);
	}
}


void ntyMonitorSleepPacketHandleRequest(const void *_self, unsigned char *buffer, int length,const void* obj) {
	
	//if ( buffer[NTY_PROTO_MSGTYPE_IDX] == 0x52 ) {
		ntylog("====================begin ntyMonitorSleepPacketHandleRequest action ==========================\n");
		int nRet = 0;
		const MessagePacket *msg = (const MessagePacket*)obj;
		if ( msg == NULL ) return ;

		C_DEVID fromId = *(C_DEVID*)(buffer+4);
		U16 jsonLength = *(U16*)( buffer+14 );
		const Client *client = msg->client;
		C_DEVID toId = client->devId;		

		U8 *jsonstring = (U8*)malloc( jsonLength+1 );
		if ( jsonstring == NULL ) { 
			ntylog("ntyMonitorSleepPacketHandleRequest --> malloc failed jsonstring buffer\n");
			return ;
		}
		memset( jsonstring, 0, jsonLength+1 );
		memcpy( jsonstring, buffer+16, jsonLength );		
		ntylog( "ntyMessagePushPacketHandleRequest receive json--> fromId:%lld,toId:%lld,json:%s,jsonlength:%d\n", fromId,toId,jsonstring, jsonLength );		
		JSON_Value *json = ntyMallocJsonValue( jsonstring );
		if ( json == NULL ) { //JSON Error and send Code to FromId Device
			ntylog( "ntyMessagePushPacketHandleRequest json malloc failed." );
			free( jsonstring );
			return;
		}
		//parse jsonstring
		CommonReq *pCommonReq = malloc( sizeof(CommonReq) );
		if ( pCommonReq == NULL ){
			ntylog(" %s --> malloc failed CommonReq", __func__ );
			free( jsonstring );
			return;
		}
		ntyJsonMonitorSleepReport( json, pCommonReq );
		//compose jsonstring to new jsonstring
		char *jsonNewStr = ntyMonitorSleepJsonCompose( pCommonReq );
		JSON_Value *jsonNewObj = ntyMallocJsonValue( jsonNewStr );
		U16 jsonNewLen = 0;
		if ( jsonNewStr != NULL ){
			jsonNewLen = strlen(jsonNewStr);
		}
		ntylog( "ntyMessagePushPacketHandleRequest compose json --> fromId:%lld,toId:%lld,json:%s,jsonlength:%d\n", fromId,toId,jsonNewStr, jsonNewLen );
		
		size_t len_ActionParam = sizeof( ActionParam );
		ActionParam *pActionParam = malloc( len_ActionParam );
		if ( pActionParam == NULL ) {
			ntylog( " %s --> malloc failed ActionParam", __func__ );
			free( jsonstring );
			free( pCommonReq );
			return ;
		}
		memset( pActionParam, 0, len_ActionParam );			 
		pActionParam->fromId = fromId;
		pActionParam->toId = toId;
		pActionParam->json = jsonNewObj;
		pActionParam->jsonstring = jsonNewStr;
		pActionParam->jsonlen = jsonNewLen;
		pActionParam->index = 0;

		ntyMonitorSleepReqAction( pActionParam );
		ntylog("====================end ntyMonitorSleepPacketHandleRequest action ==========================\n");
} 


static const ProtocolFilter ntyMessagePushPacketFilter = {
	sizeof(Packet),
	ntyPacketCtor,
	ntyPacketDtor,
	ntyPacketSetSuccessor,
	ntyPacketGetSuccessor,
	ntyMessagePushPacketHandleRequest,
};


static void ntySetSuccessor(void *_filter, void *_succ) {
	ProtocolFilter **filter = _filter;
	if (_filter && (*filter) && (*filter)->setSuccessor) {
		(*filter)->setSuccessor(_filter, _succ);
	}
}

static void ntyHandleRequest(void *_filter, unsigned char *buffer, U32 length, const void *obj) {
	ProtocolFilter **filter = _filter;
	if (_filter && (*filter) && (*filter)->handleRequest) {
#if 1 //update client timestamp

		const MessagePacket *msg = (const MessagePacket*)obj;
		if (msg == NULL) return ;
		
		const Client *client = msg->client;
		#if ENABLE_RBTREE_REPLACE_BPTREE
			ntyOnlineClientRBTree(client->devId);
		#else
			ntyOnlineClientHeap(client->devId);
		#endif
#endif
		(*filter)->handleRequest(_filter, buffer, length, obj);
	}
}


const void *pNtyLoginFilter = &ntyLoginFilter;
const void *pNtyHeartBeatFilter = &ntyHeartBeatFilter;
const void *pNtyLogoutFilter = &ntyLogoutFilter;
const void *pNtyTimeCheckFilter = &ntyTimeCheckFilter;
const void *pNtyICCIDReqFilter = &ntyICCIDReqFilter;

const void *pNtyVoiceReqFilter = &ntyVoiceReqFilter;
const void *pNtyVoiceAckFilter = &ntyVoiceAckFilter;

const void *pNtyCommonReqFilter = &ntyCommonReqFilter;
const void *pNtyCommonAckFilter = &ntyCommonAckFilter;

const void *pNtyVoiceDataReqFilter = &ntyVoiceDataReqFilter;
const void *pNtyVoiceDataAckFilter = &ntyVoiceDataAckFilter;

const void *pNtyOfflineMsgReqFilter = &ntyOfflineMsgReqFilter;
const void *pNtyOfflineMsgAckFilter = &ntyOfflineMsgAckFilter;

const void *pNtyRoutePacketFilter = &ntyRoutePacketFilter;
const void *pNtyUnBindDeviceFilter = &ntyUnBindDeviceFilter;

const void *pNtyBindDeviceFilter = &ntyBindDeviceFilter;
const void *pNtyBindConfirmReqPacketFilter = &ntyBindConfirmReqPacketFilter;

const void *pNtyMutlcastReqFilter = &ntyMutlcastReqFilter;
const void *pNtyMutlcastAckFilter = &ntyMutlcastAckFilter;

const void *pNtyLocationAsyncReqFilter = &ntyLocationAsyncReqFilter;
const void *pNtyWeatherAsyncReqFilter = &ntyWeatherAsyncReqFilter;

const void *pNtyUserDataPacketReqFilter = &ntyUserDataPacketReqFilter;

const void *pNtyBindDeviceVersionBFilter = &ntyBindDeviceVersionBFilter;
const void *pNtyUnBindDeviceVersionBFilter = &ntyUnBindDeviceVersionBFilter;

const void *pNtyPerformancePacketFilter = &ntyPerformancePacketFilter;
const void *pNtyMessagePushPacketFilter = &ntyMessagePushPacketFilter;



//ntyVoiceReqPacketHandleRequest

void* ntyProtocolFilterInit(void) {
	void *pLoginFilter = New(pNtyLoginFilter);
	void *pHeartBeatFilter = New(pNtyHeartBeatFilter);
	void *pLogoutFilter = New(pNtyLogoutFilter);
	
	void *pTimeCheckFilter = New(pNtyTimeCheckFilter);
	void *pICCIDReqFilter = New(pNtyICCIDReqFilter);
	
	void *pVoiceReqFilter = New(pNtyVoiceReqFilter);
	void *pVoiceAckFilter = New(pNtyVoiceAckFilter);

	void *pCommonReqFilter = New(pNtyCommonReqFilter);
	void *pCommonAckFilter = New(pNtyCommonAckFilter);

	void *pVoiceDataReqFilter = New(pNtyVoiceDataReqFilter);
	void *pVoiceDataAckFilter = New(pNtyVoiceDataAckFilter);

	void *pOfflineMsgReqFilter = New(pNtyOfflineMsgReqFilter);
	void *pOfflineMsgAckFilter = New(pNtyOfflineMsgAckFilter);

	void *pRoutePacketFilter = New(pNtyRoutePacketFilter);
	void *pUnBindDeviceFilter = New(pNtyUnBindDeviceFilter);
	
	void *pBindDeviceFilter = New(pNtyBindDeviceFilter);
	void *pBindConfirmReqPacketFilter = New(pNtyBindConfirmReqPacketFilter);
	
	void *pMutlcastReqFilter = New(pNtyMutlcastReqFilter);
	void *pMutlcastAckFilter = New(pNtyMutlcastAckFilter);
	
	void *pLocationAsyncReqFilter = New(pNtyLocationAsyncReqFilter);
	void *pWeatherAsyncReqFilter = New(pNtyWeatherAsyncReqFilter);

	void *pUserDataPacketReqFilter = New(pNtyUserDataPacketReqFilter);
	
	void *pBindDeviceVersionBFilter = New(pNtyBindDeviceVersionBFilter);
	void *pUnBindDeviceVersionBFilter = New(pNtyUnBindDeviceVersionBFilter);

	void *pPerformancePacketFilter = New(pNtyPerformancePacketFilter);
	void *pMessagePushPacketFilter = New(pNtyMessagePushPacketFilter);

	ntySetSuccessor(pLoginFilter, pHeartBeatFilter);
	ntySetSuccessor(pHeartBeatFilter, pLogoutFilter);
	ntySetSuccessor(pLogoutFilter, pTimeCheckFilter);
	ntySetSuccessor(pTimeCheckFilter, pICCIDReqFilter);
	
	ntySetSuccessor(pICCIDReqFilter, pVoiceReqFilter);
	ntySetSuccessor(pVoiceReqFilter, pVoiceAckFilter);
	
	ntySetSuccessor(pVoiceAckFilter, pCommonReqFilter);
	ntySetSuccessor(pCommonReqFilter, pCommonAckFilter);
	ntySetSuccessor(pCommonAckFilter, pVoiceDataReqFilter);

	ntySetSuccessor(pVoiceDataReqFilter, pVoiceDataAckFilter);
	ntySetSuccessor(pVoiceDataAckFilter, pOfflineMsgReqFilter);
	
	ntySetSuccessor(pOfflineMsgReqFilter, pOfflineMsgAckFilter);
	ntySetSuccessor(pOfflineMsgAckFilter, pRoutePacketFilter);
	
	ntySetSuccessor(pRoutePacketFilter, pUnBindDeviceFilter);
	ntySetSuccessor(pUnBindDeviceFilter, pBindDeviceFilter);
	
	ntySetSuccessor(pBindDeviceFilter, pBindConfirmReqPacketFilter);
	ntySetSuccessor(pBindConfirmReqPacketFilter, pMutlcastReqFilter);

	ntySetSuccessor(pMutlcastReqFilter, pMutlcastAckFilter);
	ntySetSuccessor(pMutlcastAckFilter, pLocationAsyncReqFilter);
	
	ntySetSuccessor(pLocationAsyncReqFilter, pWeatherAsyncReqFilter);
	ntySetSuccessor(pWeatherAsyncReqFilter, pUserDataPacketReqFilter);

	ntySetSuccessor(pUserDataPacketReqFilter, pBindDeviceVersionBFilter);
	
	ntySetSuccessor(pBindDeviceVersionBFilter, pUnBindDeviceVersionBFilter);
	ntySetSuccessor(pUnBindDeviceVersionBFilter, pPerformancePacketFilter);
	
	ntySetSuccessor(pPerformancePacketFilter, pMessagePushPacketFilter);
	ntySetSuccessor(pMessagePushPacketFilter, NULL);

	
	/*
	 * add your Filter
	 * for example:
	 * void *pFilter = New(NtyFilter);
	 * ntySetSuccessor(pLogoutFilter, pFilter);
	 */

	//Gen Crc Table
	ntyGenCrcTable();

#if ENABLE_NATTY_TIME_STAMP
	pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
	memcpy(&time_mutex, &blank_mutex, sizeof(blank_mutex));
	memcpy(&loop_mutex, &blank_mutex, sizeof(blank_mutex));
#endif

	return pLoginFilter;
}

static void ntyClientBufferRelease(Client *client) {
	pthread_mutex_lock(&client->bMutex);
#if 0
	free(client->recvBuffer);
	client->recvBuffer = NULL;
	client->rLength = 0;
#else
	memset(client->recvBuffer, 0, PACKET_BUFFER_SIZE);
	client->rLength = 0;
#endif
	pthread_mutex_unlock(&client->bMutex);
}

void ntyProtocolFilterProcess(void *_filter, unsigned char *buffer, U32 length, const void *obj) {
	//usleep(10); //many threads access
	ntylog("******ntyProtocolFilterProcess receive buffer buffer[0]:%x,buffer[1]:%x,buffer[2]:%x,buffer[3]:%x\n",buffer[0],buffer[1],buffer[2],buffer[3]);
	//ntydbg("******ntyProtocolFilterProcess receive buffer buffer[0]:%x,buffer[1]:%x,buffer[2]:%x,buffer[3]:%x\n",buffer[0],buffer[1],buffer[2],buffer[3]);
	//firstly parse the third parten
	if ( buffer[0]==0x45 && buffer[1]==0x44 && buffer[2]==0x09  ){
		ntylog("**********ntyProtocolFilterProcess the third party protocol begin**************\n");
		if ( buffer[3]==0x52 ){
			ntylog("**********ntyProtocolFilterProcess the third party protocol 0x52**************\n");
			ntyMonitorSleepPacketHandleRequest( _filter, buffer, length, obj );
			return;
		}else if ( buffer[3]==0x01 ){  //login
			ntylog("**********ntyProtocolFilterProcess the 3th protocol 0x01 login**************\n");
		}else{}
	}

	//data crc is right, and encryto
	U32 u32Crc = ntyGenCrcValue(buffer, length-4);
	U32 u32ClientCrc = *((U32*)(buffer+length-4));

	const MessagePacket *msg = (const MessagePacket*)obj;
	if ( msg == NULL ){ 
		ntylog("ntyProtocolFilterProcess msg==NULL\n");
		return ;
	}
	const Client *client = msg->client;
	if ( client == NULL ){ 
		ntylog("ntyProtocolFilterProcess client==NULL\n");
		return ;
	}
	#if 0	
	struct sockaddr_in addr;
	memcpy(&addr, &client->addr, sizeof(struct sockaddr_in));

	C_DEVID id = ntySearchDevIdFromHashTable(&addr);
	ntylog("ntyProtocolFilterProcess :%lld\n", id);
	if (id == 0) return ;
	#endif
	
	#if ENABLE_RBTREE_REPLACE_BPTREE
		void *map = ntyRBTreeMapInstance();
		Client *pClient = (Client *)ntyMapSearch( map, client->devId );	
		if ( pClient == NULL ){ 
			ntylog( "ntyProtocolFilterProcess rbtree not exit:%lld\n",client->devId );
			return;
		}
	#else
		void *pBHeap = ntyBHeapInstance();
		NRecord *Record = (NRecord*)ntyBHeapSelect(pBHeap, client->devId);
		if ( Record == NULL ){
			ntylog( "ntyProtocolFilterProcess bplustree not exit:%lld\n",client->devId );
			return;
		}
		Client *pClient = Record->value;
		if ( pClient == NULL ){
			ntylog( "ntyProtocolFilterProcess bplustree client not exit:%lld\n",client->devId );
			return;
		}
	#endif
	
	if ( u32Crc != u32ClientCrc || length < NTY_PROTO_MIN_LENGTH ) {
		ntylog("ntyProtocolFilterProcess clientcrc:%x, servercrc:%x, buffer length:%d,devid:%lld\n", u32ClientCrc, u32Crc, length,client->devId);
		if ( u32Crc != u32ClientCrc || length < NTY_PROTO_MIN_LENGTH ){	//modify by Rain 2017-10-16	crc is wrong
			return; 
		}		
		if (1) { //have part data client->connectType == PROTO_TYPE_TCP
			int bCopy = 0;
			int bIndex = 0, bLength = pClient->rLength;
			U8 bBuffer[PACKET_BUFFER_SIZE] = {0};
			do {
				if(ntyIsPacketHeader(buffer[NTY_PROTO_VERSION_IDX]) && ntyIsVoicePacketHeader(buffer[NTY_PROTO_MSGTYPE_IDX])) {
					bCopy = ntyVoicePacketLength(buffer);
					ntylog( "packet bCopy:%d,length:%d\n", bCopy,length );
					if ( bCopy < length ) {
						ntylog("complete voice packet\n");
						u32Crc = ntyGenCrcValue(buffer, bCopy-4);
						u32ClientCrc = *((U32*)(buffer+bCopy-4));
						if ( u32Crc == u32ClientCrc ) {
							ntylog("packet MSG:%x,Version:[%c]\n", buffer[NTY_PROTO_MSGTYPE_IDX], buffer[NTY_PROTO_DEVTYPE_IDX] );
							ntyHandleRequest(_filter, buffer, bCopy, obj);
							ntyClientBufferRelease(pClient);
						}
						memcpy(pClient->recvBuffer, buffer+bCopy, length-bCopy);
						pClient->rLength = length-bCopy;
					} else if ( bCopy == length ) {
						ntylog("u32Crc==u32ClientCrc,must be error\n");
						return ntyHandleRequest(_filter, buffer, length, obj);
					} else {
						ntylog("no-complete voice packet\n");
						memcpy(pClient->recvBuffer, buffer, length);
						pClient->rLength = length;
						break;
					}
					length -= bCopy;
					buffer += bCopy;
					
				} else {		
					bCopy = (length > PACKET_BUFFER_SIZE ? PACKET_BUFFER_SIZE : length);					
					bCopy = (((bLength + bCopy) > PACKET_BUFFER_SIZE) ? (PACKET_BUFFER_SIZE - bLength) : bCopy);	
					ntylog( "not head packet bCopy:%d,length:%d\n", bCopy,length );
					pthread_mutex_lock(&pClient->bMutex);
					memcpy(pClient->recvBuffer+pClient->rLength, buffer+bIndex, bCopy);
					pClient->rLength %= PACKET_BUFFER_SIZE;
					pClient->rLength += bCopy;

					memset(bBuffer, 0, PACKET_BUFFER_SIZE);
					memcpy(bBuffer, pClient->recvBuffer, pClient->rLength);
					bLength = pClient->rLength;
					pthread_mutex_unlock(&pClient->bMutex);

					U32 uCrc = ntyGenCrcValue(bBuffer, bLength-4);
					U32 uClientCrc = *((U32*)(bBuffer+bLength-4));
					if ( uCrc == uClientCrc )	{
						ntylog("MSG:%x, Version:[%c]\n", bBuffer[NTY_PROTO_MSGTYPE_IDX], bBuffer[NTY_PROTO_DEVTYPE_IDX]);
						ntyHandleRequest(_filter, bBuffer, bLength, obj);
						ntyClientBufferRelease(pClient);
					} 
					
					length -= bCopy;
					bIndex += bCopy;			
				}
				bCopy = 0;		
			} while (length);
			return;
		}		
	}

	if ( pClient != NULL ) {
		pClient->rLength = 0;
	}

	return ntyHandleRequest(_filter, buffer, length, obj);
}

void ntyProtocolFilterRelease(void *_filter) {
	Packet *self = _filter;
	if (ntyPacketGetSuccessor(self) != NULL) {
		ntyProtocolFilterRelease(self->succ);
	}
	Delete(self);
}

static void *ntyProtocolFilter = NULL;

void* ntyProtocolFilterInstance(void) {
	if (ntyProtocolFilter == NULL) {
		void* pFilter = ntyProtocolFilterInit();
		if ((unsigned long)NULL != cmpxchg((void*)(&ntyProtocolFilter), (unsigned long)NULL, (unsigned long)pFilter, WORD_WIDTH)) {
			Delete(pFilter);
		}
	}
	return ntyProtocolFilter;
}

#if 1
#define NTY_CRCTABLE_LENGTH			256
#define NTY_CRC_KEY					0x04c11db7ul

static U32 u32CrcTable[NTY_CRCTABLE_LENGTH] = {0};


void ntyGenCrcTable(void) {
	U16 i,j;
	U32 u32CrcNum = 0;

	for (i = 0;i < NTY_CRCTABLE_LENGTH;i ++) {
		U32 u32CrcNum = (i << 24);
		for (j = 0;j < 8;j ++) {
			if (u32CrcNum & 0x80000000L) {
				u32CrcNum = (u32CrcNum << 1) ^ NTY_CRC_KEY;
			} else {
				u32CrcNum = (u32CrcNum << 1);
			}
		}
		u32CrcTable[i] = u32CrcNum;
	}
}

U32 ntyGenCrcValue(U8 *buf, int length) {
	U32 u32CRC = 0xFFFFFFFF;
	
	while (length -- > 0) {
		u32CRC = (u32CRC << 8) ^ u32CrcTable[((u32CRC >> 24) ^ *buf++) & 0xFF];
	}

	return u32CRC;
}

#else
	
#define NTY_CRCTABLE_LENGTH			256
#define NTY_CRC_KEY					0xedb88320

static U32 u32CrcTable[NTY_CRCTABLE_LENGTH] = {0};


void ntyGenCrcTable(void) {
	U16 i,j;
	U32 u32CrcNum = 0;

	for (i = 0;i < NTY_CRCTABLE_LENGTH;i ++) {
		u32CrcNum = i;
		for (j = 0;j < 8;j ++) {
			if (u32CrcNum & 1) {
				u32CrcNum >>= 1;
				u32CrcNum ^= NTY_CRC_KEY;
			} else {
				u32CrcNum >>= 1;
			}
		}
		u32CrcTable[i] = u32CrcNum;
	}
}

U32 ntyGenCrcValue(U8 *buf, int length) {
	U32 u32CRC = 0;
	U8 *p, *q;
	U8 octet;
	
	u32CRC = ~u32CRC;
	q = buf + length;
	for (p = buf; p < q;p ++) {
		octet = *p;
		u32CRC = (u32CRC >> 8) ^ u32CrcTable[(u32CRC & 0xff) ^ octet];
	}

	return ~u32CRC;
}

#endif

#if 0

int ntyReleaseClientNodeByNode(struct ev_loop *loop, void *node) {
	Client *client = node;
	if (client == NULL) return -4;
#if 0
	ntylog("ntyDeleteNodeFromHashTable Start --> %d.%d.%d.%d:%d \n", *(unsigned char*)(&client->addr.sin_addr.s_addr), *((unsigned char*)(&client->addr.sin_addr.s_addr)+1),													
				*((unsigned char*)(&client->addr.sin_addr.s_addr)+2), *((unsigned char*)(&client->addr.sin_addr.s_addr)+3),													
				client.addr->sin_port);
#endif
	int ret = ntyDeleteNodeFromHashTable(&client->addr, client->devId);
	ASSERT(ret == 0);

#if 1 //
	if (client->deviceType == DEVICE_TYPE_WATCH) {
		ret = ntyExecuteDeviceLogoutUpdateHandle(client->devId);
		ntylog(" ntyReleaseClientNodeByNode --> Logout ret : %d\n", ret);
	}
#endif

	ret = ntyReleaseClientNodeSocket(loop, client->watcher, client->watcher->fd);
	ASSERT(ret == 0);

	void *tree = ntyRBTreeInstance();
	ret = ntyRBTreeInterfaceDelete(tree, client->devId);
	ASSERT(ret == 0);
		
	return 0;
}

int ntyReleaseClientNodeByDevID(struct ev_loop *loop, struct ev_io *watcher, C_DEVID devid) {
	int ret = -1;
	void *tree = ntyRBTreeInstance();

	Client *client = ntyRBTreeInterfaceSearch(tree, devid);
	if (client == NULL) {
		ntylog(" ntyReleaseClientNodeByDevID --> Client == NULL\n");
		return -1;
	}
	ntyReleaseClientNodeSocket(loop, client->watcher, client->watcher->fd);
	
	ret = ntyRBTreeInterfaceDelete(tree, devid);
	ASSERT(ret == 0);
	
	return 0;
}

int ntyReleaseClientNodeByAddr(struct ev_loop *loop, struct sockaddr_in *addr, struct ev_io *watcher) {
	ntylog(" ntyReleaseClientNodeByAddr --> Start \n");
	
	C_DEVID devid = ntySearchDevIdFromHashTable(addr);
	int ret = ntyDeleteNodeFromHashTable(addr, devid);
	if (ret == -1) {
		ntylog("Delete Node From Hash Table Parameter is Error\n");
		return -1;
	} else if (ret == -2) {
		ntylog("Hash Table Node is not Exist\n");
	}

#if 1 //Update Login info
	ntyExecuteDeviceLogoutUpdateHandle(devid);
#endif

	ntylog("Delete Node Success devid : %lld\n", devid);
	ntyReleaseClientNodeSocket(loop, watcher, watcher->fd);
	
	void *tree = ntyRBTreeInstance();
	//ntyRBTreeOperaDelete
	// delete rb-tree node
	ret = ntyRBTreeInterfaceDelete(tree, devid);
	if (ret == 1) {
		ntylog("RBTree Node Not Exist\n");
		return -2;
	} else if (ret == -1) {
		ntylog("RBTree Node Parameter is Error\n");
		return -3;
	}

	ntylog("RBTree Delete Node Success\n");

	return 0;
}

int ntyReleaseClientNodeSocket(struct ev_loop *loop, struct ev_io *watcher, int sockfd) {
	if (watcher == NULL) { 
		ntylog(" ntyReleaseClientNodeSocket --> watcher == NULL\n");
		return -1;
	}
	if (watcher->fd != sockfd) {
		ntylog(" ntyReleaseClientNodeSocket --> watcher->fd :%d, sockfd :%d\n", watcher->fd, sockfd);
		return -1;
	}
	if (sockfd < 0) return -2;
	
#if ENABLE_MAINLOOP_MUTEX //TIME Stamp 	
	pthread_mutex_lock(&loop_mutex);
	ev_io_stop(loop, watcher);
	pthread_mutex_unlock(&loop_mutex);
#endif

	close(sockfd);
	sockfd = -1;
	free(watcher);

	return 0;
}


int ntyReleaseClientNodeHashTable(struct sockaddr_in *addr) {
	C_DEVID devid = ntySearchDevIdFromHashTable(addr);

	ntylog(" ntyReleaseClientNodeHashTable --> %lld\n", devid);
	int ret = ntyDeleteNodeFromHashTable(addr, devid);
	if (ret == -1) {
		ntylog("Delete Node From Hash Table Parameter is Error\n");
		return -1;
	} else if (ret == -2) {
		ntylog("Hash Table Node is not Exist\n");
	}
	return 0;
}

#endif

