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
//
// MODULE: Ado2.cpp
//
// AUTHOR: Carlos Antollini <cantollini@hotmail.com>
//
// Copyright (c) 2001-2004. All Rights Reserved.
//
// Date: August 01, 2005
//
// Version 2.20
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name and all copyright 
// notices remains intact. 
//
// An email letting me know how you are using it would be nice as well. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage/loss of business that
// this product may cause.
//
//
//////////////////////////////////////////////////////////////////////
#include "libs.h"
#pragma hdrstop

#include "ado2.h"


#ifdef _WINDOWS

#define ChunkSize 100

/*
///////////////////////////////////////////////////////
//
// CJetEngine Class
//

BOOL CJetEngine::CompactDatabase(CString strDatabaseSource, CString strDatabaseDestination)
{
	try
	{
		::CoInitialize(NULL);
		IJetEnginePtr jet(__uuidof(JetEngine));		
		HRESULT hr = jet->CompactDatabase(_bstr_t(strDatabaseSource.GetBuffer(0)), _bstr_t(strDatabaseDestination.GetBuffer(0)));
		jet.Release();
		::CoUninitialize();
		return hr == S_OK;
	}
	catch(_com_error) 
	{       
		::CoUninitialize();
		return FALSE;
	} 
}

BOOL CJetEngine::RefreshCache(ADODB::_Connection *pconn)
{
	//Added by doodle@email.it
	try
	{
		::CoInitialize(NULL);
		IJetEnginePtr jet(__uuidof(JetEngine)); 
		HRESULT hr = jet->RefreshCache(pconn);
		::CoUninitialize();
		return hr == S_OK;
	}
	catch(_com_error) 
	{ 
		::CoUninitialize();
		return FALSE;
	} 
	
	return FALSE;
}
*/

///////////////////////////////////////////////////////
//
// CADODatabase Class
//

DWORD CADODatabase::GetRecordCount(_RecordsetPtr m_pRs)
{
	DWORD numRows = 0;
	
	numRows = (DWORD) m_pRs->GetRecordCount();

	if(numRows == -1)
	{
		if(m_pRs->EndOfFile != VARIANT_TRUE)
			m_pRs->MoveFirst();

		while(m_pRs->EndOfFile != VARIANT_TRUE)
		{
			numRows++;
			m_pRs->MoveNext();
		}
		if(numRows > 0)
			m_pRs->MoveFirst();
	}
	return numRows;
}

