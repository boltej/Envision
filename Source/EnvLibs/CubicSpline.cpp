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
#include "libs.h"
#pragma hdrstop

#include "CubicSpline.h"
#include <math.h>

double Distance(const POINT& p1, const POINT& p2);
void DrawCross(POINT point, COLORREF rgbColor, int extent, HDC hDC);


Curve::Curve(float _ax,float _ay,float _bx,float _by,float _cx,float _cy,int _nDiv) 
	{
	ax = _ax;
	ay = _ay;
	bx = _bx;
	by = _by;
	cx = _cx;
	cy = _cy;
	nDiv = _nDiv;
	}

Curve::Curve(float _ax,float _ay,float _bx,float _by,float _cx,float _cy)
	{
	ax = _ax; 
	ay = _ay;
	bx = _bx; 
	by = _by;
	cx = _cx; 
	cy = _cy;
	nDiv = (int)(max(abs((int)ax),abs((int)ay))/DIV_FACTOR);
	}


void Curve::PutCurve(float _ax,float _ay,float _bx,float _by,float _cx,float _cy) 
	{
	ax = _ax;
	ay = _ay;
	bx = _bx; 
	by = _by;
	cx = _cx; 
	cy = _cy;
	nDiv = (int)(max(abs((int)ax),abs((int)ay))/DIV_FACTOR);
	}

void Curve::Draw( HDC hdc, float x, float y) 
	{
	int origX,origY,newX,newY;
	float  t,f,g,h;
	if (nDiv==0)
		nDiv=1;

	origX = (int)x; 
	origY= (int)y;
	for(int i=1; i <= nDiv ; i++)
	   {
		t = 1.0f / (float)nDiv * (float)i;
		f = t*t*(3.0f-2.0f*t);
	   g = t*(t-1.0f)*(t-1.0f);
		h = t*t*(t-1.0f);
		newX = (int)(x + ax*f + bx*g + cx*h);
		newY = (int)(y + ay*f + by*g + cy*h);
		MoveToEx(hdc, origX, origY, NULL);
		LineTo(hdc, newX, newY);
		origX = newX;  
		origY=newY;
	   }
	}

int Curve::GetCount()
   {
	if ( nDiv==0 )
		nDiv=1;

	int pointCount = 1;

	for(int i=1; i <= nDiv ; i++)
		pointCount++;

	return pointCount;
	  }


void Curve::GetCurve(float x,float y, POINT points[], int& pointCount)
	{
	int ix,iy;
	float  t,f,g,h;

	if (nDiv==0)
		nDiv=1;

	ix = (int)x; 
	iy = (int)y;

   points[pointCount].x = ix;
	points[pointCount].y = iy;
	pointCount++;

	for(int i=1; i<=nDiv ; i++)
		{
		t = 1.0f / (float)nDiv * (float)i;
		f = t*t*(3.0f-2.0f*t);
		g = t*(t-1.0f)*(t-1.0f);
		h = t*t*(t-1.0f);
		ix = (int)(x + ax*f + bx*g + cx*h);
		iy = (int)(y + ay*f + by*g + cy*h);
		points[pointCount].x = ix;
		points[pointCount].y = iy;
		pointCount++;
	   }
	}
  


CubicSpline::CubicSpline(POINT pt[], int np)
	{
	numPoints = np;
	px = new float[numPoints];
	py = new float[numPoints];
	ax = new float[numPoints];
	ay = new float[numPoints];
	bx = new float[numPoints];
	by = new float[numPoints];
	cx = new float[numPoints];
	cy = new float[numPoints];
	k = new float[numPoints];
	mat[0] = new float[numPoints];
	mat[1] = new float[numPoints];
	mat[2] = new float[numPoints];

	for(int i=0;i<numPoints ;i++) 
	   {
		px[i] = (float)pt[i].x;  
		py[i] = (float)pt[i].y;
		}
	}


CubicSpline::	CubicSpline(float _px[] , float _py[] , int np) 
	{
	numPoints = np;
	px = new float[numPoints];
	py = new float[numPoints];
	ax = new float[numPoints];
	ay = new float[numPoints];
	bx = new float[numPoints];
	by = new float[numPoints];
	cx = new float[numPoints];
	cy = new float[numPoints];
	k = new float[numPoints];
	mat[0] = new float[numPoints];
	mat[1] = new float[numPoints];
	mat[2] = new float[numPoints];

	for(int i=0;i<numPoints ;i++) 
	   {
		px[i] = _px[i];  
		py[i] = _py[i];
		}
	}
	

