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

#include "LocalScreenManager.h"
#include "SDLVideoProvider.h"
#include "SDLDeviceScreen.h"
#include "SDLSurface.h"
#include "SDLWindow.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace mb {
	SDLVideoProvider::SDLVideoProvider(GingaScreenID screenId, const char* mrl)
			: SDLAudioProvider(screenId, mrl) {
		
		type = VideoProvider;
				
		myScreen = screenId;
		win      = NULL;
		hasTex   = false;

		if (decoder != NULL) {
			getOriginalResolution(&wRes, &hRes);
		}
	}

	SDLVideoProvider::~SDLVideoProvider() {
		hasTex = false;
	}

	void SDLVideoProvider::setLoadSymbol(string symbol) {
		this->symbol = symbol;
	}

	string SDLVideoProvider::getLoadSymbol() {
		return this->symbol;
	}

	bool SDLVideoProvider::getHasVisual() {
		assert(decoder != NULL);

		return decoder->hasVideoStream();
	}

	void* SDLVideoProvider::getProviderContent() {
		assert(decoder != NULL);

		return (void*)(decoder->getTexture());
	}

	void SDLVideoProvider::setProviderContent(void* texture) {
		assert(decoder != NULL);
		assert(texture != NULL);

		decoder->setTexture((SDL_Texture*)texture);
		textureCreated();
	}

	void SDLVideoProvider::feedBuffers() {

	}

	void SDLVideoProvider::getVideoSurfaceDescription(void* dsc) {

	}

	bool SDLVideoProvider::checkVideoResizeEvent(GingaSurfaceID frame) {
		return false;
	}

	void SDLVideoProvider::getOriginalResolution(int* width, int* height) {
		assert(decoder != NULL);
		assert(width != NULL);
		assert(height != NULL);

		decoder->getOriginalResolution(width, height);
	}

	double SDLVideoProvider::getTotalMediaTime() {
		return SDLAudioProvider::getTotalMediaTime();
	}

	int64_t SDLVideoProvider::getVPts() {
		return SDLAudioProvider::getVPts();
	}

	double SDLVideoProvider::getMediaTime() {
		return SDLAudioProvider::getMediaTime();
	}

	void SDLVideoProvider::setMediaTime(double pos) {
		SDLAudioProvider::setMediaTime(pos);
	}

	void SDLVideoProvider::playOver(GingaSurfaceID surface) {

		GingaWindowID parentId;
		IWindow* parent;

		SDLDeviceScreen::addCMPToRendererList(this);
		parentId = ScreenManagerFactory::getInstance()->getSurfaceParentWindow(surface);

		if (parentId == 0) {
			SDLAudioProvider::playOver(surface);
			return;
		}

		parent = (IWindow*)(ScreenManagerFactory::getInstance()->
				getIWindowFromId(myScreen, parentId));

		clog << "SDLVideoProvider::playOver parent(" << parent << ")" << endl;
		if (ScreenManagerFactory::getInstance()->hasWindow(myScreen, parentId)) {
			win = (SDLWindow*)ScreenManagerFactory::getInstance()->
					getIWindowFromId(myScreen, parentId);
			if (hasTex) {
				((SDLWindow*)win)->setTexture(tex);
				decoder->play();
			}

		} else {
			clog << "SDLVideoProvider::playOver parent(" << parent << ") ";
			clog << "Warning! hasWindow(parent) has returned false" << endl;
		}
	}

	void SDLVideoProvider::resume(GingaSurfaceID surface) {
		SDLAudioProvider::resume(surface);
	}

	void SDLVideoProvider::pause() {
		SDLAudioProvider::pause();
	}

	void SDLVideoProvider::stop() {
		SDLAudioProvider::stop();
	}

	void SDLVideoProvider::setSoundLevel(float level) {
		SDLAudioProvider::setSoundLevel(level);
	}

	bool SDLVideoProvider::releaseAll() {
		//TODO: release all structures
		return false;
	}

	void SDLVideoProvider::refreshDR(void* data) {
		SDLAudioProvider::refreshDR(data);
	}

	bool SDLVideoProvider::textureCreated() {
		assert(decoder != NULL);

		if (!hasTex) {
			if (decoder == NULL) {
				return false;
			}

			tex = decoder->getTexture();
			if (win != NULL) {
				((SDLWindow*)win)->setTexture(tex);
				decoder->play();
			}
			hasTex = true;
			
			return true;
		}

		return false;
	}
}
}
}
}
}
}

extern "C" ::br::pucrio::telemidia::ginga::core::mb::IContinuousMediaProvider*
		createSDLVideoProvider(GingaScreenID screenId, const char* mrl) {

	return (new ::br::pucrio::telemidia::ginga::core::mb::SDLVideoProvider(
			screenId, mrl));
}

extern "C" void destroySDLVideoProvider(
		::br::pucrio::telemidia::ginga::core::mb::
		IContinuousMediaProvider* cmp) {

	delete cmp;
}