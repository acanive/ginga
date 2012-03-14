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

#include "multidevice/services/BaseDeviceDomain.h"
#include "multidevice/services/device/ActiveDeviceService.h"
#include "multidevice/services/device/PassiveDeviceService.h"

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace multidevice {
	char DeviceDomain::headerStream[HEADER_SIZE];
	char* DeviceDomain::mdFrame           = new char[MAX_FRAME_SIZE];
	unsigned int DeviceDomain::myIP       = 0;
	bool DeviceDomain::taskIndicationFlag = false;

	DeviceDomain::DeviceDomain() {
		deviceClass       = -1;
		deviceWidth       = -1;
		deviceHeight      = -1;
		schedulePost      = -1;
		schedDevClass     = -1;
		sentTimeStamp     = -1;
		receivedTimeStamp = -1;
		deviceService     = NULL;
		newAnswerPosted   = false;
		connected         = false;

		clearHeader();

		broadcastService = new BroadcastSocketService();
		myIP = broadcastService->getInterfaceIPAddress();

		/* MUDAR */
		res = new RemoteEventService();
		res->addDeviceClass(1);
		res->addDeviceClass(2);
		/* MUDAR */
	}

	DeviceDomain::~DeviceDomain() {
		if (deviceService != NULL) {
			delete deviceService;
			deviceService = NULL;
		}

		if (broadcastService != NULL) {
			delete broadcastService;
			broadcastService = NULL;
		}
	}

	bool DeviceDomain::isConnected() {
		return connected;
	}

	void DeviceDomain::clearHeader() {
		sourceIp  = 0;
		destClass = -1;
		frameType = -1;
		frameSize = 0;
	}

	bool DeviceDomain::broadcastTaskRequest(char* data, int taskSize) {
		broadcastService->dataRequest(data, taskSize);
		return true;
	}

	char* DeviceDomain::taskReceive() {
		char* data = NULL;

		memset(headerStream, 0, HEADER_SIZE);
		memcpy(headerStream, mdFrame, HEADER_SIZE);
		parseTaskHeader();
		printTaskHeader();

		if (frameSize == 0) {
			memset(headerStream, 0, HEADER_SIZE);
			if (frameType != FT_KEEPALIVE) {
				clog << "DeviceDomain::taskReceive Warning! ";
				clog << " empty payload in a non keep alive frame" << endl;

			} else {
				clog << "DeviceDomain::taskReceive Keep Alive! ";
				clog << endl;
			}
			return NULL;
		}

		try {
			data = new char[frameSize];

		} catch (bad_alloc &e) {
			clog << "DeviceDomain::taskReceive Warning! ";
			clog << "can't alloc '" << frameSize << "' bytes." << endl;
			return NULL;
		}

		memset(data, 0, frameSize);
		memcpy(data, mdFrame + HEADER_SIZE, frameSize);

		return data;
	}

	void DeviceDomain::parseTaskHeader() {
		clearHeader();

		sourceIp = getUIntFromStream(headerStream + 1);
		destClass = (((unsigned char)headerStream[5]) & 0xFF);
		frameType = (((unsigned char)headerStream[6]) & 0xFF);
		frameSize = getUIntFromStream(headerStream + 7);
	}

	void DeviceDomain::printTaskHeader() {
		clog << "FrameId = '";
		clog << (int)(unsigned char)headerStream[0];
		clog << "' SourceIp = '" << sourceIp << "', which means '";
		clog << getStrIP(sourceIp);
		clog << "', destClase = '" << destClass << "' header[5] = '";
		clog << (int)(unsigned char)headerStream[5];
		clog << "', frameType = '" << frameType << "' header[6] = '";
		clog << (int)(unsigned char)headerStream[6];
		clog << "', frameSize = '" << frameSize << "' header[7] = '";
		clog << (int)(unsigned char)headerStream[7];
		clog << "', header[8] = '";
		clog << (int)(unsigned char)headerStream[8];
		clog << "' header[9] = '";
		clog << (int)(unsigned char)headerStream[9];
		clog << "', header[10] = '";
		clog << (int)(unsigned char)headerStream[10];
		clog << "'" << endl;
	}

	bool DeviceDomain::addDevice(
			int reqDeviceClass, int width, int height) {

		bool added = false;

		if (reqDeviceClass == 2) {
			clog << "DeviceDomain::addDevice adding new device - class 2...";
			clog << endl;

			res->addDevice(
					reqDeviceClass,
					sourceIp,
					(char*)getStrIP(sourceIp).c_str());
		}


		if (deviceService != NULL) {
			added = deviceService->addDevice(
				sourceIp, reqDeviceClass, width, height);
		}

		return added;
	}

	void DeviceDomain::postConnectionRequestTask() {
		postConnectionRequestTask(deviceWidth, deviceHeight);
	}

	void DeviceDomain::postEventTask(
			int destDevClass, int frameType, char* payload, int payloadSize) {

		char* task;
		string _doc;
		int taskSize;

		//prepare frame
		if (destDevClass == IDeviceDomain::CT_ACTIVE) {
			if (frameType == IDeviceDomain::FT_PRESENTATIONEVENT) {
				if (strstr(payload,"start::") != NULL) {
					_doc.assign(payload + 7, payloadSize - 7);
					clog << "DeviceDomain::postEventTask calling ";
					clog << "startDocument with doc = '";
					clog << _doc << "'" << endl;
					res->startDocument(2, (char*)(_doc.c_str()));

				} else if (strstr(payload,"stop::") != NULL) {
					clog << "DeviceDomain::postEventTask calling ";
					clog << "stopDocument" << endl;

					_doc.assign(payload + 6, payloadSize - 6);
					res->stopDocument(2, (char*)(_doc.c_str()));
				}
			}

		} else {
			task = mountFrame(myIP, destDevClass, frameType, payloadSize);

			memcpy(task + HEADER_SIZE, payload, payloadSize);

			taskSize = HEADER_SIZE + payloadSize;
			taskRequest(destDevClass, task, taskSize);
		}
	}

	void DeviceDomain::setDeviceInfo(int width, int height) {
		this->deviceWidth  = width;
		this->deviceHeight = height;
	}

	int DeviceDomain::getDeviceClass() {
		return deviceClass;
	}

	void DeviceDomain::checkDomainTasks() {
		int tmpClass, res;
		double receivedElapsedTime;

		if (deviceClass < 0) {
			clog << "DeviceDomain::checkDomainTasks ";
			clog << "Warning! deviceClass = " << deviceClass << endl;
			return;
		}

		if (schedulePost >= 0) {
			tmpClass = schedDevClass;
			res = schedulePost;

			schedulePost  = -1; //Modificado por Roberto
			schedDevClass = -1;

			switch (res) {
				case FT_ANSWERTOREQUEST:
					postAnswerTask(tmpClass, true);
					newAnswerPosted = true;
					break;

				default:
					clog << "DeviceDomain::checkDomainTasks RES = '";
					clog << res << "'" << endl;
					break;
			}
		}

		if (!taskIndicationFlag) {
			if (broadcastService->checkInputBuffer(mdFrame, &bytesRecv)) {
				taskIndicationFlag = true;
				if (runControlTask()) {
					receivedTimeStamp = getCurrentTimeMillis();
				}
			}

		} else {
			clog << "DeviceDomain::checkDomainTasks can't process input ";
			clog << "buffer: task indication flag is true" << endl;
		}

		broadcastService->checkOutputBuffer();

		/*} else {
			receivedElapsedTime = getCurrentTimeMillis() - receivedTimeStamp;
 			if (receivedElapsedTime > IFS || receivedTimeStamp == -1) {
 				if (getCurrentTimeMillis() - sentTimeStamp > IFS ||
 						sentTimeStamp == -1) {

 					if (broadcastService->checkOutputBuffer()) {
 						sentTimeStamp = getCurrentTimeMillis();
 					}

 				} else {
 	 				clog << "DeviceDomain::checkDomainTasks waiting IFS by ";
 	 				clog << "sentElapsedTime '";
 	 				clog << getCurrentTimeMillis() - sentTimeStamp;
 	 				clog << "' when sentTimeStamp = '" << sentTimeStamp;
 	 				clog << "'" << endl;
 				}

 			} else {
 				clog << "DeviceDomain::checkDomainTasks waiting IFS by ";
 				clog << "reveivedElapsedTime" << endl;
 			}
		}*/
	}

	void DeviceDomain::addDeviceListener(IRemoteDeviceListener* listener) {
		if (deviceService != NULL) {
			deviceService->addListener(listener);
		}
	}

	void DeviceDomain::removeDeviceListener(IRemoteDeviceListener* listener) {
		if (deviceService != NULL) {
			deviceService->removeListener(listener);
		}
	}
}
}
}
}
}
}
