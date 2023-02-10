/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
#include "EnvLibs.h"
#pragma hdrstop

#include "CacheManager.h"
using namespace std;

using namespace std;

CacheManager::CacheManager() : sensitivity(100) {
	cache = new vector<CachedImage>;
}

CacheManager::CacheManager(double sens) : sensitivity(sens) {
	cache = new vector<CachedImage>;
}

CacheManager::~CacheManager()
	{
	if ( cache != nullptr)
		delete cache;
	}


void CacheManager::storeImage(CImage* image, double top, double bot, double left, double right) {
	CachedImage newImage;
	newImage.image = image;
	newImage.top = top;
	newImage.bot = bot;
	newImage.left = left;
	newImage.right = right;

	cache->push_back(newImage);

	return;
}

bool CacheManager::hasImage(double top, double bot, double left, double right) {
	for (int i = 0; i < cache->size(); i++) {
		//I'm pretty sure this math only works if we're in the northern hemisphere...
		if (((*cache)[i].top > top && ((*cache)[i].top - top) < sensitivity) &&
			((*cache)[i].bot < bot && (bot - (*cache)[i].bot) < sensitivity) &&
			((*cache)[i].left < left && (left - (*cache)[i].left) < sensitivity) &&
			((*cache)[i].right > right && ((*cache)[i].right - right) < sensitivity)) {
			return true;
		}
	}
	return false;
}

//NOTE this should only be called after hasImage has returned true
CImage* CacheManager::getImage(double top, double bot, double left, double right) {
	for (int i = 0; i < cache->size(); i++) {
		//I'm pretty sure this math only works if we're in the northern hemisphere...
		if (((*cache)[i].top > top && ((*cache)[i].top - top) < sensitivity) &&
			((*cache)[i].bot < bot && (bot - (*cache)[i].bot) < sensitivity) &&
			((*cache)[i].left < left && (left - (*cache)[i].left) < sensitivity) &&
			((*cache)[i].right > right && ((*cache)[i].right - right) < sensitivity)) {
			return (*cache)[i].image;
		}
	}
	return NULL;
}