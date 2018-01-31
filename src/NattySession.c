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

 
#include "NattyFilter.h"
#include "NattySession.h"
#include "NattyRBTree.h"
#include "NattyBPlusTree.h"
#include "NattyUtils.h"
#include "NattyMulticast.h"
#include "NattyPush.h"

#include "NattyJson.h"

#include "NattyVector.h"
#include "NattyResult.h"

#include "NattyDaveMQ.h"
#include "NattySqlHandle.h"

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                   \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

 

#define BHEAP_VECTOR_ENABLE 	1

/*
 * send friends list to client
 */

#if 0
int ntyNotifyClient(UdpClient *client, U8 *notify) {
	int length = 0;
	
	notify[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_P2P_NOTIFY_REQ;
	
	length = NTY_PROTO_P2P_NOTIFY_CRC_IDX;
	*(U32*)(&notify[length]) = ntyGenCrcValue(notify, length);
	length += sizeof(U32);

	return ntySendBuffer(client, notify, length);

}

C_DEVID ntyClientGetDevId(void *client) {
	RBTreeNode *pNode = container_of(&client, RBTreeNode, value);
	return pNode->key;
}

/*
 * send client Ip to friend
 */

int ntyNotifyFriendConnect(void* fTree, C_DEVID id) {
	U8 notify[NTY_LOGIN_ACK_LENGTH] = {0};
	
	UdpClient *pClient = container_of(&fTree, UdpClient, friends);	
	void *pRBTree = ntyRBTreeInstance();

	UdpClient *client = ntyRBTreeInterfaceSearch(pRBTree, id);
	if (client == NULL) return -1;


	notify[NTY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	notify[NTY_PROTO_PROTOTYPE_IDX] = (U8)MSG_REQ;
	notify[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_P2P_NOTIFY_REQ;


	*(C_DEVID*)(&notify[NTY_PROTO_P2P_NOTIFY_DEVID_IDX]) = ntyClientGetDevId(pClient);
#if 0
	*(U32*)(&notify[NTY_PROTO_P2P_NOTIFY_ACKNUM_IDX]) = pClient->ackNum;
			
	*(U32*)(&notify[NTY_PROTO_P2P_NOTIFY_IPADDR_IDX]) = pClient->addr.sin_addr.s_addr;
	*(U16*)(&notify[NTY_PROTO_P2P_NOTIFY_IPPORT_IDX]) = pClient->addr.sin_port;
#endif
	return ntyNotifyClient(client, notify);
}

int ntyNotifyFriendMessage(C_DEVID fromId, C_DEVID toId) {
	U8 notify[NTY_LOGIN_ACK_LENGTH] = {0};
	
	void *pRBTree = ntyRBTreeInstance();
	UdpClient *toClient = ntyRBTreeInterfaceSearch(pRBTree, toId);
	if (toClient == NULL) return -1;

	UdpClient *fromClient = ntyRBTreeInterfaceSearch(pRBTree, fromId);
	if (fromClient == NULL) return -1;

	notify[NTY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	notify[NTY_PROTO_PROTOTYPE_IDX] = (U8)MSG_REQ;
	notify[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_P2P_NOTIFY_REQ;

	
	*(C_DEVID*)(&notify[NTY_PROTO_P2P_NOTIFY_DEVID_IDX]) = fromId;
	*(C_DEVID*)(&notify[NTY_PROTO_P2P_NOTIFY_DEST_DEVID_IDX]) = toId;
#if 0
	*(U32*)(&notify[NTY_PROTO_P2P_NOTIFY_ACKNUM_IDX]) = fromClient->ackNum;
	*(U32*)(&notify[NTY_PROTO_P2P_NOTIFY_IPADDR_IDX]) = fromClient->addr.sin_addr.s_addr;
	*(U16*)(&notify[NTY_PROTO_P2P_NOTIFY_IPPORT_IDX]) = fromClient->addr.sin_port;
#endif
	return ntyNotifyClient(toClient, notify);
}
#endif


/*
 * send friends list to client
 */
int ntySendFriendsTreeIpAddr(const void *client, U8 reqType) {
	int i = 0, length;
	U8 ack[NTY_LOGIN_ACK_LENGTH] = {0};
#if 0	
	const Client *pClient = client;
	void *pRBTree = ntyRBTreeInstance();

	C_DEVID *friends = ntyFriendsTreeGetAllNodeList(pClient->friends);
	U16 Count = ntyFriendsTreeGetNodeCount(pClient->friends);
	//ntylog("Count : %d, type:%d\n", Count, pClient->connectType);
	for (i = 0;i < Count;i ++) {
		Client *cliValue = ntyRBTreeInterfaceSearch(pRBTree, *(friends+i));
		*(C_DEVID*)(&ack[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_DEVID_IDX(i)]) = *(friends+i);
	}
	free(friends);
#else

	const Client *pClient = client;

	void *map = ntyMapInstance();
	ClientSocket *toClient = (ClientSocket *)ntyMapSearch(map, pClient->devId);

	//NVector *vector = pClient->friends;
	int Count = 0;
	C_DEVID *friends = ntyVectorGetNodeList(pClient->friends, &Count);
	if (friends != NULL) {
		for (i = 0;i < Count;i ++) {
			*(C_DEVID*)(&ack[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_DEVID_IDX(i)]) = *(friends+i);
		}
		free(friends);
	}
	
	
#endif
	ack[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	ack[NTY_PROTO_PROTOTYPE_IDX] = (U8)MSG_UPDATE;
	if (reqType) {
		ack[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_LOGIN_ACK;
	} else {
		ack[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_HEARTBEAT_ACK;
	}
	//*(U32*)(&ack[NTY_PROTO_LOGIN_ACK_ACKNUM_IDX]) = pClient->ackNum;
	*(U16*)(&ack[NTY_PROTO_LOGIN_ACK_FRIENDS_COUNT_IDX]) = Count;
	*(U32*)(&ack[NTY_PROTO_LOGIN_ACK_CRC_IDX]) = ntyGenCrcValue(ack, NTY_PROTO_LOGIN_ACK_CRC_IDX);
	length += NTY_PROTO_LOGIN_ACK_CRC_IDX+sizeof(U32);

	return ntySendBuffer(toClient, ack, length);
}


int ntySendDeviceTimeCheckAck(C_DEVID fromId, U32 ackNum) {
	int length = 0;
	U8 ack[RECV_BUFFER_SIZE] = {0};

	
	ack[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	ack[NTY_PROTO_PROTOTYPE_IDX] = (U8)PROTO_ACK;
	ack[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_TIME_CHECK_ACK;
#if 0
	ack[NTY_PROTO_DEVID_IDX] = pClient->devId;
#else
	memcpy(ack+NTY_PROTO_DEVID_IDX, &fromId, sizeof(C_DEVID));
#endif
	*(U32*)(&ack[NTY_PROTO_ACKNUM_IDX]) = ackNum;

	ntyTimeCheckStamp(ack);
	
	//*(U32*)(&ack[NTY_PROTO_TIMECHECK_CRC_IDX]) = ntyGenCrcValue(ack, NTY_PROTO_LOGIN_REQ_CRC_IDX);
	length = NTY_PROTO_TIME_ACK_CRC_IDX+sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *nSocket = (ClientSocket *)ntyMapSearch(map, fromId);
	
	return ntySendBuffer(nSocket, ack, length);
	
}

int ntySendDeviceRouterInfo(const Client *pClient, U8 *buffer, int length) {
	U8 buf[RECV_BUFFER_SIZE] = {0};
	
	buf[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buf[NTY_PROTO_PROTOTYPE_IDX] = (U8) MSG_UPDATE;	
	buf[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_DATAPACKET_REQ;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]) = pClient->devId;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]) = pClient->devId;

	U8 *tempBuf = &buf[NTY_PROTO_DATAPACKET_CONTENT_IDX];
	memcpy(tempBuf, buffer, length);
	
	*(U16*)(&buf[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)length;
	length += NTY_PROTO_DATAPACKET_CONTENT_IDX;
	length += sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *nSocket = (ClientSocket *)ntyMapSearch(map, pClient->devId);

	return ntySendBuffer(nSocket, buf, length);
}

int ntySendAppRouterInfo(const Client *pClient, C_DEVID fromId, U8 *buffer, int length) {
	U8 buf[RECV_BUFFER_SIZE] = {0};
	int i = 0;
	
	buf[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buf[NTY_PROTO_PROTOTYPE_IDX] = (U8) MSG_REQ;	
	buf[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_DATAPACKET_REQ;
#if 0
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]) = fromId;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]) = pClient->devId;
#else
	memcpy(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX], &fromId, sizeof(C_DEVID));
	memcpy(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX], &pClient->devId, sizeof(C_DEVID));
#endif
	U8 *tempBuf = &buf[NTY_PROTO_DATAPACKET_CONTENT_IDX];
	memcpy(tempBuf, buffer, length);

	ntylog(" ntySendAppRouterInfo --> fromId:%lld, toId:%lld, cmd:%s, length:%d, type:%x\n", *(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]),
		*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]), tempBuf, length, buf[NTY_PROTO_MSGTYPE_IDX]);
	
	*(U16*)(&buf[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)length;
	length += NTY_PROTO_DATAPACKET_CONTENT_IDX;
	length += sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *nSocket = (ClientSocket *)ntyMapSearch(map, pClient->devId);

	return ntySendBuffer(nSocket, buf, length);
}


static int ntyBoardcastItem(void* client, C_DEVID toId, U8 *data, int length) {
	Client *selfNode = client;

	ntylog(" ntyBoardcastItem --> Start\n");
	if (selfNode->devId == toId) return -1;
	
	void *pRBTree = ntyRBTreeInstance();
	Client *toClient = (UdpClient*)ntyRBTreeInterfaceSearch(pRBTree, toId);
	if (toClient == NULL) { //local have no client node and need to multicast 
		ntylog(" ntyBoardcastItem --> toId:%lld is not Exist\n", toId);
		
#if ENABLE_MULTICAST_SYNC //multicast 
		//data[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_MULTICAST_REQ;
		int len = length;
		U8 buffer[RECV_BUFFER_SIZE] = {0};

		buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
		buffer[NTY_PROTO_PROTOTYPE_IDX] = (U8) MSG_REQ;	
		buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_DATAPACKET_REQ;
		//buffer[NTY_PROTO_DEVID_IDX] = 
		memcpy(buffer+NTY_PROTO_DEVID_IDX, &selfNode->devId, sizeof(C_DEVID));
		memcpy(buffer+NTY_PROTO_DEST_DEVID_IDX, &toId, sizeof(C_DEVID));
		*(U16*)(&buffer[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)len;
		memcpy(buffer+NTY_PROTO_DATAPACKET_CONTENT_IDX, data, length);

		len += NTY_PROTO_DATAPACKET_CONTENT_IDX;
		len += sizeof(U32);
		
		ntyMulticastSend(buffer, len);
#endif
		return -1;
	}

	ntylog(" ntyBoardcastItem --> fromId:%lld, toId:%lld, data:%s, length:%d\n", selfNode->devId, toId, data, length);
	return ntySendAppRouterInfo(toClient, selfNode->devId, data, length);
}


int ntyBoardcastAllFriends(const Client *self, U8 *buffer, int length) {
	void *pRBTree = ntyRBTreeInstance();

	ntylog("ntyBoardcastAllFriends --> self devid:%lld\n", self->devId);
	Client *selfNode = (UdpClient*)ntyRBTreeInterfaceSearch(pRBTree, self->devId);
	if (selfNode != NULL) {
		RBTree *friends = (RBTree*)selfNode->friends;
		if (friends != NULL) {
			ntylog("ntyBoardcastAllFriends --> self devid:%lld, selfNode.id:%lld\n", self->devId, selfNode->devId);
			ntyFriendsTreeBroadcast(friends, ntyBoardcastItem, selfNode, buffer, length);
		}
	}
	return 0;
}

int ntyBoardcastAllFriendsById(C_DEVID fromId, U8 *buffer, int length) {
	void *pRBTree = ntyRBTreeInstance();

	ntylog("ntyBoardcastAllFriendsById --> self devid:%lld\n", fromId);
	Client *selfNode = (UdpClient*)ntyRBTreeInterfaceSearch(pRBTree, fromId);
	if (selfNode != NULL) {
		RBTree *friends = (RBTree*)selfNode->friends;
		if (friends != NULL) {
			ntylog("ntyBoardcastAllFriendsById --> self devid:%lld, selfNode.id:%lld\n", fromId, selfNode->devId);
			//if (fromId != selfNode->devId)
			ntyFriendsTreeBroadcast(friends, ntyBoardcastItem, selfNode, buffer, length);
		}
	}
}

int ntyBoardcastAllFriendsNotifyDisconnect(C_DEVID selfId) {
	U8 u8ResultBuffer[256] = {0};
	
	ntylog(" ntyBoardcastAllFriendsNotifyDisconnect --> Notify All Friends\n");
	sprintf(u8ResultBuffer, "Set Disconnect 1");
	ntyBoardcastAllFriendsById(selfId, u8ResultBuffer, strlen(u8ResultBuffer));

	return 0;
}

int ntyBoardcastAllFriendsNotifyConnect(C_DEVID selfId) {
	U8 u8ResultBuffer[256] = {0};
	
	ntylog(" ntyBoardcastAllFriendsNotifyConnect --> Notify All Friends\n");
	sprintf(u8ResultBuffer, "Set Connect 1");
	ntyBoardcastAllFriendsById(selfId, u8ResultBuffer, strlen(u8ResultBuffer));

	return 0;
}


#if 1
void ntyProtoHttpProxyTransform(C_DEVID fromId, C_DEVID toId, U8 *buffer, int length) {
	int n = 0;
	U8 buf[RECV_BUFFER_SIZE] = {0};

	buf[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buf[NTY_PROTO_PROTOTYPE_IDX] = (U8) MSG_REQ;	
	buf[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_DATAPACKET_REQ;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]) = fromId;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]) = toId;
	
	*(U16*)(&buf[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)length;
	memcpy(buf+NTY_PROTO_DATAPACKET_CONTENT_IDX, buffer, length);
	ntylog(" ntyProtoHttpProxyTransform ---> %s\n", buf+NTY_PROTO_DATAPACKET_CONTENT_IDX);
	
	length += NTY_PROTO_DATAPACKET_CONTENT_IDX;

	*(U32*)(&buf[length]) = ntyGenCrcValue(buf, length);
	length += sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *nSocket = (ClientSocket *)ntyMapSearch(map, toId);
	ntySendBuffer(nSocket, buf, length);
	
}

void ntyProtoHttpRetProxyTransform(C_DEVID toId, U8 *buffer, int length) {
	int n = 0;
	U8 buf[RECV_BUFFER_SIZE] = {0};
#if 0
	void *pRBTree = ntyRBTreeInstance();
	if (pRBTree == NULL) return ;
	UdpClient *destClient = (UdpClient*)ntyRBTreeInterfaceSearch(pRBTree, toId);
	if (destClient == NULL) {
		ntylog(" destClient is not exist, toId:%lld\n", toId);
		return ;
	}
#else
	void *map = ntyMapInstance();
	ClientSocket *nSocket = (ClientSocket *)ntyMapSearch(map, toId);

#endif
	buf[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buf[NTY_PROTO_PROTOTYPE_IDX] = (U8) MSG_REQ;	
	buf[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_ROUTERDATA_REQ;
	//*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]) = fromId;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]) = toId;
	
	*(U16*)(&buf[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)length;
	memcpy(buf+NTY_PROTO_DATAPACKET_CONTENT_IDX, buffer, length);

	ntylog(" ntyProtoHttpRetProxyTransform ---> %s, length:%d\n", buf+NTY_PROTO_DATAPACKET_CONTENT_IDX, length);
	length += NTY_PROTO_DATAPACKET_CONTENT_IDX;

	*(U32*)(&buf[length]) = ntyGenCrcValue(buf, length);
	length += sizeof(U32);

	ntySendBuffer(nSocket, buf, length);
	
}


#endif


/*
 * result : 0 --> OK
 *          1 --> AppId No Exist 
 *          2 --> DevId No Exist
 *          3 --> Have Bound
 *		 4 --> DB Exception
 */
void ntyProtoBindAck(C_DEVID aid, C_DEVID did, int result) {
	int length = 0;
	U8 buf[NTY_HEARTBEAT_ACK_LENGTH] = {0};
#if 0	
	void *pRBTree = ntyRBTreeInstance();
	Client *client = (Client*)ntyRBTreeInterfaceSearch(pRBTree, aid);
#else
	void *map = ntyMapInstance();
	ClientSocket *nSocket = (ClientSocket *)ntyMapSearch(map, aid);
#endif
	if (nSocket == NULL) {
		ntylog(" destClient is not exist proxy:%lld\n", aid);
		return ;
	}

	buf[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buf[NTY_PROTO_PROTOTYPE_IDX] = (U8) MSG_ACK;	
	buf[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_BIND_ACK;
	*(C_DEVID*)(&buf[NTY_PROTO_BIND_ACK_DEVICEID_IDX]) = did;
	*(int*)(&buf[NTY_PROTO_BIND_ACK_RESULT_IDX]) = result;

	length = NTY_PROTO_BIND_ACK_CRC_IDX;
	//*(U32*)(&buf[length]) = ntyGenCrcValue(buf, length);
	length += sizeof(U32);

	ntySendBuffer(nSocket, buf, length);
}


/*
 * result : 0 --> OK
 *          1 --> AppId No Exist 
 *          2 --> DevId No Exist
 *          3 --> Have Bound
 *		 4 --> DB Exception
 */
 
void ntyProtoUnBindAck(C_DEVID aid, C_DEVID did, int result) {
	int length = 0;
	U8 buf[NTY_HEARTBEAT_ACK_LENGTH] = {0};
	
#if 0	
	void *pRBTree = ntyRBTreeInstance();
	Client *client = (Client*)ntyRBTreeInterfaceSearch(pRBTree, aid);
#else
	void *map = ntyMapInstance();
	ClientSocket *nSocket = (ClientSocket *)ntyMapSearch(map, aid);
#endif
	if (nSocket == NULL) {
		ntylog(" destClient is not exist proxy:%lld\n", aid);
		return ;
	}

	buf[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buf[NTY_PROTO_PROTOTYPE_IDX] = (U8) MSG_ACK;	
	buf[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_UNBIND_ACK;
	*(C_DEVID*)(&buf[NTY_PROTO_UNBIND_ACK_DEVICEID_IDX]) = did;
	*(int*)(&buf[NTY_PROTO_UNBIND_ACK_RESULT_IDX]) = result;

	length = NTY_PROTO_UNBIND_ACK_CRC_IDX;
	//*(U32*)(&buf[length]) = ntyGenCrcValue(buf, length);
	length += sizeof(U32);

	ntySendBuffer(nSocket, buf, length);
}


void ntyProtoICCIDAck(C_DEVID did, U8 *phnum, U16 length) {
	U8 buf[NTY_HEARTBEAT_ACK_LENGTH*2] = {0};
#if 0	
	void *pRBTree = ntyRBTreeInstance();
	Client *client = (Client*)ntyRBTreeInterfaceSearch(pRBTree, did);
#else
	void *map = ntyMapInstance();
	ClientSocket *nSocket = (ClientSocket *)ntyMapSearch(map, did);
#endif
	if (nSocket == NULL) {
		ntylog(" destClient is not exist proxy:%lld\n", did);
		return ;
	}

	buf[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buf[NTY_PROTO_PROTOTYPE_IDX] = (U8) MSG_ACK;	
	buf[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_ICCID_ACK;

	memcpy(buf+NTY_PROTO_ICCIDACK_SLFID_IDX, &did, sizeof(C_DEVID));
	memcpy(buf+NTY_PROTO_ICCIDACK_CONTENT_COUNT_IDX, &length, sizeof(U16));
	memcpy(buf+NTY_PROTO_ICCIDACK_CONTENT_IDX, phnum, length);

	length += NTY_PROTO_ICCIDACK_CONTENT_IDX;
	//*(U32*)(&buf[length]) = ntyGenCrcValue(buf, length);
	length += sizeof(U32);

	ntySendBuffer(nSocket, buf, length);
}

void ntySelfLogoutPacket(C_DEVID id, U8 *buffer) {
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_LOGOUT_REQ;
	memcpy(buffer+NTY_PROTO_DEVID_IDX, &id, sizeof(C_DEVID));

	//*rLen = NTY_PROTO_MIN_LENGTH;
	U32 u32Crc = ntyGenCrcValue(buffer, NTY_PROTO_MIN_LENGTH-4);
	memcpy(buffer+NTY_PROTO_MIN_LENGTH-4, &u32Crc, sizeof(U32));
	
}


int ntyIterFriendsMessage(void *self, void *arg) {
	C_DEVID *pId = self;
	InterMsg *msg = arg;

	Client *client = msg->self;
	void *map = ntyMapInstance();
	ClientSocket *nSocket = (ClientSocket *)ntyMapSearch(map, client->devId);
	if (nSocket == NULL)  return  NTY_RESULT_NOEXIST;

	return ntyProxyBuffer(nSocket, msg->buffer, msg->length);
}

/* ** **** ******** **************** Natty V3.6 **************** ******** **** ** */
int ntySendDataResult(C_DEVID fromId, U8 *json, int length, U16 status) {
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	U16 bLength = length;
	U16 bStatus = (U16)status;
	

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_PUSH;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_DATA_RESULT;

	memcpy(&buffer[NTY_PROTO_DATA_RESULT_STATUS_IDX] , &bStatus, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_DATA_RESULT_JSON_LENGTH_IDX] , &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_DATA_RESULT_JSON_CONTENT_IDX], json, length);

	bLength = NTY_PROTO_DATA_RESULT_JSON_CONTENT_IDX + length + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);
	return ntySendBuffer(client, buffer, bLength);
	
}

int ntySendMonitorSleepDataResult(C_DEVID fromId, U8 *json, int length, U16 status) {
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	U16 bLength = length;
	U16 bStatus = (U16)status;
	

	buffer[NTY_PROTO_VERSION_IDX] = 0x45;
	buffer[NTY_PROTO_DEVTYPE_IDX] = 0x44;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = 0x04;
	buffer[NTY_PROTO_MSGTYPE_IDX] = 0xD2;

	memcpy(&buffer[NTY_PROTO_DATA_RESULT_STATUS_IDX] , &bStatus, sizeof(U16));
	memcpy(&buffer[8] , &bLength, sizeof(U16));
	memcpy(&buffer[10], json, length);

	bLength = NTY_PROTO_DATA_RESULT_JSON_CONTENT_IDX + length + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);
	return ntySendBuffer(client, buffer, bLength);
	
}


int ntySendUserDataAck(C_DEVID fromId, U8 *json, int length) {
	ntylog( "ntySendUserDataAck send to fromId:%lld,json:%s\n", fromId, json );
#if 0
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
#else
	U8 buffer[NTY_DATA_PACKET_LENGTH*2] = {0};
#endif

	U16 bLength = length;

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_ACK;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_DATAPACKET_ACK;

	memcpy(&buffer[NTY_PROTO_USERDATA_PACKET_ACK_JSON_LENGTH_IDX] , &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_USERDATA_PACKET_ACK_JSON_CONTENT_IDX], json, length);

	bLength = NTY_PROTO_USERDATA_PACKET_ACK_JSON_CONTENT_IDX + length + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);
	
	return ntySendBuffer(client, buffer, bLength);
	
}

//send to ios,do not judge the deviceType
int ntySendPushNotifyIos(C_DEVID selfId, C_DEVID gId, U8 *msg, U32 type){ 
#if 0
	#if ENABLE_RBTREE_REPLACE_BPTREE
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (ClientSocket *)ntyMapSearch( map, selfId );
	if( pClient == NULL ){
		return NTY_RESULT_NOEXIST;
	}
	#else
		void *heap = ntyBHeapInstance();
		NRecord *record = ntyBHeapSelect(heap, selfId);
		if (record == NULL) return NTY_RESULT_NOEXIST;
		Client *pClient = (Client *)record->value;
	#endif
		if (pClient->deviceType == NTY_PROTO_CLIENT_IOS) {
			if (pClient->token != NULL) {
				ntylog("ntySendPushNotify --> selfId:%lld  token:%s\n", selfId, pClient->token);
				void *pushHandle = ntyPushHandleInstance();
				return ntyPushNotifyHandle(pushHandle, msg, pClient->token, 0);
			}
		} else if (pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
			if (pClient->token != NULL) {
				ntylog("ntySendPushNotify Publish --> selfId:%lld  token:%s\n", selfId, pClient->token);
				void *pushHandle = ntyPushHandleInstance();
				return ntyPushNotifyHandle(pushHandle, msg, pClient->token, 1);
			}
		}
		
		return NTY_RESULT_FAILED;//NTY_RESULT_FAILED
#else
			
		VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
		if (tag == NULL) return NTY_RESULT_ERROR;
		memset( tag, 0, sizeof(VALUE_TYPE) );
		if ( msg != NULL ){			
			tag->length = strlen(msg);
			tag->Tag = malloc(tag->length+1);
			memset(tag->Tag, 0, tag->length+1);
			memcpy(tag->Tag, msg, tag->length);	
		}else{
			tag->length = 0;
			tag->Tag = NULL;
		}
		tag->gId = gId;
		tag->arg = type;
		tag->toId = selfId;
		tag->Type = MSG_TYPE_IOS_PUSH_HANDLE;
		ntylog("ntySendPushNotifyIos send to ios app --> begin");
		tag->cb = ntyIOSPushHandle;
		int nRet = ntyDaveMqPushMessage(tag);
		if ( nRet < 0 ){
			return NTY_RESULT_FAILED;
		}
		return NTY_RESULT_SUCCESS;
		
#endif

}

int ntySendPushNotify(C_DEVID selfId, C_DEVID gId, U8 *msg, U32 type) {
#if 0
	#if ENABLE_RBTREE_REPLACE_BPTREE
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (ClientSocket *)ntyMapSearch( map, selfId );
	if( pClient == NULL ){
		return NTY_RESULT_NOEXIST;
	}
	#else
	void *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, selfId);
	if (record == NULL) return NTY_RESULT_NOEXIST;
	Client *pClient = (Client *)record->value;
	#endif
	
	if (pClient->deviceType == NTY_PROTO_CLIENT_IOS) {
		if (pClient->token != NULL) {
			ntylog("ntySendPushNotify --> selfId:%lld  token:%s\n", selfId, pClient->token);
			void *pushHandle = ntyPushHandleInstance();
			return ntyPushNotifyHandle(pushHandle, msg, pClient->token, 0);
		}
	} else if (pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
		if (pClient->token != NULL) {
			ntylog("ntySendPushNotify Publish --> selfId:%lld  token:%s\n", selfId, pClient->token);
			void *pushHandle = ntyPushHandleInstance();
			return ntyPushNotifyHandle(pushHandle, msg, pClient->token, 1);
		}
	}
	
	return NTY_RESULT_FAILED;//NTY_RESULT_FAILED
#else
	#if ENABLE_RBTREE_REPLACE_BPTREE
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (Client *)ntyMapSearch( map, selfId );
	if( pClient == NULL ){
		ntylog( "ntySendPushNotify rbtree not exit:%lld\n",selfId );
		return NTY_RESULT_NOEXIST;
	}
	#else
	void *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, selfId);
	if (record == NULL) return NTY_RESULT_NOEXIST;
	Client *pClient = (Client *)record->value;
	#endif
	
	if ( pClient->deviceType != NTY_PROTO_CLIENT_IOS && pClient->deviceType != NTY_PROTO_CLIENT_IOS_PUBLISH ) {	
		return NTY_RESULT_FAILED;
	}
	VALUE_TYPE *tag = malloc(sizeof(VALUE_TYPE));
	if ( tag == NULL ) return NTY_RESULT_ERROR;
	memset( tag, 0, sizeof(VALUE_TYPE) );
	ntylog("ntySendPushNotify --> begin");
	if ( msg != NULL ) {	
		tag->length = strlen(msg);
		tag->Tag = malloc(tag->length+1);
		memset(tag->Tag, 0, tag->length+1);
		memcpy(tag->Tag, msg, tag->length);	
	} else {
		tag->length = 0;
		tag->Tag = NULL;
	}
	tag->gId = gId;
	tag->arg = type;
	tag->toId = selfId;
	tag->Type = MSG_TYPE_IOS_PUSH_HANDLE;
	tag->cb = ntyIOSPushHandle;
	ntyDaveMqPushMessage(tag);
	return NTY_RESULT_SUCCESS;
	
#endif
}

int ntyClassifyCategoryPushNotify(C_DEVID toId, C_DEVID gId, U8 *json, int length) {
	if (json == NULL) return NTY_RESULT_ERROR;
	
	int category = ntyCommonJsonCategory(json, length);
	ntylog("ntyClassifyCategoryPushNotify --> category:%d, json:%s,toId:%lld,gId:%lld\n", category, json,toId,gId);
	
	if (category == NTY_CATEGORY_SOSREPORT) {
		ntylog(" ntyClassifyCategoryPushNotify SOS report\n");
		return ntySendPushNotify(toId, gId, NTY_PUSH_SOSREPORT_MSG_CONTEXT, category);
	}else if (category == NTY_CATEGORY_EFENCEREPORT) {
		ntylog(" ntyClassifyCategoryPushNotify EFENCE report\n");
		return ntySendPushNotify(toId, gId, NTY_PUSH_EFENCEREPORT_MSG_CONTEXT, category);
	}else if (category == NTY_CATEGORY_BLOODREPORT) {
		ntylog(" ntyClassifyCategoryPushNotify BLOOD report\n");
		//return ntySendPushNotify(toId, gId, NTY_PUSH_BLOOD_MSG_CONTEXT, category);
	}else if (category == NTY_CATEGORY_HEARTREPORT) {
		ntylog(" ntyClassifyCategoryPushNotify HEART report\n");
		//return ntySendPushNotify(toId, gId, NTY_PUSH_HEART_MSG_CONTEXT, category);
	}else if (category == NTY_CATEGORY_WEARSTATUS) {
		ntylog(" ntyClassifyCategoryPushNotify WEASTATUS report\n");
		return ntySendPushNotify(toId, gId, NTY_PUSH_WEARSTATUS_MSG_CONTEXT, category);
	}else if (category == NTY_CATEGORY_FALLDOWNREPORT) {
		ntylog(" ntyClassifyCategoryPushNotify FALLDOWN report\n");
		return ntySendPushNotify(toId, gId, NTY_PUSH_FALLDOWN_MSG_CONTEXT, category);
	}else if (category == NTY_CATEGORY_BINDCONFIRM) {
		ntylog(" ntyClassifyCategoryPushNotify BINDCONFIRM for admin\n");
		return ntySendPushNotify(toId, gId, NTY_PUSH_BINDCONFIRM_MSG_CONTEXT, category);
	}else if (category == NTY_CATEGORY_MATTRESSREPORT) {	//mattress the third part server connect.
		ntylog(" ntyClassifyCategoryPushNotify MATTRESS report\n");
		return ntySendPushNotify(gId, toId, NTY_PUSH_MSG_CONTEXT_MATTRESS, category);
	}else{
		ntylog(" ntyClassifyCategoryPushNotify category not found\n");
	}

	return NTY_RESULT_SUCCESS;
}

int ntySendCommonBroadCastItem(C_DEVID selfId, C_DEVID toId, C_DEVID gId, U8 *json, int length, U32 msgId) {
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	void *map = ntyMapInstance();
	ClientSocket *pClient = (ClientSocket *)ntyMapSearch(map, toId);

#if 0
	if (pClient == NULL) {
		return ntySendPushNotify(toId, NULL);
	}
#else
	ntyClassifyCategoryPushNotify(toId, gId, json, length);	//ios app will be pushed message,others will not be pushed message.
#endif

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_BROADCAST;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_COMMON_BROADCAST;

	memcpy(&buffer[NTY_PROTO_COMMON_BROADCAST_DEVID_IDX], &selfId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_COMMON_BROADCAST_MSGID_IDX], &msgId, sizeof(U32));
	memcpy(&buffer[NTY_PROTO_COMMON_BROADCAST_JSON_LENGTH_IDX], &length, sizeof(U16));
	memcpy(buffer+NTY_PROTO_COMMON_BROADCAST_JSON_CONTENT_IDX, json, length);

	length = length + NTY_PROTO_COMMON_BROADCAST_JSON_CONTENT_IDX + sizeof(U32);

	return ntySendBuffer(pClient, buffer, length);
}


int ntySendCommonBroadCastIter(void *self, void *arg) {
	
	C_DEVID toId = 0, selfId = 0, gId = 0;
	memcpy(&toId, self, sizeof(C_DEVID));

	InterMsg *msg = arg;
	if (msg == NULL) {
		ntylog(" ntySendCommonBroadCastIter --> NTY_RESULT_ERROR\n");
		return NTY_RESULT_ERROR;
	}
	U8 *json = msg->buffer;
	U16 length = (U16)msg->length;
#if 0
	Client *client = msg->group;
#else
	memcpy(&gId, msg->group, sizeof(C_DEVID));
#endif
	U32 index = (U32)msg->arg;
	memcpy(&selfId, msg->self, sizeof(C_DEVID));

	ntylog("ntySendCommonBroadCastIter toId:%lld, selfId:%lld\n", toId, selfId);
	if (toId == selfId) return NTY_RESULT_SUCCESS;
#if 0
	void *map = ntyMapInstance();
	ClientSocket *pClient = (ClientSocket *)ntyMapSearch(map, toId);

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_BROADCAST;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_COMMON_BROADCAST;

	memcpy(&buffer[NTY_PROTO_COMMON_BROADCAST_DEVID_IDX], &selfId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_COMMON_BROADCAST_MSGID_IDX], &index, sizeof(U32));
	memcpy(&buffer[NTY_PROTO_COMMON_BROADCAST_JSON_LENGTH_IDX], &length, sizeof(U16));
	memcpy(buffer+NTY_PROTO_COMMON_BROADCAST_JSON_CONTENT_IDX, json, length);

	length = length + NTY_PROTO_COMMON_BROADCAST_JSON_CONTENT_IDX + sizeof(U32);

	return ntySendBuffer(pClient, buffer, length);
#else

	return ntySendCommonBroadCastItem(selfId, toId, gId, json, length, index);

#endif
}

//gId stand for devid
//selfId AppId
int ntySendCommonBroadCastResult(C_DEVID selfId, C_DEVID gId, U8 *json, int length, int index) {

#if BHEAP_VECTOR_ENABLE
  #if ENABLE_RBTREE_REPLACE_BPTREE
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (Client *)ntyMapSearch( map, gId );
	if ( pClient == NULL ) {
		ntylog("ntySendCommonBroadCastResult rbtree not exit:%lld\n",gId);
		void *group = ntyVectorCreator();
		if ( group == NULL ){ 
			ntylog(" ntySendCommonBroadCastResult vector==NULL\n");
			return NTY_RESULT_FAILED;
		}
		if(-1 == ntyQueryAppIDListSelectHandle(gId, group)) {
			ntylog(" ntySendCommonBroadCastResult ntyQueryWatchIDListSelectHandle Failed \n");
		}	
		InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
		if ( msg == NULL ){ 
			ntylog("ntySendCommonBroadCastResult msg==NULL\n");
			return NTY_RESULT_FAILED;	
		}
		msg->buffer = json;
		msg->length = length;
		msg->group = &gId;
		msg->self = &selfId;
		msg->arg = index;

		ntyVectorIterator(group, ntySendCommonBroadCastIter, msg);
		ntyFree(msg);
		ntyVectorDestory(group);
	} else {
		ntylog("ntySendCommonBroadCastResult rbtree exit:%lld\n",gId);	
		InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
		if ( msg == NULL ){
			ntylog(" ntySendCommonBroadCastResult msg==NULL \n");
			return NTY_RESULT_FAILED;
		}
		msg->buffer = json;
		msg->length = length;
		msg->group = &gId;
		msg->self = &selfId;
		msg->arg = index;		
		if ( pClient->friends == NULL ) {
			ntyFree( msg );
			ntylog(" ntySendCommonBroadCastResult pClient->friends==NULL\n");
			return NTY_RESULT_ERROR;
		}
		ntyVectorIterator( pClient->friends, ntySendCommonBroadCastIter, msg );
		ntyFree( msg );
	}
  #else
	void *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, gId);
	if ( record == NULL ) {
		ntylog(" ntySendCommonBroadCastResult record==NULL \n");
		void *group = ntyVectorCreator();
		if ( group == NULL ){ 
			ntylog(" ntySendCommonBroadCastResult group==NULL \n");
			return NTY_RESULT_FAILED;
		}
		if(-1 == ntyQueryAppIDListSelectHandle(gId, group)) {
			ntylog(" ntyQueryWatchIDListSelectHandle Failed \n");
		}
			
		InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
		if ( msg == NULL ){ 
			ntylog(" ntyQueryWatchIDListSelectHandle msg==NULL \n");
			return NTY_RESULT_FAILED;	
		}
		msg->buffer = json;
		msg->length = length;
		msg->group = &gId;
		msg->self = &selfId;
		msg->arg = index;

		ntyVectorIterator(group, ntySendCommonBroadCastIter, msg);
		ntyFree(msg);
		ntyVectorDestory(group);
	} else {
		ntylog("ntySendCommonBroadCastResult record!=NULL \n");		
		Client *pClient = (Client*)record->value;
		if ( pClient == NULL ){ 
			ntylog("ntySendCommonBroadCastResult pClient==NULL \n");	
			return NTY_RESULT_FAILED;
		}
		InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
		if ( msg == NULL ){
			ntylog(" ntyQueryWatchIDListSelectHandle msg==NULL \n");
			return NTY_RESULT_FAILED;
		}
		
		msg->buffer = json;
		msg->length = length;
		msg->group = &gId;
		msg->self = &selfId;
		msg->arg = index;		
		if ( pClient->friends == NULL ) {
			ntyFree( msg );
			ntylog(" ntySendCommonBroadCastResult pClient->friends==NULL\n");
			return NTY_RESULT_ERROR;
		}
		ntylog(" ntySendCommonBroadCastResult ntyVectorIterator \n");
		ntyVectorIterator( pClient->friends, ntySendCommonBroadCastIter, msg );
		ntyFree( msg );
	}
	#endif
	
	return NTY_RESULT_SUCCESS;
#else

	//void *heap = ntyBHeapInstance();
	//NRecord *record = ntyBHeapSelect(heap, gId);
	//Client *pClient = (Client*)record->value;
	//if (pClient == NULL) return NTY_RESULT_FAILED;
	void *group = ntyVectorCreator();
	if (group == NULL) return NTY_RESULT_FAILED;
	if(-1 == ntyQueryAppIDListSelectHandle(gId, group)) {
		ntylog(" ntyQueryWatchIDListSelectHandle Failed \n");
	}

	InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
	if (msg == NULL) return NTY_RESULT_FAILED;
	msg->buffer = json;
	msg->length = length;
	msg->group = NULL;
	msg->self = &selfId;
	msg->arg = index;
	
	ntylog(" -->> ntySendCommonBroadCastResult --> %s \n", json);
	ntyVectorIterator(group, ntySendCommonBroadCastIter, msg);
	ntyFree(msg);
	ntyVectorDestory(group);
	return NTY_RESULT_SUCCESS;
#endif	
}


//selfId,gId stand for devid
int ntySendCommonBroadCastMonitorSleepResult( C_DEVID selfId, C_DEVID gId, U8 *json, int length, int index ) {
	void *group = ntyVectorCreator();
	if ( group == NULL ){ 
		ntylog( "ntySendCommonBroadCastMonitorSleepResult group==NULL\n" );
		return NTY_RESULT_FAILED;
	}
	if( -1 == ntyQueryAppIDListSelectHandle(gId, group) ) {
		ntylog(" ntySendCommonBroadCastMonitorSleepResult ntyQueryWatchIDListSelectHandle Failed \n");
		return NTY_RESULT_FAILED;
	}

	InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
	if ( msg == NULL ){ 
		ntylog(" ntySendCommonBroadCastMonitorSleepResult InterMsg malloc Failed \n");
		return NTY_RESULT_FAILED;	
	}
	msg->buffer = json;
	msg->length = length;
	msg->group = &gId;
	msg->self = &selfId;
	msg->arg = index;

	ntyVectorIterator( group, ntySendCommonBroadCastIter, msg );
	free( msg );
	ntyVectorDestory( group );
		
	return NTY_RESULT_SUCCESS;
}

//
int ntySendCommonReq(C_DEVID fromId, C_DEVID toId, U8 *json, int length, int msgId) {
	
	
	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_REQ;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_COMMON_REQ;


	memcpy(&buffer[NTY_PROTO_COMMON_REQ_DEVID_IDX], &fromId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_RSVD_IDX], &msgId, sizeof(U32));
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_RECVID_IDX], &toId, sizeof(C_DEVID));
	
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX], json, bLength);

	bLength = bLength + NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX + sizeof(U32);

	ntylog("ntySendCommonReq buffer:%s\n", buffer+NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);

	return ntySendBuffer(client, buffer, bLength);

	
}