BOOL CADODatabase::Open(LPCTSTR lpstrConnection, LPCTSTR lpstrUserID, LPCTSTR lpstrPassword)
{
	HRESULT hr = S_OK;

	if(IsOpen())
		Close();

	if(strcmp(lpstrConnection, _T("")) != 0)
		m_strConnection = lpstrConnection;

	ASSERT(!m_strConnection.IsEmpty());

	try
	{
		if(m_nConnectionTimeout != 0)
			m_pConnection->PutConnectionTimeout(m_nConnectionTimeout);
		hr = m_pConnection->Open(_bstr_t(m_strConnection), _bstr_t(lpstrUserID), _bstr_t(lpstrPassword), NULL);
		return hr == S_OK;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
	
}

void CADODatabase::dump_com_error(_com_error &e)
{
	CString ErrorStr;
	
	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	ErrorStr.Format( "CADODataBase Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n",
		e.Error(), e.ErrorMessage(), (LPCSTR)bstrSource, (LPCSTR)bstrDescription);
	m_strErrorDescription = (LPCSTR)bstrDescription ;
	m_strLastError = _T("Connection String = " + GetConnectionString() + '\n' + ErrorStr);
	m_dwLastError = e.Error(); 
	#ifdef _DEBUG
		AfxMessageBox(ErrorStr, MB_OK | MB_ICONERROR );
	#endif
	throw CADOException(e.Error(), e.Description());
}

BOOL CADODatabase::IsOpen()
{
	if(m_pConnection )
		return m_pConnection->GetState() != adStateClosed;
	return FALSE;
}

void CADODatabase::Close()
{
	if(IsOpen())
		m_pConnection->Close();
}


///////////////////////////////////////////////////////
//
// CADORecordset Class
//

CADORecordset::CADORecordset()
{
	m_pRecordset = NULL;
	m_pCmd = NULL;
	m_strQuery = _T("");
	m_strLastError = _T("");
	m_dwLastError = 0;
	m_pRecBinding = NULL;
	m_pRecordset.CreateInstance(__uuidof(Recordset));
	m_pCmd.CreateInstance(__uuidof(Command));
	m_nEditStatus = CADORecordset::dbEditNone;
	m_nSearchDirection = CADORecordset::searchForward;
}

CADORecordset::CADORecordset(CADODatabase* pAdoDatabase)
{
	m_pRecordset = NULL;
	m_pCmd = NULL;
	m_strQuery = _T("");
	m_strLastError = _T("");
	m_dwLastError = 0;
	m_pRecBinding = NULL;
	m_pRecordset.CreateInstance(__uuidof(Recordset));
	m_pCmd.CreateInstance(__uuidof(Command));
	m_nEditStatus = CADORecordset::dbEditNone;
	m_nSearchDirection = CADORecordset::searchForward;

	m_pConnection = pAdoDatabase->GetActiveConnection();
}

BOOL CADORecordset::Open(_ConnectionPtr mpdb, LPCTSTR lpstrExec, int nOption)
{	
	Close();
	
	if(strcmp(lpstrExec, _T("")) != 0)
		m_strQuery = lpstrExec;

	ASSERT(!m_strQuery.IsEmpty());

	if(m_pConnection == NULL)
		m_pConnection = mpdb;

	m_strQuery.TrimLeft();
	BOOL bIsSelect = m_strQuery.Mid(0, strlen("Select ")).CompareNoCase("select ") == 0 && nOption == openUnknown;

	try
	{
		m_pRecordset->CursorType = adOpenStatic;
		m_pRecordset->CursorLocation = adUseClient;
		if(bIsSelect || nOption == openQuery || nOption == openUnknown)
			m_pRecordset->Open((LPCSTR)m_strQuery, _variant_t((IDispatch*)mpdb, TRUE), 
							adOpenStatic, adLockOptimistic, adCmdUnknown);
		else if(nOption == openTable)
			m_pRecordset->Open((LPCSTR)m_strQuery, _variant_t((IDispatch*)mpdb, TRUE), 
							adOpenKeyset, adLockOptimistic, adCmdTable);
		else if(nOption == openStoredProc)
		{
			m_pCmd->ActiveConnection = mpdb;
			m_pCmd->CommandText = _bstr_t(m_strQuery);
			m_pCmd->CommandType = adCmdStoredProc;
			m_pConnection->CursorLocation = adUseClient;
			
			m_pRecordset = m_pCmd->Execute(NULL, NULL, adCmdText);
		}
		else
		{
			TRACE( "Unknown parameter. %d", nOption);
			return FALSE;
		}
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}

	return m_pRecordset != NULL && m_pRecordset->GetState()!= adStateClosed;
}

BOOL CADORecordset::Open(LPCTSTR lpstrExec, int nOption)
{
	ASSERT(m_pConnection != NULL);
	ASSERT(m_pConnection->GetState() != adStateClosed);
	return Open(m_pConnection, lpstrExec, nOption);
}

BOOL CADORecordset::OpenSchema(int nSchema, LPCTSTR SchemaID /*= _T("")*/)
{
	try
	{
		_variant_t vtSchemaID = vtMissing;

		if(strlen(SchemaID) != 0)
		{
			vtSchemaID = SchemaID;
			nSchema = adSchemaProviderSpecific;
		}
			
		m_pRecordset = m_pConnection->OpenSchema((enum SchemaEnum)nSchema, vtMissing, vtSchemaID);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::Requery()
{
	if(IsOpen())
	{
		try
		{
			m_pRecordset->Requery(adExecuteRecord);
		}
		catch(_com_error &e)
		{
			dump_com_error(e);
			return FALSE;
		}
	}
	return TRUE;
}


BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, double& dbValue)
{	
	double val = (double)NULL;
	_variant_t vtFld;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch(vtFld.vt)
		{
		case VT_R4:
			val = vtFld.fltVal;
			break;
		case VT_R8:
			val = vtFld.dblVal;
			break;
		case VT_DECIMAL:
			//Corrected by Jos� Carlos Mart�nez Gal�n
			val = vtFld.decVal.Lo32;
			val *= (vtFld.decVal.sign == 128)? -1 : 1;
			val /= pow(10.0f, (int) vtFld.decVal.scale); 
			break;
		case VT_UI1:
			val = vtFld.iVal;
			break;
		case VT_I2:
		case VT_I4:
			val = vtFld.lVal;
			break;
		case VT_INT:
			val = vtFld.intVal;
			break;
		case VT_CY:   //Added by John Andy Johnson!!!!
			vtFld.ChangeType(VT_R8);
			val = vtFld.dblVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			val = 0;
			break;
		default:
			val = vtFld.dblVal;
		}
		dbValue = val;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


BOOL CADORecordset::GetFieldValue(int nIndex, double& dbValue)
{	
	double val = (double)NULL;
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch(vtFld.vt)
		{
		case VT_R4:
			val = vtFld.fltVal;
			break;
		case VT_R8:
			val = vtFld.dblVal;
			break;
		case VT_DECIMAL:
			//Corrected by Jos� Carlos Mart�nez Gal�n
			val = vtFld.decVal.Lo32;
			val *= (vtFld.decVal.sign == 128)? -1 : 1;
			val /= pow(10.0f, (int) vtFld.decVal.scale); 
			break;
		case VT_UI1:
			val = vtFld.iVal;
			break;
		case VT_I2:
		case VT_I4:
			val = vtFld.lVal;
			break;
		case VT_INT:
			val = vtFld.intVal;
			break;
		case VT_CY:   //Added by John Andy Johnson!!!!
			vtFld.ChangeType(VT_R8);
			val = vtFld.dblVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			val = 0;
			break;
		default:
			val = 0;
		}
		dbValue = val;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, long& lValue)
{
	long val = (long)NULL;
	_variant_t vtFld;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		if(vtFld.vt != VT_NULL && vtFld.vt != VT_EMPTY)
			val = vtFld.lVal;
		lValue = val;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, long& lValue)
{
	long val = (long)NULL;
	_variant_t vtFld;
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		if(vtFld.vt != VT_NULL && vtFld.vt != VT_EMPTY)
			val = vtFld.lVal;
		lValue = val;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, unsigned long& ulValue)
{
	long val = (long)NULL;
	_variant_t vtFld;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		if(vtFld.vt != VT_NULL && vtFld.vt != VT_EMPTY)
			val = vtFld.ulVal;
		ulValue = val;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, unsigned long& ulValue)
{
	long val = (long)NULL;
	_variant_t vtFld;
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		if(vtFld.vt != VT_NULL && vtFld.vt != VT_EMPTY)
			val = vtFld.ulVal;
		ulValue = val;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, int& nValue)
{
	int val = NULL;
	_variant_t vtFld;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch(vtFld.vt)
		{
		case VT_BOOL:
			val = vtFld.boolVal;
			break;
		case VT_I2:
		case VT_UI1:
			val = vtFld.iVal;
			break;
		case VT_INT:
			val = vtFld.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			val = 0;
			break;
		default:
			val = vtFld.iVal;
		}	
		nValue = val;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, int& nValue)
{
	int val = (int)NULL;
	_variant_t vtFld;
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch(vtFld.vt)
		{
		case VT_BOOL:
			val = vtFld.boolVal;
			break;
		case VT_I2:
		case VT_UI1:
			val = vtFld.iVal;
			break;
		case VT_INT:
			val = vtFld.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			val = 0;
			break;
		default:
			val = vtFld.iVal;
		}	
		nValue = val;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, CString& strValue, CString strDateFormat)
{
	CString str;
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch(vtFld.vt) 
		{
		case VT_R4:
			str = DblToStr(vtFld.fltVal);
			break;
		case VT_R8:
			str = DblToStr(vtFld.dblVal);
			break;
		case VT_BSTR:
			str = vtFld.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			str = IntToStr(vtFld.iVal);
			break;
		case VT_INT:
			str = IntToStr(vtFld.intVal);
			break;
		case VT_I4:
			str = LongToStr(vtFld.lVal);
			break;
		case VT_UI4:
			str = ULongToStr(vtFld.ulVal);
			break;
		case VT_DECIMAL:
			{
			//Corrected by Jos� Carlos Mart�nez Gal�n
			double val = vtFld.decVal.Lo32;
			val *= (vtFld.decVal.sign == 128)? -1 : 1;
			val /= pow(10.0f, (int) vtFld.decVal.scale); 
			str = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtFld);

				if(strDateFormat.IsEmpty())
					strDateFormat = _T("%Y-%m-%d %H:%M:%S");
				str = dt.Format(strDateFormat);
			}
			break;
		case VT_CY:		//Added by John Andy Johnson!!!
			{
				vtFld.ChangeType(VT_R8);
 
				CString str;
				str.Format(_T("%f"), vtFld.dblVal);
 
				_TCHAR pszFormattedNumber[64];
 
				//	LOCALE_USER_DEFAULT
				if(GetCurrencyFormat(
						LOCALE_USER_DEFAULT,	// locale for which string is to be formatted 
						0,						// bit flag that controls the function's operation
						str,					// pointer to input number string
						NULL,					// pointer to a formatting information structure
												//	NULL = machine default locale settings
						pszFormattedNumber,		// pointer to output buffer
						63))					// size of output buffer
				{
					str = pszFormattedNumber;
				}
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			str.Empty();
			break;
		case VT_BOOL:
			str = vtFld.boolVal == VARIANT_TRUE? 'T':'F';
			break;
		default:
			str.Empty();
			return FALSE;
		}
		strValue = str;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, CString& strValue, CString strDateFormat)
{
	CString str;
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch(vtFld.vt) 
		{
		case VT_R4:
			str = DblToStr(vtFld.fltVal);
			break;
		case VT_R8:
			str = DblToStr(vtFld.dblVal);
			break;
		case VT_BSTR:
			str = vtFld.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			str = IntToStr(vtFld.iVal);
			break;
		case VT_INT:
			str = IntToStr(vtFld.intVal);
			break;
		case VT_I4:
			str = LongToStr(vtFld.lVal);
			break;
		case VT_UI4:
			str = ULongToStr(vtFld.ulVal);
			break;
		case VT_DECIMAL:
			{
			//Corrected by Jos� Carlos Mart�nez Gal�n
			double val = vtFld.decVal.Lo32;
			val *= (vtFld.decVal.sign == 128)? -1 : 1;
			val /= pow(10.0f, (int) vtFld.decVal.scale); 
			str = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtFld);
				
				if(strDateFormat.IsEmpty())
					strDateFormat = _T("%Y-%m-%d %H:%M:%S");
				str = dt.Format(strDateFormat);
			}
			break;
		case VT_CY:		//Added by John Andy Johnson!!!
			{
				vtFld.ChangeType(VT_R8);
 
				CString str;
				str.Format(_T("%f"), vtFld.dblVal);
 
				_TCHAR pszFormattedNumber[64];
 
				//	LOCALE_USER_DEFAULT
				if(GetCurrencyFormat(
						LOCALE_USER_DEFAULT,	// locale for which string is to be formatted 
						0,						// bit flag that controls the function's operation
						str,					// pointer to input number string
						NULL,					// pointer to a formatting information structure
												//	NULL = machine default locale settings
						pszFormattedNumber,		// pointer to output buffer
						63))					// size of output buffer
				{
					str = pszFormattedNumber;
				}
			}
			break;
		case VT_BOOL:
			str = vtFld.boolVal == VARIANT_TRUE? 'T':'F';
			break;
		case VT_EMPTY:
		case VT_NULL:
			str.Empty();
			break;
		default:
			str.Empty();
			return FALSE;
		}
		strValue = str;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, COleDateTime& time)
{
	_variant_t vtFld;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch(vtFld.vt) 
		{
		case VT_DATE:
			{
				COleDateTime dt(vtFld);
				time = dt;
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			time.SetStatus(COleDateTime::null);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, COleDateTime& time)
{
	_variant_t vtFld;
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch(vtFld.vt) 
		{
		case VT_DATE:
			{
				COleDateTime dt(vtFld);
				time = dt;
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			time.SetStatus(COleDateTime::null);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, bool& bValue)
{
	_variant_t vtFld;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch(vtFld.vt) 
		{
		case VT_BOOL:
			bValue = vtFld.boolVal == VARIANT_TRUE? true: false;
			break;
		case VT_EMPTY:
		case VT_NULL:
			bValue = false;
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, bool& bValue)
{
	_variant_t vtFld;
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch(vtFld.vt) 
		{
		case VT_BOOL:
			bValue = vtFld.boolVal == VARIANT_TRUE? true: false;
			break;
		case VT_EMPTY:
		case VT_NULL:
			bValue = false;
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, COleCurrency& cyValue)
{
	_variant_t vtFld;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch(vtFld.vt) 
		{
		case VT_CY:
			cyValue = (CURRENCY)vtFld.cyVal;
			break;
		case VT_EMPTY:
		case VT_NULL:
			{
			cyValue = COleCurrency();
			cyValue.m_status = COleCurrency::null;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, COleCurrency& cyValue)
{
	_variant_t vtFld;
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch(vtFld.vt) 
		{
		case VT_CY:
			cyValue = (CURRENCY)vtFld.cyVal;
			break;
		case VT_EMPTY:
		case VT_NULL:
			{
			cyValue = COleCurrency();
			cyValue.m_status = COleCurrency::null;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, _variant_t& vtValue)
{
	try
	{
		vtValue = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, _variant_t& vtValue)
{
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	try
	{
		vtValue = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::IsFieldNull(LPCTSTR lpFieldName)
{
	_variant_t vtFld;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		return vtFld.vt == VT_NULL;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::IsFieldNull(int nIndex)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		return vtFld.vt == VT_NULL;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::IsFieldEmpty(LPCTSTR lpFieldName)
{
	_variant_t vtFld;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		return vtFld.vt == VT_EMPTY || vtFld.vt == VT_NULL;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::IsFieldEmpty(int nIndex)
{
	_variant_t vtFld;
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		return vtFld.vt == VT_EMPTY || vtFld.vt == VT_NULL;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::SetFieldEmpty(LPCTSTR lpFieldName)
{
	_variant_t vtFld;
	vtFld.vt = VT_EMPTY;
	
	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldEmpty(int nIndex)
{
	_variant_t vtFld;
	vtFld.vt = VT_EMPTY;

	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	return PutFieldValue(vtIndex, vtFld);
}


DWORD CADORecordset::GetRecordCount()
{
	DWORD nRows = 0;
	
	nRows = (DWORD) m_pRecordset->GetRecordCount();

	if(nRows == -1)
	{
		nRows = 0;
		if(m_pRecordset->EndOfFile != VARIANT_TRUE)
			m_pRecordset->MoveFirst();
		
		while(m_pRecordset->EndOfFile != VARIANT_TRUE)
		{
			nRows++;
			m_pRecordset->MoveNext();
		}
		if(nRows > 0)
			m_pRecordset->MoveFirst();
	}
	
	return nRows;
}

BOOL CADORecordset::IsOpen()
{
	if(m_pRecordset != NULL && IsConnectionOpen())
		return m_pRecordset->GetState() != adStateClosed;
	return FALSE;
}

void CADORecordset::Close()
{
	if(IsOpen())
	{
		if (m_nEditStatus != dbEditNone)
		      CancelUpdate();

		m_pRecordset->PutSort(_T(""));
		m_pRecordset->Close();	
	}
}


BOOL CADODatabase::Execute(LPCTSTR lpstrExec)
{
	ASSERT(m_pConnection != NULL);
	ASSERT(strcmp(lpstrExec, _T("")) != 0);
	_variant_t vRecords;
	
	m_nRecordsAffected = 0;

	try
	{
		m_pConnection->CursorLocation = adUseClient;
		m_pConnection->Execute(_bstr_t(lpstrExec), &vRecords, adExecuteNoRecords);
		m_nRecordsAffected = vRecords.iVal;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;	
	}
}

BOOL CADORecordset::RecordBinding(CADORecordBinding &pAdoRecordBinding)
{
	HRESULT hr;
	m_pRecBinding = NULL;

	//Open the binding interface.
	if(FAILED(hr = m_pRecordset->QueryInterface(__uuidof(IADORecordBinding), (LPVOID*)&m_pRecBinding )))
	{
		_com_issue_error(hr);
		return FALSE;
	}
	
	//Bind the recordset to class
	if(FAILED(hr = m_pRecBinding->BindToRecordset(&pAdoRecordBinding)))
	{
		_com_issue_error(hr);
		return FALSE;
	}
	return TRUE;
}

BOOL CADORecordset::GetFieldInfo(LPCTSTR lpFieldName, CADOFieldInfo* fldInfo)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);
	
	return GetFieldInfo(pField, fldInfo);
}

BOOL CADORecordset::GetFieldInfo(int nIndex, CADOFieldInfo* fldInfo)
{
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return GetFieldInfo(pField, fldInfo);
}


BOOL CADORecordset::GetFieldInfo(FieldPtr pField, CADOFieldInfo* fldInfo)
{
	memset(fldInfo, 0, sizeof(CADOFieldInfo));

	lstrcpy(fldInfo->m_strName, (LPCTSTR)pField->GetName());
	fldInfo->m_lDefinedSize = (long) pField->GetDefinedSize();
	fldInfo->m_nType = pField->GetType();
	fldInfo->m_lAttributes = pField->GetAttributes();
	if(!IsEof())
		fldInfo->m_lSize = (long) pField->GetActualSize();
	return TRUE;
}

BOOL CADORecordset::GetChunk(LPCTSTR lpFieldName, CString& strValue)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);
	
	return GetChunk(pField, strValue);
}

BOOL CADORecordset::GetChunk(int nIndex, CString& strValue)
{
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);
	
	return GetChunk(pField, strValue);
}


BOOL CADORecordset::GetChunk(FieldPtr pField, CString& strValue)
{
	CString str;
	long lngSize, lngOffSet = 0;
	_variant_t varChunk;

	lngSize = (long) pField->ActualSize;
	
	str.Empty();
	while(lngOffSet < lngSize)
	{ 
		try
		{
			varChunk = pField->GetChunk(ChunkSize);
			
			str += varChunk.bstrVal;
			lngOffSet += ChunkSize;
		}
		catch(_com_error &e)
		{
			dump_com_error(e);
			return FALSE;
		}
	}

	lngOffSet = 0;
	strValue = str;
	return TRUE;
}

BOOL CADORecordset::GetChunk(LPCTSTR lpFieldName, LPVOID lpData)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return GetChunk(pField, lpData);
}

BOOL CADORecordset::GetChunk(int nIndex, LPVOID lpData)
{
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return GetChunk(pField, lpData);
}

BOOL CADORecordset::GetChunk(FieldPtr pField, LPVOID lpData)
{
	long lngSize, lngOffSet = 0;
	_variant_t varChunk;    
	UCHAR chData;
	HRESULT hr;
	long lBytesCopied = 0;

	lngSize = (long) pField->ActualSize;
	
	while(lngOffSet < lngSize)
	{ 
		try
		{
			varChunk = pField->GetChunk(ChunkSize);

			//Copy the data only upto the Actual Size of Field.  
            for(long lIndex = 0; lIndex <= (ChunkSize - 1); lIndex++)
            {
                hr= SafeArrayGetElement(varChunk.parray, &lIndex, &chData);
                if(SUCCEEDED(hr))
                {
                    //Take BYTE by BYTE and advance Memory Location
                    //hr = SafeArrayPutElement((SAFEARRAY FAR*)lpData, &lBytesCopied ,&chData); 
					((UCHAR*)lpData)[lBytesCopied] = chData;
                    lBytesCopied++;
                }
                else
                    break;
            }
			lngOffSet += ChunkSize;
		}
		catch(_com_error &e)
		{
			dump_com_error(e);
			return FALSE;
		}
	}

	lngOffSet = 0;
	return TRUE;
}

BOOL CADORecordset::AppendChunk(LPCTSTR lpFieldName, LPVOID lpData, UINT nBytes)
{

	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return AppendChunk(pField, lpData, nBytes);
}


BOOL CADORecordset::AppendChunk(int nIndex, LPVOID lpData, UINT nBytes)
{
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return AppendChunk(pField, lpData, nBytes);
}

BOOL CADORecordset::AppendChunk(FieldPtr pField, LPVOID lpData, UINT nBytes)
{
	HRESULT hr;
	_variant_t varChunk;
	long lngOffset = 0;
	UCHAR chData;
	SAFEARRAY FAR *psa = NULL;
	SAFEARRAYBOUND rgsabound[1];

	try
	{
		//Create a safe array to store the array of BYTES 
		rgsabound[0].lLbound = 0;
		rgsabound[0].cElements = nBytes;
		psa = SafeArrayCreate(VT_UI1,1,rgsabound);

		while(lngOffset < (long)nBytes)
		{
			chData	= ((UCHAR*)lpData)[lngOffset];
			hr = SafeArrayPutElement(psa, &lngOffset, &chData);

			if(FAILED(hr))
				return FALSE;
			
			lngOffset++;
		}
		lngOffset = 0;

		//Assign the Safe array  to a variant. 
		varChunk.vt = VT_ARRAY|VT_UI1;
		varChunk.parray = psa;

		hr = pField->AppendChunk(varChunk);

		if(SUCCEEDED(hr)) return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}

	return FALSE;
}

CString CADORecordset::GetString(LPCTSTR lpCols, LPCTSTR lpRows, LPCTSTR lpNull, long numRows)
{
	_bstr_t varOutput;
	_bstr_t varNull("");
	_bstr_t varCols("\t");
	_bstr_t varRows("\r");

	if(strlen(lpCols) != 0)
		varCols = _bstr_t(lpCols);

	if(strlen(lpRows) != 0)
		varRows = _bstr_t(lpRows);
	
	if(numRows == 0)
		numRows =(long)GetRecordCount();			
			
	varOutput = m_pRecordset->GetString(adClipString, numRows, varCols, varRows, varNull);

	return (LPCTSTR)varOutput;
}

CString IntToStr(int nVal)
{
	CString strRet;
	char buff[10];
	
	_itoa_s(nVal, buff, 10, 10);
	strRet = buff;
	return strRet;
}

CString LongToStr(long lVal)
{
	CString strRet;
	char buff[20];
	
	_ltoa_s(lVal, buff, 20, 10);
	strRet = buff;
	return strRet;
}

CString ULongToStr(unsigned long ulVal)
{
	CString strRet;
	char buff[20];
	
	_ultoa_s(ulVal, buff, 20, 10);
	strRet = buff;
	return strRet;

}


CString DblToStr(double dblVal, int ndigits)
{
	CString strRet;
	char buff[50];

   _gcvt_s(buff, 50, dblVal, ndigits );
	strRet = buff;
	return strRet;
}

CString DblToStr(float fltVal)
{
	CString strRet;
	char buff[50];
	
   _gcvt_s(buff, 50, fltVal, 10 );
	strRet = buff;
	return strRet;
}

void CADORecordset::Edit()
{
	m_nEditStatus = dbEdit;
}

BOOL CADORecordset::AddNew()
{
	m_nEditStatus = dbEditNone;
	if(m_pRecordset->AddNew() != S_OK)
		return FALSE;

	m_nEditStatus = dbEditNew;
	return TRUE;
}

BOOL CADORecordset::AddNew(CADORecordBinding &pAdoRecordBinding)
{
	try
	{
		if(m_pRecBinding->AddNew(&pAdoRecordBinding) != S_OK)
		{
			return FALSE;
		}
		else
		{
			m_pRecBinding->Update(&pAdoRecordBinding);
			return TRUE;
		}
			
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}	
}

BOOL CADORecordset::Update()
{
	BOOL bret = TRUE;

	if(m_nEditStatus != dbEditNone)
	{

		try
		{
			if(m_pRecordset->Update() != S_OK)
				bret = FALSE;
		}
		catch(_com_error &e)
		{
			dump_com_error(e);
			bret = FALSE;
		}

		if(!bret)
			m_pRecordset->CancelUpdate();
		m_nEditStatus = dbEditNone;
	}
	return bret;
}

void CADORecordset::CancelUpdate()
{
	m_pRecordset->CancelUpdate();
	m_nEditStatus = dbEditNone;
}

BOOL CADORecordset::SetFieldValue(int nIndex, CString strValue)
{
	_variant_t vtFld;
	_variant_t vtIndex;	
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	if(!strValue.IsEmpty())
		vtFld.vt = VT_BSTR;
	else
		vtFld.vt = VT_NULL;

	//Corrected by Giles Forster 10/03/2001
	vtFld.bstrVal = strValue.AllocSysString();

	BOOL bret = PutFieldValue(vtIndex, vtFld);

	//Corrected by Flemming27 at CodeProject.com
	SysFreeString(vtFld.bstrVal);
	return bret;
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, CString strValue)
{
	_variant_t vtFld;

	if(!strValue.IsEmpty())
		vtFld.vt = VT_BSTR;
	else
		vtFld.vt = VT_NULL;

	//Corrected by Giles Forster 10/03/2001
	vtFld.bstrVal = strValue.AllocSysString();

	BOOL bret =  PutFieldValue(lpFieldName, vtFld);
	//Corrected by Flemming27 at CodeProject.com
	SysFreeString(vtFld.bstrVal);
	return bret;
}

BOOL CADORecordset::SetFieldValue(int nIndex, int nValue)
{
	_variant_t vtFld;
	
	vtFld.vt = VT_I2;
	vtFld.iVal = nValue;
	
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, int nValue)
{
	_variant_t vtFld;
	
	vtFld.vt = VT_I2;
	vtFld.iVal = nValue;
	
	
	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, long lValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_I4;
	vtFld.lVal = lValue;
	
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	return PutFieldValue(vtIndex, vtFld);
	
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, long lValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_I4;
	vtFld.lVal = lValue;
	
	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, unsigned long ulValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_UI4;
	vtFld.ulVal = ulValue;
	
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	return PutFieldValue(vtIndex, vtFld);
	
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, unsigned long ulValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_UI4;
	vtFld.ulVal = ulValue;
	
	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, double dblValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_R8;
	vtFld.dblVal = dblValue;

	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, double dblValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_R8;
	vtFld.dblVal = dblValue;
		
	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, COleDateTime time)
{
	_variant_t vtFld;
	vtFld.vt = VT_DATE;
	vtFld.date = time;
	
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, COleDateTime time)
{
	_variant_t vtFld;
	vtFld.vt = VT_DATE;
	vtFld.date = time;
	
	return PutFieldValue(lpFieldName, vtFld);
}



BOOL CADORecordset::SetFieldValue(int nIndex, bool bValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_BOOL;
	vtFld.boolVal = bValue;
	
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, bool bValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_BOOL;
	vtFld.boolVal = bValue;
	
	return PutFieldValue(lpFieldName, vtFld);
}


BOOL CADORecordset::SetFieldValue(int nIndex, COleCurrency cyValue)
{
	if(cyValue.m_status == COleCurrency::invalid)
		return FALSE;

	_variant_t vtFld;
		
	vtFld.vt = VT_CY;
	vtFld.cyVal = cyValue.m_cur;
	
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, COleCurrency cyValue)
{
	if(cyValue.m_status == COleCurrency::invalid)
		return FALSE;

	_variant_t vtFld;

	vtFld.vt = VT_CY;
	vtFld.cyVal = cyValue.m_cur;	
		
	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, _variant_t vtValue)
{
	_variant_t vtIndex;
	
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;
	
	return PutFieldValue(vtIndex, vtValue);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, _variant_t vtValue)
{	
	return PutFieldValue(lpFieldName, vtValue);
}


BOOL CADORecordset::SetBookmark()
{
	if(m_varBookmark.vt != VT_EMPTY)
	{
		m_pRecordset->Bookmark = m_varBookmark;
		return TRUE;
	}
	return FALSE;
}

BOOL CADORecordset::Delete()
{
	if(m_pRecordset->Delete(adAffectCurrent) != S_OK)
		return FALSE;

	if(m_pRecordset->Update() != S_OK)
		return FALSE;
	
	m_nEditStatus = dbEditNone;
	return TRUE;
}

BOOL CADORecordset::Find(LPCTSTR lpFind, int nSearchDirection)
{

	m_strFind = lpFind;
	m_nSearchDirection = nSearchDirection;

	ASSERT(!m_strFind.IsEmpty());

	if(m_nSearchDirection == searchForward)
	{
		m_pRecordset->Find(_bstr_t(m_strFind), 0, adSearchForward, "");
		if(!IsEof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	else if(m_nSearchDirection == searchBackward)
	{
		m_pRecordset->Find(_bstr_t(m_strFind), 0, adSearchBackward, "");
		if(!IsBof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	else
	{
		TRACE("Unknown parameter. %d", nSearchDirection);
		m_nSearchDirection = searchForward;
	}
	return FALSE;
}

BOOL CADORecordset::FindFirst(LPCTSTR lpFind)
{
	m_pRecordset->MoveFirst();
	return Find(lpFind);
}

BOOL CADORecordset::FindNext()
{
	if(m_nSearchDirection == searchForward)
	{
		m_pRecordset->Find(_bstr_t(m_strFind), 1, adSearchForward, m_varBookFind);
		if(!IsEof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	else
	{
		m_pRecordset->Find(_bstr_t(m_strFind), 1, adSearchBackward, m_varBookFind);
		if(!IsBof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CADORecordset::PutFieldValue(LPCTSTR lpFieldName, _variant_t vtFld)
{
	if(m_nEditStatus == dbEditNone)
		return FALSE;
	
	try
	{
		m_pRecordset->Fields->GetItem(lpFieldName)->Value = vtFld; 
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;	
	}
}


BOOL CADORecordset::PutFieldValue(_variant_t vtIndex, _variant_t vtFld)
{
	if(m_nEditStatus == dbEditNone)
		return FALSE;

	try
	{
		m_pRecordset->Fields->GetItem(vtIndex)->Value = vtFld;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::Clone(CADORecordset &pRs)
{
	try
	{
		pRs.m_pRecordset = m_pRecordset->Clone(adLockUnspecified);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::SetFilter(LPCTSTR strFilter)
{
	ASSERT(IsOpen());
	
	try
	{
		m_pRecordset->PutFilter(strFilter);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::SetSort(LPCTSTR strCriteria)
{
	ASSERT(IsOpen());
	
	try
	{
		m_pRecordset->PutSort(strCriteria);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::SaveAsXML(LPCTSTR lpstrXMLFile)
{
	HRESULT hr;

	ASSERT(IsOpen());
	
	try
	{
		hr = m_pRecordset->Save(lpstrXMLFile, adPersistXML);
		return hr == S_OK;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
	return TRUE;
}

BOOL CADORecordset::OpenXML(LPCTSTR lpstrXMLFile)
{
	HRESULT hr = S_OK;

	if(IsOpen())
		Close();

	try
	{
		hr = m_pRecordset->Open(lpstrXMLFile, "Provider=MSPersist;", adOpenForwardOnly, adLockOptimistic, adCmdFile);
		return hr == S_OK;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::Execute(CADOCommand* pAdoCommand)
{
	if(IsOpen())
		Close();

	ASSERT(!pAdoCommand->GetText().IsEmpty());
	try
	{
		m_pConnection->CursorLocation = adUseClient;
		m_pRecordset = pAdoCommand->GetCommand()->Execute(NULL, NULL, pAdoCommand->GetType());
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

void CADORecordset::dump_com_error(_com_error &e)
{
	CString ErrorStr;
	
	
	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	ErrorStr.Format( "CADORecordset Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n",
		e.Error(), e.ErrorMessage(), (LPCSTR)bstrSource, (LPCSTR)bstrDescription );
	m_strLastError = _T("Query = " + GetQuery() + '\n' + ErrorStr);
	m_dwLastError = e.Error();
	#ifdef _DEBUG
		AfxMessageBox( ErrorStr, MB_OK | MB_ICONERROR );
	#endif	
	throw CADOException(e.Error(), e.Description());
}


///////////////////////////////////////////////////////
//
// CADOCommad Class
//

CADOCommand::CADOCommand(CADODatabase* pAdoDatabase, CString strCommandText, int nCommandType)
{
	m_pCommand = NULL;
	m_pCommand.CreateInstance(__uuidof(Command));
	m_strCommandText = strCommandText;
	m_pCommand->CommandText = m_strCommandText.AllocSysString();
	m_nCommandType = nCommandType;
	m_pCommand->CommandType = (CommandTypeEnum)m_nCommandType;
	m_pCommand->ActiveConnection = pAdoDatabase->GetActiveConnection();	
	m_nRecordsAffected = 0;
}

BOOL CADOCommand::AddParameter(CADOParameter* pAdoParameter)
{
	ASSERT(pAdoParameter->GetParameter() != NULL);

	try
	{
		m_pCommand->Parameters->Append(pAdoParameter->GetParameter());
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection, long lSize, int nValue)
{

	_variant_t vtValue;

	vtValue.vt = VT_I2;
	vtValue.iVal = nValue;

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection, long lSize, long lValue)
{

	_variant_t vtValue;

	vtValue.vt = VT_I4;
	vtValue.lVal = lValue;

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection, long lSize, double dblValue, int nPrecision, int nScale)
{

	_variant_t vtValue;

	vtValue.vt = VT_R8;
	vtValue.dblVal = dblValue;

	return AddParameter(strName, nType, nDirection, lSize, vtValue, nPrecision, nScale);
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection, long lSize, CString strValue)
{

	_variant_t vtValue;

	vtValue.vt = VT_BSTR;
	vtValue.bstrVal = strValue.AllocSysString();

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection, long lSize, COleDateTime time)
{

	_variant_t vtValue;

	vtValue.vt = VT_DATE;
	vtValue.date = time;

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}


BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection, long lSize, _variant_t vtValue, int nPrecision, int nScale)
{
	try
	{
		_ParameterPtr pParam = m_pCommand->CreateParameter(strName.AllocSysString(), (ADODB::DataTypeEnum)nType, (ADODB::ParameterDirectionEnum)nDirection, lSize, vtValue);
		pParam->PutPrecision(nPrecision);
		pParam->PutNumericScale(nScale);
		m_pCommand->Parameters->Append(pParam);
		
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


void CADOCommand::SetText(CString strCommandText)
{
	ASSERT(!strCommandText.IsEmpty());

	m_strCommandText = strCommandText;
	m_pCommand->CommandText = m_strCommandText.AllocSysString();
}

void CADOCommand::SetType(int nCommandType)
{
	m_nCommandType = nCommandType;
	m_pCommand->CommandType = (CommandTypeEnum)m_nCommandType;
}

BOOL CADOCommand::Execute(int nCommandType /*= typeCmdStoredProc*/)
{
	_variant_t vRecords;
	m_nRecordsAffected = 0;
	try
	{
		m_nCommandType = nCommandType;
		m_pCommand->Execute(&vRecords, NULL, nCommandType);
		m_nRecordsAffected = vRecords.iVal;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

void CADOCommand::dump_com_error(_com_error &e)
{
	CString ErrorStr;
	
	
	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	ErrorStr.Format( "CADOCommand Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n",
		e.Error(), e.ErrorMessage(), (LPCSTR)bstrSource, (LPCSTR)bstrDescription );
	m_strLastError = ErrorStr;
	m_dwLastError = e.Error();
	#ifdef _DEBUG
		AfxMessageBox(ErrorStr, MB_OK | MB_ICONERROR);
	#endif	
	throw CADOException(e.Error(), e.Description());
}


///////////////////////////////////////////////////////
//
// CADOParameter Class
//

CADOParameter::CADOParameter(int nType, long lSize, int nDirection, CString strName)
{
	m_pParameter = NULL;
	m_pParameter.CreateInstance(__uuidof(Parameter));
	m_strName = _T("");
	m_pParameter->Direction = (ADODB::ParameterDirectionEnum)nDirection;
	m_strName = strName;
	m_pParameter->Name = m_strName.AllocSysString();
	m_pParameter->Type = (ADODB::DataTypeEnum)nType;
	m_pParameter->Size = lSize;
	m_nType = nType;
}

BOOL CADOParameter::SetValue(int nValue)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);
	
	vtVal.vt = VT_I2;
	vtVal.iVal = nValue;

	try
	{
		if(m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(int);

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


BOOL CADOParameter::SetValue(long lValue)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);
	
	vtVal.vt = VT_I4;
	vtVal.lVal = lValue;

	try
	{
		if(m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(long);

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::SetValue(double dblValue)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);
	
	vtVal.vt = VT_R8;
	vtVal.dblVal = dblValue;

	try
	{
		if(m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(double);

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::SetValue(CString strValue)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);
	
	if(!strValue.IsEmpty())
		vtVal.vt = VT_BSTR;
	else
		vtVal.vt = VT_NULL;

	//Corrected by Giles Forster 10/03/2001
	vtVal.bstrVal = strValue.AllocSysString();
	//vtVal.SetString(strValue.GetBuffer(0));

	try
	{
		if(m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(char) * strValue.GetLength();

		m_pParameter->Value = vtVal;
		::SysFreeString(vtVal.bstrVal);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		::SysFreeString(vtVal.bstrVal);
		return FALSE;
	}
}

BOOL CADOParameter::SetValue(COleDateTime time)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);
	
	vtVal.vt = VT_DATE;
	vtVal.date = time;

	try
	{
		if(m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(DATE);

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::SetValue(_variant_t vtValue)
{

	ASSERT(m_pParameter != NULL);

	try
	{
		if(m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(VARIANT);
		
		m_pParameter->Value = vtValue;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(int& nValue)
{
	_variant_t vtVal;
	int nVal = 0;

	try
	{
		vtVal = m_pParameter->Value;

		switch(vtVal.vt)
		{
		case VT_BOOL:
			nVal = vtVal.boolVal;
			break;
		case VT_I2:
		case VT_UI1:
			nVal = vtVal.iVal;
			break;
		case VT_INT:
			nVal = vtVal.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			nVal = 0;
			break;
		default:
			nVal = vtVal.iVal;
		}	
		nValue = nVal;
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(long& lValue)
{
	_variant_t vtVal;
	long lVal = 0;

	try
	{
		vtVal = m_pParameter->Value;
		if(vtVal.vt != VT_NULL && vtVal.vt != VT_EMPTY)
			lVal = vtVal.lVal;
		lValue = lVal;
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(double& dbValue)
{
	_variant_t vtVal;
	double dblVal;
	try
	{
		vtVal = m_pParameter->Value;
		switch(vtVal.vt)
		{
		case VT_R4:
			dblVal = vtVal.fltVal;
			break;
		case VT_R8:
			dblVal = vtVal.dblVal;
			break;
		case VT_DECIMAL:
			//Corrected by Jos� Carlos Mart�nez Gal�n
			dblVal = vtVal.decVal.Lo32;
			dblVal *= (vtVal.decVal.sign == 128)? -1 : 1;
			dblVal /= pow(10.f, (int) vtVal.decVal.scale); 
			break;
		case VT_UI1:
			dblVal = vtVal.iVal;
			break;
		case VT_I2:
		case VT_I4:
			dblVal = vtVal.lVal;
			break;
		case VT_INT:
			dblVal = vtVal.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			dblVal = 0;
			break;
		default:
			dblVal = 0;
		}
		dbValue = dblVal;
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(CString& strValue, CString strDateFormat)
{
	_variant_t vtVal;
	CString strVal;

	try
	{
		vtVal = m_pParameter->Value;
		switch(vtVal.vt) 
		{
		case VT_R4:
			strVal = DblToStr(vtVal.fltVal);
			break;
		case VT_R8:
			strVal = DblToStr(vtVal.dblVal);
			break;
		case VT_BSTR:
			strVal = vtVal.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			strVal = IntToStr(vtVal.iVal);
			break;
		case VT_INT:
			strVal = IntToStr(vtVal.intVal);
			break;
		case VT_I4:
			strVal = LongToStr(vtVal.lVal);
			break;
		case VT_DECIMAL:
			{
			//Corrected by Jos� Carlos Mart�nez Gal�n
			double val = vtVal.decVal.Lo32;
			val *= (vtVal.decVal.sign == 128)? -1 : 1;
			val /= pow(10.0f, (int) vtVal.decVal.scale); 
			strVal = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtVal);

				if(strDateFormat.IsEmpty())
					strDateFormat = _T("%Y-%m-%d %H:%M:%S");
				strVal = dt.Format(strDateFormat);
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			strVal.Empty();
			break;
		default:
			strVal.Empty();
			return FALSE;
		}
		strValue = strVal;
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(COleDateTime& time)
{
	_variant_t vtVal;

	try
	{
		vtVal = m_pParameter->Value;
		switch(vtVal.vt) 
		{
		case VT_DATE:
			{
				COleDateTime dt(vtVal);
				time = dt;
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			time.SetStatus(COleDateTime::null);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(_variant_t& vtValue)
{
	try
	{
		vtValue = m_pParameter->Value;
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


void CADOParameter::dump_com_error(_com_error &e)
{
	CString ErrorStr;
	
	
	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	ErrorStr.Format( "CADOParameter Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n",
		e.Error(), e.ErrorMessage(), (LPCSTR)bstrSource, (LPCSTR)bstrDescription );
	m_strLastError = ErrorStr;
	m_dwLastError = e.Error();
	#ifdef _DEBUG
		AfxMessageBox(ErrorStr, MB_OK | MB_ICONERROR);
	#endif	
	throw CADOException(e.Error(), e.Description());
}

#endif // _WINDOWS