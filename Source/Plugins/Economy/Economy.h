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

#include <EnvExtension.h>
#include <Maplayer.h>
#include <MapExprEngine.h>
#include <Policy.h>
#include <randgen\Randunif.hpp>

using namespace std;



//! Defines the initialization routines for the DLL. Economy loads 
//! an .xml document (that defines a set of decision rules) and uses
//! the class MapExprEngine to evaluate the string expressions that
//! are specified in the .xml document. Each decision rule is
//! connected to a policy. The decision rules (i.e. the corresponding
//! policy) that results in the highest value, is applied and the 
//! corresponding policy id is written into a column of the IDU table.

class Economy : public EnvModelProcess
{
public:
   //! Default constructor, initialize member variables.
   //! Initialize the member variables m_pIDULayer, m_pExpr, 
   //! m_pMapExprEngineUtilities and m_pPolicy with NULL.
   Economy( void );
   //! Default destructor, free memory.
   //! Free memory dynamically allocated to the member
   //! variable m_pMapExprEngineUtilities.
   ~Economy( void );

protected:
   //! MapLayer object, to hold values of IDU.shp.
   MapLayer *m_pIDULayer;
   //! MapExpr object, to handle string expressions derived from .xml file.
   MapExpr *m_pExpr;
   //! MapExprEngine object, to evaluate expressions derived from .xml file.
   MapExprEngine *m_pMapExprEngine;
   //! Policy object, to handle policies specified in the policies .xml file.
   //Policy *m_pPolicy;
   //! Strings for error handling.
   CString msg, failureID;
   LPCTSTR m_initStrUtil;
   vector<string> m_path;

   CString m_utilityIDCol;
   int m_colUtilityID;
   
   CString m_mode;

   CArray< float, float > m_values;    // values for the expression results for a given IDU
   RandUniform m_rndUnif;

   //! Read xml file for utilites.
   //! Load xml file specified in filename, iterate through nodes <utility> and
   //! get for each utility a name (string), equation (string) and policy ID (int). 
   //! Read for each node, via the policy ID, the corresponding policy. Derive the 
   //! spatial query for applicability of policy (and hence the range for the equation).
   //! Call m_pMapExprEngineUtilities->AddExpression(), with the arguments: name, equation and
   //! spatial query.
   bool LoadXml( LPCTSTR filename );
   //! Evaluate the expressions and write policy ID to IDU.shp column.
   //! Iterate through IDU's and: use equations get maximum value and to 
   //! calculate their results and choose the one with the max result to 
   //! derive the corresponding policy and write the policy ID in a column 
   //! (UTILITYID) of the IDU.shp.
   bool EvaluateUtilities( EnvContext *pContext, bool useAddDelta );
   int EvaluateUtilitiesDeterministic( MapExprEngine *pContext );
   int EvaluateUtilitiesStochastic( MapExprEngine *pContext );

public:
   //! Initialize and carry out procedure to generate baseyear conditions.
   //! Get IDU.shp MapLayer, request dynamic memory for m_pMapExprEngine,
   //! call LoadXml() to read the expressions from xml file , compile 
   //! expressions derived from xml file and call Evaluate().
   bool Init( EnvContext *pContext, LPCTSTR initStr );
   //! Carry out procedure by calling Evaluate().
   bool Run( EnvContext *pContext );
};