int ntySendDataRoute(C_DEVID toId, U8 *buffer, int length) {
	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);

	return ntySendBuffer(client, buffer, length);
}

int ntySendDataMessagePush(C_DEVID toId, U8 *buffer, int length){
	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);

	return ntySendBuffer(client, buffer, length);

}

int ntySendVoiceBroadCastItem(C_DEVID fromId, C_DEVID toId, C_DEVID gId, U8 *json, int length, int index) {
	ntylog( "ntySendVoiceBroadCastItem begin,fromId:%lld,toId:%lld,gId:%lld\n",fromId,toId, gId);
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_BROADCAST;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_VOICE_BROADCAST;	
	memcpy(&buffer[NTY_PROTO_VOICE_BROADCAST_DEVID_IDX], &fromId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_VOICE_BROADCAST_MSGID_IDX], &index, sizeof(U32));
	memcpy(&buffer[NTY_PROTO_VOICE_BROADCAST_JSON_LENGTH_IDX], &length, sizeof(U16));
	memcpy(buffer+NTY_PROTO_VOICE_BROADCAST_JSON_CONTENT_IDX, json, length);
	
	length = length + NTY_PROTO_VOICE_BROADCAST_JSON_CONTENT_IDX + sizeof(U32);
	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);
#if 0
	if (client == NULL) {
		return ntySendPushNotify(toId, NULL);
	}
