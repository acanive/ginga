/******************************************************************************
Este arquivo eh parte da implementacao do ambiente declarativo do middleware
Ginga (Ginga-NCL).

Direitos Autorais Reservados (c) 1989-2007 PUC-Rio/Laboratorio TeleMidia

Este programa eh software livre; voce pode redistribui-lo e/ou modificah-lo sob
os termos da Licenca Publica Geral GNU versao 2 conforme publicada pela Free
Software Foundation.

Este programa eh distribuido na expectativa de que seja util, porem, SEM
NENHUMA GARANTIA; nem mesmo a garantia implicita de COMERCIABILIDADE OU
ADEQUACAO A UMA FINALIDADE ESPECIFICA. Consulte a Licenca Publica Geral do
GNU versao 2 para mais detalhes.

Voce deve ter recebido uma copia da Licenca Publica Geral do GNU versao 2 junto
com este programa; se nao, escreva para a Free Software Foundation, Inc., no
endereco 59 Temple Street, Suite 330, Boston, MA 02111-1307 USA.

Para maiores informacoes:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
******************************************************************************
This file is part of the declarative environment of middleware Ginga (Ginga-NCL)

Copyright: 1989-2007 PUC-RIO/LABORATORIO TELEMIDIA, All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 2 as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License version 2 for more
details.

You should have received a copy of the GNU General Public License version 2
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

For further information contact:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
*******************************************************************************/


#include "multidevice/services/network/BroadcastSocketService.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#ifndef _MSC_VER
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#endif

#ifdef _MSC_VER
typedef int socklen_t;
#endif

#include "system/compat/SystemCompat.h"
#include "system/compat/PracticalSocket.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::compat;

#ifdef __APPLE__
  #ifdef __DARWIN_UNIX03
    #include <ifaddrs.h>
    #define inaddrr(x) (*(struct in_addr *) myAddr->x[sizeof sa.sin_port])
  #endif