CubicSpline::~CubicSpline()
   {
	delete[] px;
	delete[] py;
	delete[] ax;
	delete[] ay;
	delete[] bx;
	delete[] by;
	delete[] cx;
	delete[] cy;
	delete[] k;
	delete[] mat[0];
	delete[] mat[1];
	delete[] mat[2];
	}

void CubicSpline::Generate() 
	{
	float AMag , AMagOld;
   // vector A
   int i;
	for(i= 0 ; i<=numPoints-2 ; i++ ) 
		{
	   ax[i] = px[i+1] - px[i];
		ay[i] = py[i+1] - py[i];
		}
	// k
	AMagOld = (float)sqrt(ax[0]*ax[0] + ay[0]*ay[0]);
	for(i=0 ; i<=numPoints-3 ; i++) 
		{
		AMag = (float)sqrt(ax[i+1]*ax[i+1] + ay[i+1]*ay[i+1]);
		k[i] = AMagOld / AMag;
		AMagOld = AMag;
		}
	k[numPoints-2] = 1.0f;

	// Matrix
	for(i=1; i<=numPoints-2;i++) 
		{
		mat[0][i] = 1.0f;
		mat[1][i] = 2.0f*k[i-1]*(1.0f + k[i-1]);
		mat[2][i] = k[i-1]*k[i-1]*k[i];
		}
	mat[1][0] = 2.0f;
	mat[2][0] = k[0];
	mat[0][numPoints-1] = 1.0f;
	mat[1][numPoints-1] = 2.0f*k[numPoints-2];

	// 
	for(i=1; i<=numPoints-2;i++) 
		{
		bx[i] = 3.0f*(ax[i-1] + k[i-1]*k[i-1]*ax[i]);
		by[i] = 3.0f*(ay[i-1] + k[i-1]*k[i-1]*ay[i]);
		}
	bx[0] = 3.0f*ax[0];
	by[0] = 3.0f*ay[0];
	bx[numPoints-1] = 3.0f*ax[numPoints-2];
	by[numPoints-1] = 3.0f*ay[numPoints-2];

	//
	MatrixSolve(bx);
	MatrixSolve(by);

	for(i=0 ; i<=numPoints-2 ; i++ ) 
	   {
	   cx[i] = k[i]*bx[i+1];
		cy[i] = k[i]*by[i+1];
		}
	}

void CubicSpline::MatrixSolve(float B[]) 
	{
	float* Work = new float[numPoints];
	float* WorkB = new float[numPoints];
   int i;
	for(i=0;i<=numPoints-1;i++) 
		{
		Work[i] = B[i] / mat[1][i];
		WorkB[i] = Work[i];
		}

	for(int j=0 ; j<10 ; j++) 
		{ ///  need convergence judge
		Work[0] = (B[0] - mat[2][0]*WorkB[1])/mat[1][0];
		for(i=1; i<numPoints-1 ; i++ ) 
			{
			Work[i] = (B[i]-mat[0][i]*WorkB[i-1]-mat[2][i]*WorkB[i+1])
							/mat[1][i];
			}

      Work[numPoints-1] = (B[numPoints-1] - mat[0][numPoints-1]*WorkB[numPoints-2])/mat[1][numPoints-1];

		for(i=0 ; i<=numPoints-1 ; i++ ) 
			WorkB[i] = Work[i];
		}

   for(i=0 ; i<=numPoints-1 ; i++ ) 
	  	{
	   B[i] = Work[i];
	   }
	delete[] Work;
	delete[] WorkB;
	}


void CubicSpline::Draw(HDC hdc) 
	{
	Curve c;
	for(int i=0; i<numPoints-1 ; i++) 
		{
		c.PutCurve(ax[i],ay[i],bx[i],by[i],cx[i],cy[i]);
		c.Draw(hdc,px[i],py[i]);
		}		
	}


int CubicSpline::GetCurveCount()
	{
	Curve c;
	int count = 0;
	for(int i=0; i<numPoints-1 ; i++) 
		{
		c.PutCurve(ax[i],ay[i],bx[i],by[i],cx[i],cy[i]);
		count += c.GetCount();
		}
	return count;
	}