#else
	ntySendPushNotify(toId, gId, NTY_PUSH_VOICE_MSG_CONTEXT, NTY_CATEGORY_VOICE);
/*
	//push to ios app,then do not send to ios app from SDK.
	void *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect( heap, toId );
	if ( record == NULL ) { 
		ntylog("ntySendVoiceBroadCastItem Select FromId : %lld is No Exist \n", toId);
		return NTY_RESULT_FAILED;
	}
	
	Client *pClient = (Client*)record->value;
	if ( pClient == NULL ){ 
		ntylog("ntySendVoiceBroadCastItem pClient==NULL\n");
		return NTY_RESULT_FAILED;
	}

	if ( pClient->deviceType==NTY_PROTO_CLIENT_IOS || pClient->deviceType==NTY_PROTO_CLIENT_IOS_PUBLISH ){
		ntylog("ntySendVoiceBroadCastItem push to ios app,then do not send to ios app from SDK.\n");
		return NTY_RESULT_SUCCESS;
	}
*/
#endif
	usleep( 100000 );
	int ret = ntySendBuffer(client, buffer, length);
	return ret;
}

int ntySendVoiceBroadCast(C_DEVID fromId, C_DEVID toId, C_DEVID gId, U8 *json, int length, int index) {
	ntylog( "ntySendVoiceBroadCast begin,fromId:%lld,toId:%lld,gId:%lld\n",fromId,toId, gId);
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_BROADCAST;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_VOICE_BROADCAST;	
	memcpy(&buffer[NTY_PROTO_VOICE_BROADCAST_DEVID_IDX], &fromId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_VOICE_BROADCAST_MSGID_IDX], &index, sizeof(U32));
	memcpy(&buffer[NTY_PROTO_VOICE_BROADCAST_JSON_LENGTH_IDX], &length, sizeof(U16));
	memcpy(buffer+NTY_PROTO_VOICE_BROADCAST_JSON_CONTENT_IDX, json, length);
	
	length = length + NTY_PROTO_VOICE_BROADCAST_JSON_CONTENT_IDX + sizeof(U32);
	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);

	return ntySendBuffer(client, buffer, length);
}


