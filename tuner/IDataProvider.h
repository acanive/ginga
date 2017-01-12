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

#ifndef IDATAPROVIDER_H_
#define IDATAPROVIDER_H_

#include <stdint.h>

#include "IChannel.h"
#include "IProviderListener.h"
#include "IFrontendFilter.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace tuning {
	//data provider capabilities
	static const short DPC_CAN_FETCHDATA = 0x01;
	static const short DPC_CAN_DEMUXBYHW = 0x02;
	static const short DPC_CAN_FILTERPID = 0x04;
	static const short DPC_CAN_FILTERTID = 0x08;
	static const short DPC_CAN_DECODESTC = 0x10;
	static const short DPC_CAN_CTLSTREAM = 0x20;

	// pes filter types
	static const int PFT_DEFAULTTS = 0x01;
	static const int PFT_PCR       = 0x02;
	static const int PFT_VIDEO     = 0x03;
	static const int PFT_AUDIO     = 0x04;
	static const int PFT_OTHER     = 0x05;

	class IDataProvider {
		public:
			virtual ~IDataProvider(){};
			virtual short getCaps()=0;
			virtual void setListener(ITProviderListener* listener)=0;
			virtual void attachFilter(IFrontendFilter* filter)=0;
			virtual void removeFilter(IFrontendFilter* filter)=0;

			virtual char* receiveData(int* len)=0;

			virtual bool tune()=0;
			virtual IChannel* getCurrentChannel()=0;
			virtual bool getSTCValue(uint64_t* stc, int* valueType)=0;
			virtual bool changeChannel(int factor)=0;
			virtual bool setChannel(string channelValue)=0;
			virtual int createPesFilter(
					int pid, int pesType, bool compositeFiler)=0;

			virtual string getPesFilterOutput()=0;
			virtual void close()=0;
	};
}
}
}
}
}
}

#endif /*IDATAPROVIDER_H_*/