void CubicSpline::GetCurve(POINT points[], int& PointCount)
	{
	Curve c;
	for(int i=0; i<numPoints-1 ; i++) 
	   {
		c.PutCurve(ax[i],ay[i],bx[i],by[i],cx[i],cy[i]);
		c.GetCurve(px[i],py[i], points, PointCount);
		}
	}


//////////// closed cubic spline ////////////////////
void CubicSpline::GenClosed() 
	{
	float AMag , AMagOld , AMag0;
   // vector A
   int i;
	for(i= 0 ; i<=numPoints-2 ; i++ ) 
		{
		ax[i] = px[i+1] - px[i];
		ay[i] = py[i+1] - py[i];
		}
	ax[numPoints-1] = px[0] - px[numPoints-1];
	ay[numPoints-1] = py[0] - py[numPoints-1];

	// k
	AMag0 = AMagOld = (float)sqrt(ax[0]*ax[0] + ay[0]*ay[0]);
	for(i=0 ; i<=numPoints-2 ; i++) 
		{
		AMag = (float)sqrt(ax[i+1]*ax[i+1] + ay[i+1]*ay[i+1]);
		k[i] = AMagOld / AMag;
		AMagOld = AMag;
		}
	k[numPoints-1]=AMagOld/AMag0; 

	// Matrix
	for(i=1; i<=numPoints-1;i++) 
		{
		mat[0][i] = 1.0f;
		mat[1][1] = 2.0f*k[i-1]*(1.0f + k[i-1]);
		mat[2][i] = k[i-1]*k[i-1]*k[i];
		}
	mat[0][0] = 1.0f;
	mat[1][0] = 2.0f*k[numPoints-1]*(1.0f + k[numPoints-1]);
	mat[2][0] = k[numPoints-1]*k[numPoints-1]*k[0];

	// 
	for(i=1; i<=numPoints-1;i++) 
		{
		bx[i] = 3.0f*(ax[i-1] + k[i-1]*k[i-1]*ax[i]);
		by[i] = 3.0f*(ay[i-1] + k[i-1]*k[i-1]*ay[i]);
		}
	bx[0] = 3.0f*(ax[numPoints-1] + k[numPoints-1]*k[numPoints-1]*ax[0]);
	by[0] = 3.0f*(ay[numPoints-1] + k[numPoints-1]*k[numPoints-1]*ay[0]);

	//
	MatrixSolveEX(bx);
	MatrixSolveEX(by);

	for(i=0 ; i<=numPoints-2 ; i++ ) 
		{
		cx[i] = k[i]*bx[i+1];
		cy[i] = k[i]*by[i+1];
		}

	cx[numPoints-1] = k[numPoints-1]*bx[0];
	cy[numPoints-1] = k[numPoints-1]*by[0];
	}

///// tridiagonal matrix + elements of [0][0], [N-1][N-1] //// 
void CubicSpline::MatrixSolveEX(float B[]) 
	{
	float* Work = new float[numPoints];
	float* WorkB = new float[numPoints];
     int i;

	for(i=0;i<=numPoints-1;i++) 
		{
		Work[i] = B[i] / mat[1][i];
		WorkB[i] = Work[i];
		}

	for(int j=0 ; j<10 ; j++) 
		{  // need judge of convergence
		Work[0] = (B[0]-mat[0][0]*WorkB[numPoints-1]-mat[2][0]*WorkB[1])
					/mat[1][0];

		for( i=1; i < numPoints-1 ; i++ ) 
			Work[i] = (B[i]-mat[0][i]*WorkB[i-1]-mat[2][i]*WorkB[i+1]) /mat[1][i];
	
      Work[numPoints-1] = (B[numPoints-1]-mat[0][numPoints-1]*WorkB[numPoints-2]-mat[2][numPoints-1]*WorkB[0])
							/mat[1][numPoints-1];

		for(i=0 ; i<=numPoints-1 ; i++ ) 
			WorkB[i] = Work[i];
		}

	for( i=0 ; i <= numPoints-1 ; i++ ) 
		B[i] = Work[i];
	}

void CubicSpline::DrawClosed(HDC hdc) 
	{
	Curve c;
	for(int i=0; i<numPoints ; i++) 
		{
		c.PutCurve(ax[i],ay[i],bx[i],by[i],cx[i],cy[i]);
		c.Draw(hdc ,px[i],py[i]);
		}
	}


double Distance(const POINT& p1, const POINT& p2)
   {
	int dx = abs(p1.x - p2.x);
	int dy = abs(p1.y - p2.y);

	return sqrt(double(dx*dx + dy*dy));

   }