int ntySendVoiceBroadCastIter(void *self, void *arg) {
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	C_DEVID toId = 0, selfId = 0, gId = 0;
	memcpy(&toId, self, sizeof(C_DEVID));

	InterMsg *msg = arg;
	U8 *json = msg->buffer;
	U16 length = (U16)msg->length;
#if 0
	Client *pClient = msg->group;
#else
	memcpy(&gId, msg->group, sizeof(C_DEVID));
#endif
	U32 index = (U32)msg->arg;
	memcpy(&selfId, msg->self, sizeof(C_DEVID));

	ntylog(" ntySendVoiceBroadCastIter --> toId:%lld, selfId:%lld\n", toId, selfId);
	if (toId == selfId){ 
		ntylog( "ntySendVoiceBroadCastIter toId=slefId do not broadcast\n" );	
		return 0;
	}
#if 0
	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_BROADCAST;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_VOICE_BROADCAST;
	memcpy(&buffer[NTY_PROTO_VOICE_BROADCAST_DEVID_IDX], &selfId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_VOICE_BROADCAST_MSGID_IDX], &index, sizeof(U32));
	memcpy(&buffer[NTY_PROTO_VOICE_BROADCAST_JSON_LENGTH_IDX], &length, sizeof(U16));
	memcpy(buffer+NTY_PROTO_VOICE_BROADCAST_JSON_CONTENT_IDX, json, length);

	length = length + NTY_PROTO_VOICE_BROADCAST_JSON_CONTENT_IDX + sizeof(U32);
	int nRet = ntySendBuffer(client, buffer, length);
	ntylog( "ntySendVoiceBroadCastIter ntySendBuffer after\n" );
	return nRet;
