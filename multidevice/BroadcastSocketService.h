/* Copyright (C) 1989-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _BroadcastSocketService_H_
#define _BroadcastSocketService_H_

#include "ISocketService.h"

#include "system/SystemCompat.h"
#include "system/PracticalSocket.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::compat;

#include "system/Thread.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::thread;

#include <vector>
#include <string>
using namespace std;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace multidevice {
  class BroadcastSocketService : public ISocketService {
	private:
		static const int port           = 41000;
		unsigned int interfaceIP;
		string broadcastIPAddr;
		string localIPAddr;

		UDPSocket* udpSocket;

		pthread_mutex_t mutexBuffer;
		vector<struct frame*>* outputBuffer;

	public:
		BroadcastSocketService();
		virtual ~BroadcastSocketService();

	private:
		bool buildDomainAddress();
		unsigned int discoverBroadcastAddress();

	public:
		unsigned int getInterfaceIPAddress();
		int getServicePort();
		void dataRequest(char* data, int taskSize, bool repeat=true);

	private:
		bool sendData(struct frame* f);

	public:
		bool checkOutputBuffer();
		bool checkInputBuffer(char* data, int* size);
  };
}
}
}
}
}
}

#endif /*_BroadcastSocketService_H_*/