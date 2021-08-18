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
// GNU General Public License Agreement
// Copyright (C) 2004-2010 CodeCogs, Zyba Ltd, Broadwood, Holford, TA5 1DU, England.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by CodeCogs. 
// You must retain a copy of this licence in all copies. 
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public License for more details.
// ---------------------------------------------------------------------------------
//! Calculates the zeros of a function using the secant method.

#ifndef MATHS_ROOTFINDING_SECANT_H
#define MATHS_ROOTFINDING_SECANT_H

#include <assert.h>
#include <cmath>
#define ABS(x) ((x) < 0 ? -(x) : (x))

namespace Maths
{

namespace RootFinding
{

//! Calculates the zeros of a function using the secant method.
	double f(double x, double beta, int function, double alpha, double yo, double A) {
		if (function == 1)
		{
			return cos(2 * x) - (1 / beta)*sin(2 * x) - exp(-2 * x / beta);
		}
		else

			return (pow(pow(x, (-1.0f / alpha)) + pow(yo, (-1.0f / alpha)), (alpha - 1)))*(pow(yo, ((alpha - 1) / alpha)))*exp(-pow((pow(x, (-1.0f / alpha)) + pow(yo, (-1.0f / alpha))), alpha) + (1.0f / yo)) - A;


	}

	double secant(double beta, int function, double alpha, double yo, double A, double x0 = -1E+7, double x1 = 1E+7, double eps = 1E-10, int maxit = 1000)
{

    assert(x0 < x1);

    double x;

    int n = 0;
    do {

		double y0 = f(x0, beta, function, alpha, yo, A), y1 = f(x1, beta, function, alpha, yo, A);
        if (ABS(y1 - y0) < 0.001) y1 += 0.001;

        x = (x0 * y1 - x1 * y0) / (y1 - y0), x0 = x1, x1 = x;

        n++;

    } while (n < maxit && ABS(x1 - x0) >= eps);

    return x;
}

}

}

#undef ABS

#endif