#else
	int nRet = ntySendVoiceBroadCastItem(selfId, toId, gId, json, length, index);
	ntylog( "ntySendVoiceBroadCastIter ntySendVoiceBroadCastItem after\n" );
	usleep( 100000 );
	
	return nRet;

#endif
}


int ntySendVoiceBroadCastResult(C_DEVID fromId, C_DEVID gId, U8 *json, int length, int index) {

#if BHEAP_VECTOR_ENABLE

  #if ENABLE_RBTREE_REPLACE_BPTREE
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (Client *)ntyMapSearch( map, gId );
	if ( pClient == NULL ) {
		ntylog("ntySendVoiceBroadCastResult rbtree not exit:%lld\n",gId);
		void *group = ntyVectorCreator();
		if (group == NULL) return NTY_RESULT_FAILED;
		if( -1 == ntyQueryAppIDListSelectHandle(gId, group) ) {	//find all the appids binded gId  
			ntylog("ntySendVoiceBroadCastResult ntyQueryWatchIDListSelectHandle Failed \n");
			return NTY_RESULT_FAILED;
		}			
		InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
		if (msg == NULL) return NTY_RESULT_FAILED;
		
		msg->buffer = json;
		msg->length = length;
		msg->group = &gId;
		msg->self = &fromId;
		msg->arg = index;
		ntyVectorIterator(group, ntySendVoiceBroadCastIter, msg);  //broadcast to all the appids binded the deviceid
		ntyFree(msg);
		ntyVectorDestory(group);

		//when fromId is app,then send to the device.	
		Client *pClientPtr = (Client *)ntyMapSearch( map, fromId );
		if ( pClientPtr == NULL ){ 
			ntylog("ntySendVoiceBroadCastResult fromId not exit:%lld\n",fromId);
			return NTY_RESULT_FAILED;
		}
		if (pClientPtr->deviceType == NTY_PROTO_CLIENT_ANDROID 
			|| pClientPtr->deviceType == NTY_PROTO_CLIENT_IOS
			|| pClientPtr->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {	
			ntylog( "ntySendVoiceBroadCastResult fromId:%lld is AppId,send to the device.\n",fromId );
			ntySendVoiceBroadCast(fromId, gId, gId, json, length, index);
		}	

	}else {
		ntylog("ntySendVoiceBroadCastResult rbtree exit:%lld\n",gId);
		InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
		msg->buffer = json;
		msg->length = length;
		msg->group = &gId;
		msg->self = &fromId;
		msg->arg = index;
		if ( pClient->friends == NULL ) {
			ntylog( "ntySendVoiceBroadCastResult pClient->friends==NULL\n" );
			ntyFree(msg);
			return NTY_RESULT_ERROR;
		}
		ntyVectorIterator(pClient->friends, ntySendVoiceBroadCastIter, msg);	//broadcast to all the appids binded the deviceid
		ntyFree(msg);

		//when fromId is app,then send to the device.	
		Client *pClientPtr = (Client *)ntyMapSearch( map, fromId );
		if ( pClientPtr == NULL ){ 
			ntylog("ntySendVoiceBroadCastResult fromId not exit:%lld\n",fromId);
			return NTY_RESULT_FAILED;
		}
	
		if (pClientPtr->deviceType == NTY_PROTO_CLIENT_ANDROID 
			|| pClientPtr->deviceType == NTY_PROTO_CLIENT_IOS
			|| pClientPtr->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
			ntylog( "ntySendVoiceBroadCastResult fromId:%lld is AppId,send to the device.\n",fromId );
			ntySendVoiceBroadCast(fromId, gId, gId, json, length, index);
		}
	}

  #else
	void *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, gId);
	if ( record == NULL	) {
		void *group = ntyVectorCreator();
		if (group == NULL) return NTY_RESULT_FAILED;
		if( -1 == ntyQueryAppIDListSelectHandle(gId, group) ) {	//find all the appids binded gId  
			ntylog("ntySendVoiceBroadCastResult ntyQueryWatchIDListSelectHandle Failed \n");
			return NTY_RESULT_FAILED;
		}			
		InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
		if (msg == NULL) return NTY_RESULT_FAILED;
		
		msg->buffer = json;
		msg->length = length;
		msg->group = &gId;
		msg->self = &fromId;
		msg->arg = index;
		ntylog(" ntySendCommonBroadCastResult record==NULL ntyVectorIterator\n");
		ntyVectorIterator(group, ntySendVoiceBroadCastIter, msg);  //broacast to all the appids binded the deviceid
		ntyFree(msg);
		ntyVectorDestory(group);

		//when fromId is app,then send to the device.	
		NRecord *recordPtr = ntyBHeapSelect(heap, fromId);
		if ( recordPtr == NULL ) { 
			ntylog("ntySendVoiceBroadCastResult gId record==NULL,Select FromId:%lld is Not Exist \n", fromId);
			return NTY_RESULT_FAILED;
		}
		Client *pClientPtr = (Client*)recordPtr->value;
		if ( pClientPtr == NULL ){ 
			ntylog("ntySendVoiceBroadCastResult gId record==NULL pClient==NULL\n");
			return NTY_RESULT_FAILED;
		}
		if (pClientPtr->deviceType == NTY_PROTO_CLIENT_ANDROID 
			|| pClientPtr->deviceType == NTY_PROTO_CLIENT_IOS
			|| pClientPtr->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {	
			ntylog( "ntySendVoiceBroadCastResult fromId is AppId,then send to the device.\n" );
			ntySendVoiceBroadCast(fromId, gId, gId, json, length, index);
		}	

	}else {
		Client *pClient = (Client*)record->value;
		if (pClient == NULL){
			ntylog( "ntySendVoiceBroadCastResult record!=NULL pClient==NULL\n" );
			return NTY_RESULT_NOEXIST;
		}
		InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
		msg->buffer = json;
		msg->length = length;
		msg->group = &gId;
		msg->self = &fromId;
		msg->arg = index;
		if ( pClient->friends == NULL ) {
			ntylog( "ntySendVoiceBroadCastResult record==NULL pClient->friends==NULL\n" );
			ntyFree(msg);
			return NTY_RESULT_ERROR;
		}

		ntylog(" ntySendCommonBroadCastResult record!=NULL ntyVectorIterator\n");
		ntyVectorIterator(pClient->friends, ntySendVoiceBroadCastIter, msg);	//broacast to all the appids binded the deviceid
		ntyFree(msg);

		//when fromId is app,then send to the device.	
		NRecord *recordPtr = ntyBHeapSelect(heap, fromId);
		if ( recordPtr == NULL ) { 
			ntylog("ntySendVoiceBroadCastResult gId record==NULL, Select FromId:%lld is not Exist \n", fromId);
			return NTY_RESULT_FAILED;
		}

		Client *pClientPtr = (Client*)recordPtr->value;
		if ( pClientPtr == NULL ){ 
			ntylog("ntySendVoiceBroadCastResult gId record==NULL pClientPtr==NULL\n");
			return NTY_RESULT_FAILED;
		}
	
		if (pClientPtr->deviceType == NTY_PROTO_CLIENT_ANDROID 
			|| pClientPtr->deviceType == NTY_PROTO_CLIENT_IOS
			|| pClientPtr->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
			ntylog( "ntySendVoiceBroadCastResult fromId is AppId,then send to the device.\n" );
			ntySendVoiceBroadCast(fromId, gId, gId, json, length, index);
		}
	}
	#endif
#else
	//get fromId group all
	//fromId type
  #if ENABLE_RBTREE_REPLACE_BPTREE
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (ClientSocket *)ntyMapSearch( map, fromId );
	if ( pClient == NULL ){ 
		ntylog("ntySendVoiceBroadCastResult fromId not exit:%lld\n",fromId);
		return NTY_RESULT_FAILED;
	}	
  #else
	void *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, fromId);
	Client *pClient = (Client*)record->value;
	if (pClient == NULL) return NTY_RESULT_FAILED;
  #endif
	
	void *group = NULL;
	if (pClient->deviceType == NTY_PROTO_CLIENT_ANDROID 
		|| pClient->deviceType == NTY_PROTO_CLIENT_IOS
		|| pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
		group = ntyVectorCreator();

		if(-1 == ntyQueryAppIDListSelectHandle(gId, group)) {
			ntylog(" ntyQueryWatchIDListSelectHandle Failed \n");
		}
	} else if (pClient->deviceType == NTY_PROTO_CLIENT_WATCH) {
		group = pClient->friends;
	} else {
		ntylog(" Protocol Device Type is Error : %c\n", pClient->deviceType);
	}
	InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
	if (msg == NULL) return NTY_RESULT_ERROR;
	msg->buffer = json;
	msg->length = length;
	msg->group = pClient;
	msg->self = &fromId;
	msg->arg = index;

	if (group == NULL){ 
		free(msg);
		return NTY_RESULT_ERROR;
	}
	ntylog(" ntySendVoiceBroadCastResult --> ntyVectorIterator\n");
	ntyVectorIterator(group, ntySendVoiceBroadCastIter, msg);
	free(msg);
	ntyVectorDestory(group);

	//if fromId is AppId, need to send gId self
	//else don't do that
	if (pClient->deviceType == NTY_PROTO_CLIENT_ANDROID 
		|| pClient->deviceType == NTY_PROTO_CLIENT_IOS
		|| pClient->deviceType == NTY_PROTO_CLIENT_IOS_PUBLISH) {
		ntySendVoiceBroadCastItem(fromId, gId, gId, json, length, index);
	}
#endif

	return NTY_RESULT_SUCCESS;
	
}


