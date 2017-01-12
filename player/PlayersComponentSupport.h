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

#ifndef PLAYERSCOMPONENTSUPPORT_H_
#define PLAYERSCOMPONENTSUPPORT_H_

#include "config.h"

#if HAVE_ISDBT
# include "dataprocessing/EPGProcessor.h"
using namespace ::br::pucrio::telemidia::ginga::core::dataprocessing::epg;
#endif

#if HAVE_IC
#include "ic/InteractiveChannelManager.h"
#endif

#include "system/GingaLocatorFactory.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::fs;

#include "mb/LocalScreenManager.h"
using namespace ::br::pucrio::telemidia::ginga::core::mb;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace player {
}
}
}
}
}
}

#endif /* PLAYERSCOMPONENTSUPPORT_H_ */
