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

#include "player/Player.h"

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include "player/PlayersComponentSupport.h"

#include <iostream>

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace player {
	IScreenManager* Player::dm = NULL;

	Player::Player(GingaScreenID screenId, string mrl) {
		Thread::mutexInit(&listM, NULL);
		Thread::mutexInit(&lockedListM, NULL);
		Thread::mutexInit(&referM, NULL);
		Thread::mutexInit(&pnMutex, NULL);

		if (dm == NULL) {
			dm = ScreenManagerFactory::getInstance();
		}

		this->myScreen            = screenId;
		this->mrl                 = mrl;
		this->outputWindow        = 0;
		this->surface             = 0;

		this->notifying           = false;
		this->presented           = false;
		this->visible             = true;
		this->immediatelyStartVar = false;
		this->status              = STOP;
		this->forcedNaturalEnd    = false;
		this->scope               = "";
		this->scopeType           = -1;
		this->scopeInitTime       = -1;
		this->scopeEndTime        = -1;
		this->outTransTime        = -1;
		this->notifyContentUpdate = false;
		this->mirrorSrc           = NULL;
	}

	Player::~Player() {
		set<IPlayer*>::iterator i;

		this->status = STOP;

		Thread::mutexLock(&listM);
		listeners.clear();

		Thread::mutexLock(&lockedListM);
		lockedListeners.clear();

		if (mirrorSrc != NULL) {
			((Player*)mirrorSrc)->removeMirror(this);
		}

		i = mirrors.begin();
		while (i != mirrors.end()) {
			(*i)->setMirrorSrc(NULL);
			++i;
		}
		mirrors.clear();

		if (dm->hasWindow(myScreen, outputWindow)) {
			dm->revertWindowContent(myScreen, outputWindow);
			dm->deleteWindow(myScreen, outputWindow);
			outputWindow = 0;
		}

		if (dm->hasSurface(myScreen, surface)) {
			dm->deleteSurface(surface);
			surface = 0;
		}

		Thread::mutexLock(&referM);
		referredPlayers.clear();

		properties.clear();

		Thread::mutexLock(&pnMutex);
		pendingNotifications.clear();

		Thread::mutexUnlock(&referM);
		Thread::mutexUnlock(&lockedListM);
		Thread::mutexUnlock(&listM);
		Thread::mutexUnlock(&pnMutex);

		Thread::mutexDestroy(&referM);
		Thread::mutexDestroy(&lockedListM);
		Thread::mutexDestroy(&listM);
		Thread::mutexDestroy(&pnMutex);
   	}

	void Player::setMirrorSrc(IPlayer* mirrorSrc) {
		this->mirrorSrc = mirrorSrc;
	}

	void Player::addMirror(IPlayer* mirror) {
		this->mirrors.insert(mirror);
	}

	bool Player::removeMirror(IPlayer* mirror) {
		set<IPlayer*>::iterator i;

		i = mirrors.find(mirror);
		if (i != mirrors.end()) {
			mirrors.erase(i);
			return true;
		}

		return false;
	}

	void Player::setMrl(string mrl, bool visible) {
		this->mrl     = mrl;
		this->visible = visible;
	}

	void Player::setNotifyContentUpdate(bool notify) {
		this->notifyContentUpdate = notify;
	}

	void Player::addListener(IPlayerListener* listener) {
		LockedPlayerListener* lpl = NULL;

		if (notifying) {
			Thread::mutexLock(&lockedListM);
			lpl        = new LockedPlayerListener;
			lpl->isAdd = true;
			lpl->l     = listener;

			lockedListeners.push_back(lpl);
			Thread::mutexUnlock(&lockedListM);

		} else {
			Thread::mutexLock(&listM);
			listeners.insert(listener);
			Thread::mutexUnlock(&listM);
		}
	}

	void Player::removeListener(IPlayerListener* listener) {
		LockedPlayerListener* lpl = NULL;
		set<IPlayerListener*>::iterator i;

		if (notifying) {
			Thread::mutexLock(&lockedListM);
			lpl        = new LockedPlayerListener;
			lpl->isAdd = false;
			lpl->l     = listener;

			lockedListeners.push_back(lpl);
			Thread::mutexUnlock(&lockedListM);

		} else {
			Thread::mutexLock(&listM);
			i = listeners.find(listener);
			if (i != listeners.end()) {
				listeners.erase(i);
			}
			Thread::mutexUnlock(&listM);
		}
	}

	void Player::performLockedListenersRequest() {
		LockedPlayerListener* lpl;
		vector<LockedPlayerListener*>::iterator i;
		IPlayerListener* listener;
		set<IPlayerListener*>::iterator j;

		Thread::mutexLock(&lockedListM);
		i = lockedListeners.begin();
		while (i != lockedListeners.end()) {
			lpl      = *i;
			listener = lpl->l;

			if (lpl->isAdd) {
				listeners.insert(listener);

			} else {
				j = listeners.find(listener);
				if (j != listeners.end()) {
					listeners.erase(j);
				}
			}

			delete lpl;
			++i;
		}

		lockedListeners.clear();
		Thread::mutexUnlock(&lockedListM);
	}

	void Player::notifyPlayerListeners(
			short code, string parameter, short type, string value) {

		PendingNotification* pn;
		vector<PendingNotification*>::iterator i;

		short c;
		string p;
		short t;
		string v;
		set<IPlayerListener*>* clone = NULL;

		if (notifying) {
			Thread::mutexLock(&pnMutex);
			pn            = new PendingNotification;
			pn->code      = code;
			pn->parameter = parameter;
			pn->type      = type;
			pn->value     = value;

			pendingNotifications.push_back(pn);
			Thread::mutexUnlock(&pnMutex);

			return;
		}

		notifying = true;
		Thread::mutexLock(&listM);
		notifying = true;

		performLockedListenersRequest();

		if (listeners.empty()) {
			if (code == PL_NOTIFY_STOP) {
				presented = true;
			}
			Thread::mutexUnlock(&listM);
			notifying = false;
			return;
		}

		if (code == PL_NOTIFY_NCLEDIT) {
			ntsNotifyPlayerListeners(&listeners, code, parameter, type, value);

		} else {
			if (code == PL_NOTIFY_STOP) {
				presented = true;
			}

			ntsNotifyPlayerListeners(&listeners, code, parameter, type, value);
		}

		Thread::mutexLock(&pnMutex);
		if (!pendingNotifications.empty()) {
			clone = new set<IPlayerListener*>(listeners);
		}

		Thread::mutexUnlock(&listM);
		notifying = false;

		if (clone != NULL && !clone->empty()) {
			i = pendingNotifications.begin();
			while (i != pendingNotifications.end()) {
				(*i)->clone = new set<IPlayerListener*>(*clone);

				pthread_t tId;
				pthread_attr_t tattr;

				pthread_attr_init(&tattr);
				pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
				pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);

				pthread_create(&tId, &tattr, Player::detachedNotifier, (*i));
				pthread_detach(tId);

				++i;
			}
			pendingNotifications.clear();
			delete clone;
		}
		Thread::mutexUnlock(&pnMutex);
	}

	void* Player::detachedNotifier(void* ptr) {
		PendingNotification* pn = (PendingNotification*)ptr;

		ntsNotifyPlayerListeners(
				pn->clone, pn->code, pn->parameter, pn->type, pn->value);

		delete pn->clone;
		delete pn;

		return NULL;
	}

	void Player::ntsNotifyPlayerListeners(
			set<IPlayerListener*>* list,
			short code, string parameter, short type, string value) {

		set<IPlayerListener*>::iterator i;

		i = list->begin();
		while (i != list->end()) {
			if ((*i) != NULL) {
				(*i)->updateStatus(code, parameter, type, value);
			}
			++i;
		}
	}

	void Player::setSurface(GingaSurfaceID surface) {
		if (this->surface != 0) {
			dm->deleteSurface(surface);
			this->surface = 0;
		}
		this->surface = surface;
	}

	GingaSurfaceID Player::getSurface() {
		return surface;
	}

	void Player::setMediaTime(double newTime) {
		this->elapsedTime = newTime;
	}

	double Player::getMediaTime() {
		double mediaTime;
		mediaTime = 0;

		if (status == PAUSE) {
			mediaTime = elapsedTime;

		} else {
			mediaTime = elapsedTime + getCurrentTimeMillis() -
				    initTime - elapsedPause;
		}

		return (mediaTime / 1000);
	}

	double Player::getTotalMediaTime() {
		return -1.0;
	}

	bool Player::setKeyHandler(bool isHandler) {
		if (isHandler) {
			SystemCompat::uSleep(200000);
		}
		return false;
	}

	void Player::setScope(
			string scope,
			short type,
			double initTime, double endTime, double outTransDur) {

		clog << "Player::setScope '" << scope << "'" << endl;
		this->scope         = scope;
		this->scopeType     = type;
		this->scopeInitTime = initTime;
		this->scopeEndTime  = endTime;
		this->outTransTime  = outTransDur;
	}

	void Player::mirrorIt(Player* mirrorSrc, Player* mirror) {
		GingaSurfaceID iSrcSur;
		GingaWindowID iSrcWin;

		GingaSurfaceID iSur;
		GingaWindowID iWin;

		if (mirrorSrc != NULL && mirror != NULL) {
			iSrcSur = mirrorSrc->getSurface();
			iSur = mirror->getSurface();
			if (iSrcSur != 0 && iSur != 0) {
				iSrcWin = dm->getSurfaceParentWindow(iSrcSur);
				iWin =  dm->getSurfaceParentWindow(iSur);
				if (iSrcWin != 0 && iWin != 0) {
					dm->setWindowMirrorSrc(myScreen, iWin, iSrcWin);
				}
			}
		}
	}

	void Player::checkMirrors() {
		set<IPlayer*>::iterator i;

		mirrorIt((Player*)mirrorSrc, this);

		i = mirrors.begin();
		while (i != mirrors.end()) {
			mirrorIt(this, (Player*)(*i));
			++i;
		}
	}

	bool Player::play() {
		checkMirrors();
		this->forcedNaturalEnd = false;
		this->status = PLAY;
		if (scopeInitTime > 0) {
			elapsedTime = scopeInitTime * 1000;

		} else {
			elapsedTime = 0;
		}
		elapsedPause = 0;
		initTime = getCurrentTimeMillis();

		return true;
	}

	void Player::stop() {
		this->status = STOP;
	}

	void Player::abort() {
		stop();
	}

	void Player::pause() {
		pauseTime = getCurrentTimeMillis();
		elapsedTime = elapsedTime + (pauseTime - initTime);
		this->status = PAUSE;
	}

	void Player::resume() {
		initTime = getCurrentTimeMillis();
		elapsedPause = elapsedPause + (initTime - pauseTime);
		this->status = PLAY;
	}

	string Player::getPropertyValue(string name) {
		if (properties.count(name) != 0) {
			return properties[name];
		}

		return "";
	}

	void Player::setPropertyValue(string name, string value) {
		vector<string>* params = NULL;

		if (outputWindow != 0) {
			if (name == "bounds") {
				params = split(value, ",");
				if (params->size() == 4) {
					dm->setWindowBounds(myScreen, outputWindow,
															util::stof((*params)[0]),
															util::stof((*params)[1]),
															util::stof((*params)[2]),
															util::stof((*params)[3]));
				}
				delete params;

			} else if (name == "location") {
				params = split(value, ",");
				if (params->size() == 2) {
					dm->moveWindowTo(myScreen, outputWindow,
					                 util::stof((*params)[0]), util::stof((*params)[1]));
				}
				delete params;

			} else if (name == "size") {
				params = split(value, ",");
				if (params->size() == 2) {
					dm->resizeWindow(myScreen, outputWindow,
					                 util::stof((*params)[0]), util::stof((*params)[1]));
				}
				delete params;

			} else if (name == "left") {
				dm->setWindowX(myScreen, outputWindow, util::stof(value));

			} else if (name == "top") {
				dm->setWindowY(myScreen, outputWindow, util::stof(value));

			} else if (name == "width") {
				dm->setWindowW (myScreen, outputWindow, util::stof(value));

			} else if (name == "height") {
				dm->setWindowH (myScreen, outputWindow, util::stof(value));

			} else if (name == "transparency") {
				dm->setWindowCurrentTransparency(myScreen, outputWindow,
				                                 util::stof(value));
			}
		}
		properties[name] = value;
	}

	void Player::addTimeReferPlayer(IPlayer* referPlayer) {
		Thread::mutexLock(&referM);
		referredPlayers.insert(referPlayer);
		Thread::mutexUnlock(&referM);
	}

	void Player::removeTimeReferPlayer(IPlayer* referPlayer) {
		set<IPlayer*>::iterator i;

		Thread::mutexLock(&referM);
		i = referredPlayers.find(referPlayer);
		if (i != referredPlayers.end()) {
			referredPlayers.erase(i);
			Thread::mutexUnlock(&referM);
			return;
		}
		Thread::mutexUnlock(&referM);
	}

	void Player::notifyReferPlayers(int transition) {
		set<IPlayer*>::iterator i;

		Thread::mutexLock(&referM);
		i = referredPlayers.begin();
		while (i != referredPlayers.end()) {
			(*i)->timebaseObjectTransitionCallback(transition);
			++i;
		}
		Thread::mutexUnlock(&referM);
	}

	void Player::timebaseObjectTransitionCallback(int transition) {
		if (transition == PL_NOTIFY_STOP) {
			setReferenceTimePlayer(NULL);
			stop();
		}
	}

	void Player::setTimeBasePlayer(IPlayer* timeBasePlayer) {
		clog << "Player::setTimeBasePlayer" << endl;
		if (timeBasePlayer != NULL) {
			this->timeBasePlayer = timeBasePlayer;
			this->timeBasePlayer->addTimeReferPlayer(this);
		}
	}

	bool Player::hasPresented() {
		return presented;
	}

	void Player::setPresented(bool presented) {
		this->presented = presented;
	}

	bool Player::isVisible() {
		return this->visible;
	}

	void Player::setVisible(bool visible) {
		this->visible = visible;
	}

	bool Player::immediatelyStart() {
		return immediatelyStartVar;
	}

	void Player::setImmediatelyStart(bool immediattelyStartVal) {
		this->immediatelyStartVar = immediattelyStartVal;
	}

	void Player::checkScopeTime() {
		pthread_t threadId_;

		if (scopeInitTime >= 0 && scopeEndTime >= scopeInitTime) {
			pthread_create(
					&threadId_, 0, Player::scopeTimeHandler, (void*)this);

			pthread_detach(threadId_);
		}
	}

	void* Player::scopeTimeHandler(void* ptr) {
		double expectedSleepTime;
		Player* p = (Player*)ptr;

		expectedSleepTime = (
				p->scopeEndTime -
				(p->scopeInitTime + (p->getMediaTime() / 1000)));

		clog << "Player::scopeTimeHandler expectedSleepTime = '";
		clog << expectedSleepTime << "'" << endl;

		if (expectedSleepTime > 0) {
			if (p->outTransTime > 0.0) {
				expectedSleepTime = (
						p->outTransTime -
						(p->scopeInitTime + (p->getMediaTime() / 1000)));
			}

			SystemCompat::uSleep(expectedSleepTime * 1000000);

			if (p->outTransTime > 0.0) {
				p->notifyPlayerListeners(PL_NOTIFY_OUTTRANS);

				expectedSleepTime = (
						p->scopeEndTime -
						(p->scopeInitTime + (p->getMediaTime() / 1000)));

				SystemCompat::uSleep(expectedSleepTime * 1000000);
			}
		}

		p->forceNaturalEnd(true);

		return ptr;
	}

	void Player::forceNaturalEnd(bool forceIt) {
		forcedNaturalEnd = forceIt;
		if (forceIt) {
			notifyPlayerListeners(PL_NOTIFY_STOP);
		}
	}

	bool Player::isForcedNaturalEnd() {
		if (mrl == "") {
			return false;
		}
		return forcedNaturalEnd;
	}

	bool Player::setOutWindow(GingaWindowID windowId) {
		if (surface != 0 && dm->getSurfaceParentWindow(surface) == 0) {
			dm->setSurfaceParentWindow(myScreen, surface, windowId);
		}
		return true;
	}
}
}
}
}
}
}