int ntySendLocationBroadCastIter(void *self, void *arg) {
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	C_DEVID toId = 0, selfId = 0;
	memcpy(&toId, self, sizeof(C_DEVID));

	InterMsg *msg = arg;
	U8 *json = msg->buffer;
	U16 length = (U16)msg->length;
	Client *pClient = msg->group;
	memcpy(&selfId, msg->self, sizeof(C_DEVID));

	ntylog(" toId:%lld, selfId:%lld\n", toId, selfId);
	if (toId == selfId) return 0;

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_BROADCAST;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_LOCATION_BROADCAST;

	memcpy(&buffer[NTY_PROTO_LOCATION_BROADCAST_DEVID_IDX], &selfId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_LOCATION_BROADCAST_JSON_LENGTH_IDX], &length, sizeof(U16));
	memcpy(buffer+NTY_PROTO_LOCATION_BROADCAST_JSON_CONTENT_IDX, json, length);

	length = length + NTY_PROTO_VOICE_BROADCAST_JSON_CONTENT_IDX + sizeof(U32);

	return ntySendBuffer(client, buffer, length);
}


int ntySendLocationBroadCastResult(C_DEVID fromId, C_DEVID gId, U8 *json, int length) {
  #if ENABLE_RBTREE_REPLACE_BPTREE
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (Client *)ntyMapSearch( map, gId );
	if ( pClient == NULL ){ 
		ntylog( "ntySendLocationBroadCastResult rbtree not exit:%lld\n", gId);
		return NTY_RESULT_NOEXIST;
	}
  #else
	void *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, gId);
	if (record == NULL) return NTY_RESULT_NOEXIST;
	
	Client *pClient = (Client*)record->value;
	if (pClient == NULL) return NTY_RESULT_NOEXIST;
  #endif
	
	InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
	if (msg == NULL) {
		return NTY_RESULT_ERROR;
	}
	memset(msg, 0, sizeof(InterMsg));
	msg->buffer = json;
	msg->length = length;
	msg->group = pClient;
	msg->self = &fromId;

	if (pClient->friends == NULL) {
		free(msg);
		return NTY_RESULT_ERROR;
	}
  #if 1
	//ntylog("ntySendLocationBroadCastResult --> friend addr:%llx\n", (C_DEVID)pClient->friends);
	ntylog(" ntySendLocationBroadCastResult --> fromId:%lld, devId:%lld \n", fromId, pClient->devId);
	ntyVectorIterator(pClient->friends, ntySendLocationBroadCastIter, msg);
  #else
	ntyVectorIter(pClient->friends, ntySendLocationBroadCastIter, msg);
  #endif
	free(msg);
	return NTY_RESULT_SUCCESS;
}



