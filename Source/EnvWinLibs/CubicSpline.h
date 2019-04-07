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
#include "EnvWinLibs.h"

const float DIV_FACTOR = 4.0f; //adjust this factor to adjust the curve smoothness

class Curve
{
public:
	float  ax,ay;
	float  bx,by;
	float  cx,cy;
	int    nDiv;

   // constructors
	Curve(float _ax,float _ay,float _bx,float _by,float _cx,float _cy,int _nDiv);
	Curve(float _ax,float _ay,float _bx,float _by,float _cx,float _cy);
	Curve() {};

	void PutCurve(float _ax,float _ay,float _bx,float _by,float _cx,float _cy);
	void Draw( HDC hdc, float x, float y);
	int  GetCount();
	void GetCurve(float x,float y, POINT points[], int& pointCount);	
};



class  WINLIBSAPI CubicSpline
{
friend class Curve;

protected:
	float* px;
	float* py;
	float* ax;
	float* ay;
	float* bx;
	float* by;
	float* cx;
	float* cy;
	float* k;
	float* mat[3];

	int  numPoints;

public:
	// constructors
	CubicSpline(POINT pt[], int np);
	CubicSpline(float _px[] , float _py[] , int np) ;
	~CubicSpline();

	void Generate();
	void Draw( HDC hdc );
	void DrawClosed( HDC hdc );
	int  GetCurveCount();
	void GetCurve(POINT points[], int& PointCount);
	void GenClosed();

protected:
	void MatrixSolve(float B[]);

   ///// tridiagonal matrix + elements of [0][0], [N-1][N-1] //// 
	void MatrixSolveEX(float B[]);
};



// CubicSplineWnd

class WINLIBSAPI CubicSplineWnd : public CWnd
{
	DECLARE_DYNAMIC(CubicSplineWnd)

public:
	CubicSplineWnd();
	virtual ~CubicSplineWnd();

   int  AddControlPoint( POINT &pt ) { return (int) m_controlPoints.Add( pt ); }
   void AllowAddPoints ( bool flag ) { m_allowAddPoints = flag; }

protected:
   int m_moveIndex;

   bool m_allowAddPoints;

	CArray<POINT, POINT> m_controlPoints;
	CArray<POINT, POINT> m_curvePoints;
   
   int IsInsideControlPoint( POINT &point);
   void Update(HDC& hDC);

protected:
 	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
};