void DrawCross(POINT point, COLORREF rgbColor, int extent, HDC hDC)
   {
	HPEN hPen,oldPen;
	hPen = CreatePen(PS_SOLID,1,rgbColor);
   oldPen = (HPEN)SelectObject(hDC,hPen);
	
	MoveToEx(hDC,point.x - extent, point.y,NULL);
	LineTo(hDC, point.x + extent, point.y);
    
	MoveToEx(hDC, point.x, point.y - extent,NULL);
   LineTo(hDC,point.x, point.y + extent);
	SelectObject(hDC,oldPen);
	DeleteObject(hPen);
   }



// CubicSplineWnd

IMPLEMENT_DYNAMIC(CubicSplineWnd, CWnd)

CubicSplineWnd::CubicSplineWnd()
   : m_moveIndex( -1 )
   , m_allowAddPoints( true )
{ }


CubicSplineWnd::~CubicSplineWnd()
{ }


BEGIN_MESSAGE_MAP(CubicSplineWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

// CubicSplineWnd message handlers


void CubicSplineWnd::OnPaint() 
   {
	CWnd::OnPaint();

   CDC *pDC = GetDC();
   this->Update( pDC->m_hDC );
   ReleaseDC( pDC );   
   }

void CubicSplineWnd::OnLButtonDown(UINT nFlags, CPoint point) 
   {
	//check if the mouse left clicking into one of the control points
	int index = IsInsideControlPoint(point);
	
	if( index >= 0 )
   	{
		//we are moving a control point around
		m_moveIndex = index;
	   }

	else //we are adding control points
	   {
      if ( m_allowAddPoints )
         {
		   CDC *pDC = GetDC();
		   DrawCross(point, RGB(255,0,0), 4, pDC->m_hDC);
		   m_controlPoints.Add(point);
		   if( m_controlPoints.GetSize() > 1 )
		      {
			   //create a spline object
			   CubicSpline spline(m_controlPoints.GetData(), (int) m_controlPoints.GetSize());
   			
            //generate a curve
			   spline.Generate();

			   //get the curve points number
			   m_curvePoints.SetSize( spline.GetCurveCount() );

			   //get the points number
			   int pointCount = 0;
			   spline.GetCurve(m_curvePoints.GetData(), pointCount);

			   //paint the curve
			   Update( pDC->m_hDC );
	   	   }

   	   ReleaseDC(pDC);
         }
	   }

	CWnd::OnLButtonDown(nFlags, point);
   }


void CubicSplineWnd::OnMouseMove(UINT nFlags, CPoint point) 
   {
	// TODO: Add your message handler code here and/or call default
	if( m_moveIndex >= 0 )
	   {
		m_controlPoints[m_moveIndex] = point;

		if( m_controlPoints.GetSize()>1 )
		   {
			CubicSpline spline(m_controlPoints.GetData(), (int) m_controlPoints.GetSize());
			spline.Generate();
	
			m_curvePoints.SetSize(spline.GetCurveCount());
			int pointCount = 0;
			spline.GetCurve( m_curvePoints.GetData(), pointCount);
		   }

		CDC* pDC = GetDC();
		Update( pDC->m_hDC );
		ReleaseDC( pDC );
	   }

	CWnd::OnMouseMove(nFlags, point);
   }


void CubicSplineWnd::OnLButtonUp(UINT nFlags, CPoint point) 
   {
	m_moveIndex = -1;
	CWnd::OnLButtonUp(nFlags, point);
   }


void CubicSplineWnd::Update(HDC& hDC)
   {
	//brush the DC
	RECT rect;
	GetClientRect(&rect);
	FillRect(hDC, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

	//draw the control points
	for( int i=0; i < m_controlPoints.GetSize(); i++ )
		DrawCross( m_controlPoints[i], RGB(255,0,0), 4, hDC );

	//draw the curve
	if( m_curvePoints.GetSize() > 1 )
		Polyline(hDC, m_curvePoints.GetData(), (int) m_curvePoints.GetSize());
	}


int CubicSplineWnd::IsInsideControlPoint( POINT& point )
   {
	int count = (int) m_controlPoints.GetSize();

	if ( count > 0 )	
	   { 
		for( int i=0; i < count; i++ )
		   {
			if( Distance( m_controlPoints[i], point) < 5 )
				return i;
		   }
	   }

   return -1;
   }