int ntySendLoginAckResult(C_DEVID fromId, U8 *json, int length, U16 status) {
	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);


	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_ACK;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_LOGIN_ACK;

	memcpy(&buffer[NTY_PROTO_LOGIN_ACK_STATUS_IDX], &status, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_LOGIN_ACK_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_LOGIN_ACK_JSON_CONTENT_IDX], json, bLength);

	bLength = bLength + NTY_PROTO_LOGIN_ACK_JSON_CONTENT_IDX + sizeof(U32);

	return ntySendBuffer(client, buffer, bLength);
	
}

int ntySendOfflineMsgReqResult(C_DEVID fromId, U8 *json, int length) {
	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	
	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_REQ;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_OFFLINE_MSG_REQ;

	memcpy(&buffer[NTY_PROTO_OFFLINE_MSG_REQ_DEVICEID_IDX], &fromId, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_OFFLINE_MSG_REQ_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_OFFLINE_MSG_REQ_JSON_CONTENT_IDX], json, bLength);

	bLength = bLength + NTY_PROTO_OFFLINE_MSG_REQ_JSON_CONTENT_IDX + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);
	
	return ntySendBuffer(client, buffer, bLength);
}


int ntySendOfflineMsgAckResult(C_DEVID fromId, U8 *json, int length, U16 status) {
	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	
	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_ACK;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_OFFLINE_MSG_ACK;

	memcpy(&buffer[NTY_PROTO_OFFLINE_MSG_ACK_STATUS_IDX], &status, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_OFFLINE_MSG_ACK_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_OFFLINE_MSG_ACK_JSON_CONTENT_IDX], json, bLength);

	bLength = bLength + NTY_PROTO_OFFLINE_MSG_ACK_JSON_CONTENT_IDX + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);
	
	return ntySendBuffer(client, buffer, bLength);
}


int ntySendLocationPushResult(C_DEVID fromId, U8 *json, int length) {
	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	
	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_PUSH;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_LOCATION_PUSH;

	memcpy(&buffer[NTY_PROTO_LOCATION_PUSH_DEVID_IDX], &fromId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_LOCATION_PUSH_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_LOCATION_PUSH_JSON_CONTENT_IDX], json, bLength);

	bLength = bLength + NTY_PROTO_LOCATION_PUSH_JSON_CONTENT_IDX + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);
	
	return ntySendBuffer(client, buffer, bLength);
}

int ntySendWeatherPushResult(C_DEVID fromId, U8 *json, int length) {
	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	
	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_PUSH;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_WEATHER_PUSH;

	memcpy(&buffer[NTY_PROTO_WEATHER_PUSH_DEVID_IDX], &fromId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_WEATHER_PUSH_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_WEATHER_PUSH_JSON_CONTENT_IDX], json, bLength);

	bLength = bLength + NTY_PROTO_WEATHER_PUSH_JSON_CONTENT_IDX + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);
	
	return ntySendBuffer(client, buffer, bLength);
}

int ntySendHeartBeatResult(C_DEVID fromId) {
	int length = 0;
	U8 buffer[NTY_HEARTBEAT_ACK_LENGTH] = {0};
	
	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_ACK;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_HEARTBEAT_ACK;

	buffer[NTY_PROTO_HEARTBEAT_ACK_STATUS_IDX] = 200;

	length = NTY_PROTO_HEARTBEAT_ACK_CRC_IDX + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);
	
	return ntySendBuffer(client, buffer, length);
}


int ntySendICCIDAckResult(C_DEVID fromId, U8 *json, int length, U16 status) {
	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};

	
	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_ACK;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_ICCID_ACK;

	*(U16*)(buffer+NTY_PROTO_ICCID_ACK_STATUS_IDX) = status;
	memcpy(&buffer[NTY_PROTO_ICCID_ACK_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_ICCID_ACK_JSON_CONTENT_IDX], json, bLength);

	ntylog("\n ntySendICCIDAckResult:%s, length:%d\n", json, length);
	ntylog(" NTY_PROTO_ICCID_ACK_JSON_CONTENT_IDX:%s, length:%d\n", buffer+NTY_PROTO_ICCID_ACK_JSON_CONTENT_IDX, length);
	bLength = bLength + NTY_PROTO_ICCID_ACK_JSON_CONTENT_IDX + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, fromId);
	
	return ntySendBuffer(client, buffer, bLength);
}



/*
 * json add to common
 * 
 */
int ntySendRecodeJsonPacket(C_DEVID fromId, C_DEVID toId, U8 *json, int length) {
#if 0
	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_REQ;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_COMMON_REQ;

	//insert db
	int msgId;
	int ret = ntyExecuteCommonItemMsgInsertHandle(fromId, toId, json, &msgId);
	if (ret != NTY_RESULT_SUCCESS) {
		return ret;
	}

	memcpy(&buffer[NTY_PROTO_COMMON_REQ_DEVID_IDX], &fromId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_RSVD_IDX], &msgId, sizeof(U32));
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_RECVID_IDX], &toId, sizeof(C_DEVID));
	
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX], json, bLength);

	bLength = bLength + NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX + sizeof(U32);

	ntylog("ntySendRecodeJsonPacket buffer:%s\n", buffer+NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);
#if 0
	return ntySendBuffer(client, buffer, bLength);
#else

	ret = ntySendBuffer(client, buffer, bLength);
	if (ret < NTY_RESULT_SUCCESS) {
		ntyExecuteCommonItemMsgDeleteHandle(msgId);
	}

#endif

#else

	int msgId = 0;
	int ret = ntyExecuteCommonItemMsgInsertHandle(fromId, toId, json, &msgId);
	if (ret != NTY_RESULT_SUCCESS) {
		ntylog("ntyExecuteCommonItemMsgInsertHandle --> failed, ret : %d\n", ret);
		return ret;
	}

	ret = ntySendCommonReq(fromId, toId, json, length, msgId);
	if (ret < NTY_RESULT_SUCCESS) {
		ntylog("ntyExecuteCommonItemMsgInsertHandle ntySendCommonReq ret:%d\n", ret);
		ntyExecuteCommonItemMsgDeleteHandle(msgId);
	}

#endif

	return ret;
}
/*************************************************************************************************
** Function: ntySendRecodeJsonPacketVersionB 
** Description: insert to the tables of TB_COMMON_MSG and TB_COMMON_OFFLINE_MSG with request json.
**              Get the MesgId from the table,then,send to the device. Don't care about the result.
** Input: C_DEVID fromId, C_DEVID toId, U8 *json, int length.
** Output: 
** Return: int >=0:success, <0:error
** Author:luoyb
** Date: 2017-06-08
** Others:
*************************************************************************************************/

int ntySendRecodeJsonPacketVersionB(C_DEVID fromId, C_DEVID toId, U8 *json, int length) {
#if 0
	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};

	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_REQ;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_COMMON_REQ;

	//insert db
	int msgId;
	int ret = ntyExecuteCommonItemMsgInsertHandle(fromId, toId, json, &msgId);
	if (ret != NTY_RESULT_SUCCESS) {
		return ret;
	}

	memcpy(&buffer[NTY_PROTO_COMMON_REQ_DEVID_IDX], &fromId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_RSVD_IDX], &msgId, sizeof(U32));
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_RECVID_IDX], &toId, sizeof(C_DEVID));
	
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX], json, bLength);

	bLength = bLength + NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX + sizeof(U32);

	ntylog("ntySendRecodeJsonPacket buffer:%s\n", buffer+NTY_PROTO_COMMON_REQ_JSON_CONTENT_IDX);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);
#if 0
	return ntySendBuffer(client, buffer, bLength);
#else

	ret = ntySendBuffer(client, buffer, bLength);
	if (ret < NTY_RESULT_SUCCESS) {
		ntyExecuteCommonItemMsgDeleteHandle(msgId);
	}

#endif

