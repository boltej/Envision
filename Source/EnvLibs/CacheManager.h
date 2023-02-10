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
#pragma once
#include "EnvLibs.h"

#include "MAP.h"
#include <vector>

using namespace std;

struct CachedImage {
	double top;
	double bot;
	double left;
	double right;
	CImage* image;
};

class LIBSAPI CacheManager {
	private:
		vector<CachedImage> *cache;
		double sensitivity;
	public:
		CacheManager();
		CacheManager(double sensitivity);
		~CacheManager();
		bool hasImage(double top, double bot, double left, double right);
		CImage* getImage(double top, double bot, double left, double right);
		void storeImage(CImage* image, double top, double bot, double left, double right);
};