#endif

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace multidevice {

#ifdef __APPLE__
  #ifdef __DARWIN_UNIX03
	static int sd = -1;
	static struct sockaddr_in domain_addr;
	static int                domain_addr_len;
	static struct sockaddr_in broadcast_addr;
	static socklen_t          broadcast_addr_len;
  #endif
#else
	static UDPSocket* udpSocket;
#endif

	BroadcastSocketService::BroadcastSocketService() {
		interfaceIP  = 0;
		broadcastIPAddr = "0.0.0.0";
		outputBuffer = new vector<struct frame*>;
		udpSocket       = NULL;

		pthread_mutex_init(&mutexBuffer, NULL);

		if (buildDomainAddress()) {
#ifdef __DARWIN_UNIX03
			interfaceIP = discoverBroadcastAddress();
#else

			try {
				broadcastIPAddr = udpSocket->getBroadcastAddress();
				interfaceIP = udpSocket->getLocalIPAddress();
				clog << endl << "broadcastIPAddr= " << broadcastIPAddr << endl;
				clog << "interfaceIP= " << interfaceIP << endl;
				//udpSocket->setLocalAddressAndPort(broadcastIPAddr,port);

			}
			catch (SocketException &e) {
				clog << e.what() << endl;
				clog << " BroadcastSocketService::getBroadcastAddress() error! " << endl;
			}
#endif
		}
		else {
			try {
				//TODO: configure port
				udpSocket = new UDPSocket(42088);
				udpSocket->setNonBlocking(true);
				clog << "BroadcastSocketService: using UDP port 42088 for device search" << endl;
				broadcastIPAddr = udpSocket->getBroadcastAddress();
				interfaceIP = udpSocket->getLocalIPAddress();
				clog << endl << "broadcastIPAddr= " << broadcastIPAddr << endl;
				clog << "interfaceIP= " << interfaceIP << endl;
				
			}
			catch (SocketException &e) {
				clog << e.what() << endl;
				clog << " BroadcastSocketService::getLocalIPAddress() error! " << endl;
			}
		}
	}

	BroadcastSocketService::~BroadcastSocketService() {
#if __APPLE__
  #ifdef __DARWIN_UNIX03
		if (sd > 0) {
			close(sd);
		}
  #endif
#else
		if (udpSocket != NULL) {
			udpSocket->disconnect();
			//udpSocket->cleanUp();
			delete udpSocket;
		}

		if (outputBuffer != NULL) {
			delete outputBuffer;
			outputBuffer = NULL;
		}
#endif
		Thread::mutexDestroy(&mutexBuffer);
	}

	bool BroadcastSocketService::buildDomainAddress() {
#ifndef __DARWIN_UNIX03
		try {
			udpSocket = new UDPSocket();
			udpSocket->setReuseAddr(true);
			udpSocket->setLocalPort(port);
			udpSocket->setNonBlocking(true);
			return true;
		}
		catch (SocketException &e) {
			clog << e.what() << endl;
			clog << "BroadcastSocketService::buildClientAddress Warning!" << endl;
			return false;
		}
#elif __APPLE__
  #ifdef __DARWIN_UNIX03
		int ret;
		int trueVar = 1;

		sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sd == -1) {
			clog << "BroadcastSocketService::buildClientAddress Warning!";
			clog << " can't create socket (sd == -1)" << endl;
			return false;
		}
		setsockopt(sd, SOL_SOCKET, SO_BSDCOMPAT, &trueVar, sizeof(trueVar));
		domain_addr.sin_family       = AF_INET;
		domain_addr.sin_port         = htons(port);
		domain_addr.sin_addr.s_addr  = INADDR_ANY;
		domain_addr_len              = sizeof(domain_addr);
		ret = bind(sd, (struct sockaddr *)&domain_addr, domain_addr_len);

		if (ret == -1) {
			clog << "BroadcastSocketService::buildClientAddress Warning!";
			clog << " can't bind socket (ret == -1)" << endl;
			return false;
		}
  #endif
#endif
		return true;

	}

	unsigned int BroadcastSocketService::discoverBroadcastAddress() {
//kept for compatibility with the iOS passive device client
//TODO: organize/clean ios defines
#ifdef __APPLE__
  #ifdef __DARWIN_UNIX03

		struct ifconf interfaces;
		struct ifreq* netInterface;
		struct sockaddr_in* myAddr;
		int numOfInterfaces, result, i;
		string interfaceName;
		bool validInterface;
		char buffer[1024000];

		interfaces.ifc_len = sizeof(buffer);
		interfaces.ifc_buf = buffer;
		result = ioctl(sd, SIOCGIFCONF, (char *) &interfaces);
		netInterface = interfaces.ifc_req;
		numOfInterfaces = interfaces.ifc_len/sizeof(struct ifreq);

		struct ifaddrs *ifaddr, *ifa;
		int family, s;
		char host[NI_MAXHOST];

		if (getifaddrs(&ifaddr) == -1) {
			perror("getifaddrs");
			SystemCompat::gingaProcessExit(EXIT_FAILURE);
		}

		/* Walk through linked list, maintaining head pointer so we
		can free list later */

		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
			family = ifa->ifa_addr->sa_family;

			/* Display interface name and family (including symbolic
			form of the latter for the common families) */

			printf("%s  address family: %d%s\n",
			ifa->ifa_name, family,
			(family == AF_INET) ?   " (AF_INET)" :
			(family == AF_INET6) ?  " (AF_INET6)" : "");

			/* For an AF_INET* interface address, display the address */
			validInterface = (strcmp("en0", ifa->ifa_name) == 0);
			if (validInterface && (family == AF_INET || family == AF_INET6)) {
				s = getnameinfo(ifa->ifa_addr,
				(family == AF_INET) ? sizeof(struct sockaddr_in) :
				sizeof(struct sockaddr_in6),
				host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

				if (s != 0) {
					clog << "BroadcastSocketService::discoverBroadcastAddress";
					clog << " getnameinfo() failed: " << gai_strerror(s);
					clog << endl;
					return 0;
				}

				broadcast_addr.sin_family = AF_INET;
				broadcast_addr.sin_port = htons(port);
				broadcast_addr_len      = sizeof(broadcast_addr);
				broadcast_addr.sin_addr.s_addr = inet_addr(inet_ntoa(
						((struct sockaddr_in *)ifa->ifa_broadaddr)->sin_addr));

				/*clog << "Broadcast addr = ";
				clog << inet_ntoa(
						((struct sockaddr_in *)ifa->ifa_broadaddr)->sin_addr);

				clog << endl;*/

				memset(
						broadcast_addr.sin_zero,
						'\0',
						sizeof(broadcast_addr.sin_zero));

				if (result >= 0) {
					clog << "BroadcastSocketService::";
					clog << "discoverBroadcastAddress interfaceName: '";
					clog << ifa->ifa_name;

					inet_aton(
							host,
							&(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));

					freeifaddrs(ifaddr);
					return (unsigned int)(((struct sockaddr_in *)
							ifa->ifa_addr)->sin_addr.s_addr);
				}
			}
		}

		freeifaddrs(ifaddr);
  #endif
#endif
		clog << "BroadcastSocketService::discoverBroadcastAddress Warning!";
		clog << " can't discover broadcast address" << endl;
		return 0;
	}

	unsigned int BroadcastSocketService::getInterfaceIPAddress() {
		return interfaceIP;
	}

	int BroadcastSocketService::getServicePort() {
		return port;
	}

	void BroadcastSocketService::dataRequest(
			char* data, int taskSize, bool repeat) {

		struct frame* f;
		pthread_mutex_lock(&mutexBuffer);
		f = new struct frame;
		f->data      = data;
		f->size      = taskSize;
		f->repeat    = repeat;
		outputBuffer->push_back(f);
		pthread_mutex_unlock(&mutexBuffer);
	}

	bool BroadcastSocketService::sendData(struct frame* f) {

		char* data;
		int taskSize, result, i;

		data     = f->data;
		taskSize = f->size;

		//clog << "BroadcastSocketService::sendData Sending to "<< broadcastIPAddr;
		//clog << " taskSize = " << taskSize << endl; //<< "' and headerSize = '";
		//clog << headerSize << "'" << endl;

#ifndef __DARWIN_UNIX03
		try {
			for (i = 0; i < NUM_OF_COPIES; i++) {
				udpSocket->sendTo(data,taskSize,broadcastIPAddr,port);
			}
		}
		catch (SocketException &e) {
			clog << e.what() << endl;
			clog << "BroadcastSocketService::sendData Error!!";
			return false;
		}
#elif __APPLE__
  #ifdef __DARWIN_UNIX03
		for (i = 0; i < NUM_OF_COPIES; i++) {
			result = sendto(
					sd,
					data,
					taskSize,
					0,
					(struct sockaddr*)&broadcast_addr,
					broadcast_addr_len);

			if (result == -1) {
				perror("BroadcastSocketService::taskRequest sendto");
				return false;
			}

		}
  #endif
#endif
	return true;

	}

	bool BroadcastSocketService::checkOutputBuffer() {
		vector<struct frame*>::iterator i;
		bool sent = false;
		struct frame* f = NULL;
	    clog << "=========== checkOutputBuffer ==========="<<endl;
		pthread_mutex_lock(&mutexBuffer);
		i = outputBuffer->begin();
		if (i != outputBuffer->end()) {
			f = *i;
			sent = sendData(f);
			if (sent) {
				outputBuffer->erase(i);
				delete[] f->data;
				f->data = NULL;
				delete f;
				f = NULL;
			}

		} else {
			/*clog << "BroadcastSocketService::checkOutputBuffer ";
			clog << "empty buffer" << endl;*/
				}
		pthread_mutex_unlock(&mutexBuffer);

		return sent;
	}

	bool BroadcastSocketService::checkInputBuffer(char* data, int* size) {
#ifndef __DARWIN_UNIX03
		int res = 0;
		int recvFrom = 0;
		int sz = 0;
		char recvString[MAX_FRAME_SIZE + 1];
	    string sourceAddress = "";
	    unsigned short sourcePort = 0;
	    //TODO: checkInput loop fix

	    if (udpSocket == NULL) {
		
		return false;
	    }

	    res = udpSocket->select_t(0,0);

	    clog << "=========== checkInputBuffer ==========="<<endl;
		switch (res) {
			case -1:
				clog << "BroadcastSocketService::checkInputBuffer ERROR res=-1"<<endl;
				//clog << "Warning! select ERRNO = " << errno << endl;
				memset(data, 0, MAX_FRAME_SIZE);
				return false;

			case 1: {
					clog << "BroadcastSocketService::checkInputBuffer ";
					clog << "receiving data ..." << endl;

					memset(data, 0, MAX_FRAME_SIZE);
					memset(recvString, 0, MAX_FRAME_SIZE);
					try {
						sz = udpSocket->recvFrom(recvString, MAX_FRAME_SIZE, sourceAddress, sourcePort);

						if (sz > 0) {
							recvString[sz] = '\0';
							clog << "BroadcastSocketService::udpSocket->recvFrom " << sourceAddress;
							clog << " " << sz << " bytes" << endl;
						}
						else {
							return false;
						}
						//if (sourceAddress == interfaceIP)

					}
					catch (SocketException &e) {
						clog << e.what() << endl;
						memset(data, 0, MAX_FRAME_SIZE);
						memset(recvString, 0, MAX_FRAME_SIZE);
						return false;
					}
					if (sz <= HEADER_SIZE) {
						clog << "BroadcastSocketService::checkInputBuffer ";
						clog << "Warning! Received invalid frame: ";
						clog << "bytes received = '" << sz << "' ";
						clog << "HEADER_SIZE = '" << HEADER_SIZE << "' ";
						clog << endl;
						memset(data, 0, MAX_FRAME_SIZE);
						memset(recvString, 0, MAX_FRAME_SIZE);
						return false;
					}

					*size = sz;
					memcpy(data, recvString, (int)(*size));

					recvFrom = getUIntFromStream(data + 1);

					if (!isValidRecvFrame(recvFrom, interfaceIP, data)) {
						clog << "BroadcastSocketService::checkInputBuffer if (!isValidRecvFrame())" << endl;
						memset(data, 0, MAX_FRAME_SIZE);
						memset(recvString, 0, MAX_FRAME_SIZE);

						return false;
					}
					else {
						clog << "BroadcastSocketService::checkInputBuffer if (isValidRecvFrame())" << endl;
						memset(recvString, 0, MAX_FRAME_SIZE);
					}
					break;
				}


			default:
				memset(data, 0, MAX_FRAME_SIZE);
				memset(recvString, 0, MAX_FRAME_SIZE);
				return false;
		}
#elif __APPLE__
  #ifdef __DARWIN_UNIX03
		int nfds, res, recvFrom;
		fd_set fdset;
		struct timeval tv_timeout;

		FD_ZERO(&fdset);
		FD_SET(sd, &fdset);
		nfds               = sd + 1;
		tv_timeout.tv_sec  = 0;
		tv_timeout.tv_usec = 0;

		res = select(nfds, &fdset, NULL, NULL, &tv_timeout);

		switch (res) {
			case -1:
				clog << "BroadcastSocketService::checkInputBuffer ";
				clog << "Warning! select ERRNO = " << errno << endl;
				memset(data, 0, MAX_FRAME_SIZE);
				return false;

			case 1:
				clog << "BroadcastSocketService::checkInputBuffer ";
				clog << "receiving data ..." << endl;

				memset(data, 0, MAX_FRAME_SIZE);
				//result = recv(sd, headerStream, headerSize, 0);
				*size = recvfrom(
						sd,
						data,
						MAX_FRAME_SIZE,
						MSG_DONTWAIT,
						(struct sockaddr*)&broadcast_addr,
						&broadcast_addr_len);

				if (*size == -1) {
					if (errno != EAGAIN) {
						clog << "BroadcastSocketService::checkInputBuffer ";
						herror("check domain error: ");
						clog << "Warning! receive data ERRNO = " << errno;
						clog << endl;
						memset(data, 0, MAX_FRAME_SIZE);
						return false;

					} else {
						memset(data, 0, MAX_FRAME_SIZE);
						return false;
					}
				}

				if (*size <= HEADER_SIZE) {
					clog << "BroadcastSocketService::checkInputBuffer ";
					clog << "Warning! Received invalid frame: ";
					clog << "bytes received = '" << *size << "' ";
					clog << "HEADER_SIZE = '" << HEADER_SIZE << "' ";
					clog << endl;

					memset(data, 0, MAX_FRAME_SIZE);
					return false;
				}

				recvFrom = getUIntFromStream(data + 1);
				if (!isValidRecvFrame(recvFrom, data)) {
					memset(data, 0, MAX_FRAME_SIZE);
					return false;
				}
				break;

			default:
				memset(data, 0, MAX_FRAME_SIZE);
				return false;
		}
  #endif
#endif
		return true;
	}
}
}
}
}
}
}