#else

	int msgId = 0;
	int ret = ntyExecuteCommonItemMsgInsertHandle(fromId, toId, json, &msgId);
	if (ret != NTY_RESULT_SUCCESS) {
		ntylog("ntyExecuteCommonItemMsgInsertHandle --> failed, ret : %d\n", ret);
		return ret;
	}

	ret = ntySendCommonReq(fromId, toId, json, length, msgId);
	//Don't care about the result for Reset and Restore commonds.
	//if (ret < NTY_RESULT_SUCCESS) {
	//	ntyExecuteCommonItemMsgDeleteHandle(msgId);
	//}

#endif

	return ret;
}


#if 1


int ntyBigPacketEncode(U8 *pBuffer, int length) {
	int i = 0, j = 0, k = 0, idx = 0;
	int pktCount = length / NTY_VOICEREQ_DATA_LENGTH;
	int pktLength = pktCount * NTY_VOICEREQ_PACKET_LENGTH + (length % NTY_VOICEREQ_DATA_LENGTH) + NTY_VOICEREQ_EXTEND_LENGTH;
	//U8 *pktIndex = pBuffer + pktCount * NTY_VOICEREQ_PACKET_LENGTH;

	ntylog("ntyBigPacketEncode pktLength :%d, pktCount:%d\n", pktLength, pktCount);
	if (pktCount >= (NTY_VOICEREQ_COUNT_LENGTH-1)) return -1;

	j = pktLength - NTY_CRCNUM_LENGTH;
	k = length;

	ntylog("ntyBigPacketEncode j:%d, k:%d, pktCount:%d, last:%d\n", j, k, pktCount, (length % NTY_VOICEREQ_DATA_LENGTH));
	for (idx = 0;idx < (length % NTY_VOICEREQ_DATA_LENGTH);idx ++) {
		pBuffer[--j] = pBuffer[--k];
	}	
			
	for (i = pktCount;i > 0;i --) {
		j = i * NTY_VOICEREQ_PACKET_LENGTH - NTY_CRCNUM_LENGTH;
		k = i * NTY_VOICEREQ_DATA_LENGTH;
		ntylog("ntyBigPacketEncode j:%d, k:%d\n", j, k);
		for (idx = 0;idx < NTY_VOICEREQ_DATA_LENGTH;idx ++) {
			pBuffer[--j] = pBuffer[--k];
		}
	}

	return pktLength;
}

int ntySendBigPacket(U8 *buffer, int length, C_DEVID fromId, C_DEVID gId, C_DEVID toId, U32 index) {
	U16 Count = length / NTY_VOICEREQ_PACKET_LENGTH + 1 ;
	U32 pktLength = NTY_VOICEREQ_DATA_LENGTH, i;
	U8 *pkt = buffer;
	int ret = -1;
	ntylog("ntySendBigPacket destId:%d, index:%d, length:%d", NTY_PROTO_VOICE_DATA_REQ_GROUP_IDX, index, length);
	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, toId);

	for ( i = 0;i < Count;i ++ ) {
		pkt = buffer+(i*NTY_VOICEREQ_PACKET_LENGTH);
		pkt[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
		pkt[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_ANDROID;
		pkt[NTY_PROTO_PROTOTYPE_IDX] = (U8) MSG_REQ;	
		pkt[NTY_PROTO_VOICEREQ_TYPE_IDX] = NTY_PROTO_VOICE_DATA_REQ;
		memcpy(pkt+NTY_PROTO_VOICE_DATA_REQ_DEVID_IDX, &fromId, sizeof(C_DEVID));
		memcpy(pkt+NTY_PROTO_VOICE_DATA_REQ_GROUP_IDX, &gId, sizeof(C_DEVID));
		memcpy(pkt+NTY_PROTO_VOICE_DATA_REQ_PKTINDEX_IDX, &i, sizeof(U16));
		memcpy(pkt+NTY_PROTO_VOICE_DATA_REQ_PKTTOTLE_IDX, &Count , sizeof(U16));
		memcpy(pkt+NTY_PROTO_VOICE_DATA_REQ_OFFLINEMSGID_IDX, &index, sizeof(U32));

		if (i == Count-1) { //last packet
			pktLength = (length % NTY_VOICEREQ_PACKET_LENGTH) - NTY_VOICEREQ_EXTEND_LENGTH;
		}
		memcpy(pkt+NTY_PROTO_VOICE_DATA_REQ_PKTLENGTH_IDX, &pktLength, sizeof(U32));	
		ret = ntySendBuffer(client, pkt, pktLength+NTY_VOICEREQ_EXTEND_LENGTH);	
		ntylog("ntySendBigPacket index:%d,pktLength:%d,Count:%d,send:%d,fromId:%lld,gId:%lld,toId:%lld\n", i,pktLength+NTY_VOICEREQ_EXTEND_LENGTH, Count, ret, fromId,gId,toId);
		//ntylog(" pktLength:%d, Count:%d, ret:%d, selfIdx:%d\n",pktLength+NTY_VOICEREQ_EXTEND_LENGTH, Count, ret, NTY_PROTO_VOICEREQ_SELFID_IDX);
		usleep(10000); //add by Rain 2017-10-29
#if 0
		usleep(20 * 1000); //Window Send
#endif
	}

	return 0;
}

int ntySendBinBufferBroadCastIter(void *self, void *arg) {
	C_DEVID toId = 0, selfId = 0;
	memcpy(&toId, self, sizeof(C_DEVID));

	InterMsg *msg = arg;
	U8 *data = msg->buffer;
	int length = msg->length;
	Client *gClient = msg->group;
	U32 index = (U32)msg->arg;
	memcpy(&selfId, msg->self, sizeof(C_DEVID));

	ntylog(" toId:%lld, selfId:%lld\n", toId, selfId);
	if (toId == selfId) return 0;

	ntySendBigPacket(data, length, selfId, gClient->devId, toId, index);
}


int ntySendBinBufferBroadCastResult(U8 *u8Buffer, int length, C_DEVID fromId, C_DEVID gId, U32 index) {
	
	int ret = ntyBigPacketEncode(u8Buffer, length);
	ntylog(" ntySendBigBuffer --> Ret %d, %x, %lld\n", ret, u8Buffer[0], gId);
#if 0
	ntySendBigPacket(u8Buffer, length, gId, index);
#else
  #if ENABLE_RBTREE_REPLACE_BPTREE
	void *map = ntyRBTreeMapInstance();
	Client *pClient = (Client *)ntyMapSearch( map, gId );	
	if ( pClient == NULL){
		ntylog( "ntySendBinBufferBroadCastResult rbtree not exit:%lld\n",gId );
		return NTY_RESULT_NOEXIST;
	}
  #else
	void *heap = ntyBHeapInstance();
	NRecord *record = ntyBHeapSelect(heap, gId);
	if (record == NULL) return NTY_RESULT_NOEXIST;
	Client *pClient = (Client*)record->value;
	if (pClient == NULL) return NTY_RESULT_NOEXIST;
  #endif
	InterMsg *msg = (InterMsg*)malloc(sizeof(InterMsg));
	msg->buffer = u8Buffer;
	msg->length = length;
	msg->group = pClient;
	msg->self = &fromId;
	msg->arg = index;
#endif
	ntyVectorIterator(pClient->friends, ntySendCommonBroadCastIter, msg);
	ntyFree(msg);

	return NTY_RESULT_SUCCESS;
}

int ntySendVoiceBufferResult(U8 *u8Buffer, int length, C_DEVID fromId, C_DEVID gId, C_DEVID toId, U32 index) {
	length = ntyBigPacketEncode(u8Buffer, length);

	return ntySendBigPacket(u8Buffer, length, fromId, gId, toId, index);
}

#endif



int ntySendBindConfirmPushResult(C_DEVID proposerId, C_DEVID devId, C_DEVID adminId, U8 *json, int length) {
	ntylog("ntySendBindConfirmPushResult begin\n");
	ntylog("ntySendBindConfirmPushResult json:%s\n", json);

	U16 bLength = (U16)length;
	U8 buffer[NTY_DATA_PACKET_LENGTH] = {0};
	
	buffer[NTY_PROTO_VERSION_IDX] = NTY_PROTO_VERSION;
	buffer[NTY_PROTO_DEVTYPE_IDX] = NTY_PROTO_CLIENT_DEFAULT;
	buffer[NTY_PROTO_PROTOTYPE_IDX] = PROTO_PUSH;
	buffer[NTY_PROTO_MSGTYPE_IDX] = NTY_PROTO_BIND_CONFIRM_PUSH;

	memcpy(&buffer[NTY_PROTO_BIND_CONFIRM_PUSH_PROPOSER_IDX], &proposerId, sizeof(C_DEVID));
	memcpy(&buffer[NTY_PROTO_BIND_CONFIRM_PUSH_JSON_LENGTH_IDX], &bLength, sizeof(U16));
	memcpy(&buffer[NTY_PROTO_BIND_CONFIRM_PUSH_JSON_CONTENT_IDX], json, bLength);


	bLength = bLength + NTY_PROTO_BIND_CONFIRM_PUSH_JSON_CONTENT_IDX + sizeof(U32);

	void *map = ntyMapInstance();
	ClientSocket *client = (ClientSocket *)ntyMapSearch(map, adminId);
	
#if 0 //
	if (client == NULL) {
		return ntySendPushNotify(adminId, NULL);
	}
#else
	ntySendPushNotify(adminId, devId, NTY_PUSH_BINDCONFIRM_MSG_CONTEXT, NTY_CATEGORY_BINDCONFIRM);
#endif
	

	return ntySendBuffer(client, buffer, bLength);

	
}


