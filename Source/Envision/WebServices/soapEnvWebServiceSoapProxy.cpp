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
/* soapEnvWebServiceSoapProxy.cpp
   Generated by gSOAP 2.7.16 from EnvWebServices.h
   Copyright(C) 2000-2010, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/


#include "stdafx.h"
#pragma hdrstop

#include "soapEnvWebServiceSoapProxy.h"

EnvWebServiceSoapProxy::EnvWebServiceSoapProxy()
{	EnvWebServiceSoapProxy_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);
}

EnvWebServiceSoapProxy::EnvWebServiceSoapProxy(const struct soap &_soap) :soap(_soap)
{ }

EnvWebServiceSoapProxy::EnvWebServiceSoapProxy(soap_mode iomode)
{	EnvWebServiceSoapProxy_init(iomode, iomode);
}

EnvWebServiceSoapProxy::EnvWebServiceSoapProxy(soap_mode imode, soap_mode omode)
{	EnvWebServiceSoapProxy_init(imode, omode);
}

void EnvWebServiceSoapProxy::EnvWebServiceSoapProxy_init(soap_mode imode, soap_mode omode)
{	soap_imode(this, imode);
	soap_omode(this, omode);
	soap_endpoint = NULL;
	static const struct Namespace namespaces[] =
{
	{"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
	{"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
	{"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
	{"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
	{"ns2", "http://envision.bioe.orst.edu/EnvWebServiceSoap", NULL, NULL},
	{"ns1", "http://envision.bioe.orst.edu/", NULL, NULL},
	{"ns3", "http://envision.bioe.orst.edu/EnvWebServiceSoap12", NULL, NULL},
	{NULL, NULL, NULL, NULL}
};
	this->namespaces = namespaces;
}

EnvWebServiceSoapProxy::~EnvWebServiceSoapProxy()
{ }

void EnvWebServiceSoapProxy::destroy()
{	soap_destroy(this);
	soap_end(this);
}

void EnvWebServiceSoapProxy::soap_noheader()
{	header = NULL;
}

const SOAP_ENV__Fault *EnvWebServiceSoapProxy::soap_fault()
{	return this->fault;
}

const char *EnvWebServiceSoapProxy::soap_fault_string()
{	return *soap_faultstring(this);
}

const char *EnvWebServiceSoapProxy::soap_fault_detail()
{	return *soap_faultdetail(this);
}

int EnvWebServiceSoapProxy::soap_close_socket()
{	return soap_closesock(this);
}

void EnvWebServiceSoapProxy::soap_print_fault(FILE *fd)
{	::soap_print_fault(this, fd);
}

#ifndef WITH_LEAN
void EnvWebServiceSoapProxy::soap_stream_fault(std::ostream& os)
{	::soap_stream_fault(this, os);
}

char *EnvWebServiceSoapProxy::soap_sprint_fault(char *buf, size_t len)
{	return ::soap_sprint_fault(this, buf, len);
}
#endif

int EnvWebServiceSoapProxy::GetEnvisionSetupDateTime(_ns1__GetEnvisionSetupDateTime *ns1__GetEnvisionSetupDateTime, _ns1__GetEnvisionSetupDateTimeResponse *ns1__GetEnvisionSetupDateTimeResponse)
{	struct soap *soap = this;
	struct __ns2__GetEnvisionSetupDateTime soap_tmp___ns2__GetEnvisionSetupDateTime;
	const char *soap_action = NULL;
	if (!soap_endpoint)
		soap_endpoint = "http://envws.bioe.orst.edu/EnvWebServices.asmx";
	soap_action = "http://envision.bioe.orst.edu/GetEnvisionSetupDateTime";
	soap->encodingStyle = NULL;
	soap_tmp___ns2__GetEnvisionSetupDateTime.ns1__GetEnvisionSetupDateTime = ns1__GetEnvisionSetupDateTime;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize___ns2__GetEnvisionSetupDateTime(soap, &soap_tmp___ns2__GetEnvisionSetupDateTime);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___ns2__GetEnvisionSetupDateTime(soap, &soap_tmp___ns2__GetEnvisionSetupDateTime, "-ns2:GetEnvisionSetupDateTime", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___ns2__GetEnvisionSetupDateTime(soap, &soap_tmp___ns2__GetEnvisionSetupDateTime, "-ns2:GetEnvisionSetupDateTime", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!ns1__GetEnvisionSetupDateTimeResponse)
		return soap_closesock(soap);
	ns1__GetEnvisionSetupDateTimeResponse->soap_default(soap);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	ns1__GetEnvisionSetupDateTimeResponse->soap_get(soap, "ns1:GetEnvisionSetupDateTimeResponse", "");
	if (soap->error)
		return soap_recv_fault(soap, 0);
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

int EnvWebServiceSoapProxy::GetEnvisionVersion(_ns1__GetEnvisionVersion *ns1__GetEnvisionVersion, _ns1__GetEnvisionVersionResponse *ns1__GetEnvisionVersionResponse)
{	struct soap *soap = this;
	struct __ns2__GetEnvisionVersion soap_tmp___ns2__GetEnvisionVersion;
	const char *soap_action = NULL;
	if (!soap_endpoint)
		soap_endpoint = "http://envws.bioe.orst.edu/EnvWebServices.asmx";
	soap_action = "http://envision.bioe.orst.edu/GetEnvisionVersion";
	soap->encodingStyle = NULL;
	soap_tmp___ns2__GetEnvisionVersion.ns1__GetEnvisionVersion = ns1__GetEnvisionVersion;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize___ns2__GetEnvisionVersion(soap, &soap_tmp___ns2__GetEnvisionVersion);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___ns2__GetEnvisionVersion(soap, &soap_tmp___ns2__GetEnvisionVersion, "-ns2:GetEnvisionVersion", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___ns2__GetEnvisionVersion(soap, &soap_tmp___ns2__GetEnvisionVersion, "-ns2:GetEnvisionVersion", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!ns1__GetEnvisionVersionResponse)
		return soap_closesock(soap);
	ns1__GetEnvisionVersionResponse->soap_default(soap);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	ns1__GetEnvisionVersionResponse->soap_get(soap, "ns1:GetEnvisionVersionResponse", "");
	if (soap->error)
		return soap_recv_fault(soap, 0);
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

int EnvWebServiceSoapProxy::GetStudyAreaSetupDateTime(_ns1__GetStudyAreaSetupDateTime *ns1__GetStudyAreaSetupDateTime, _ns1__GetStudyAreaSetupDateTimeResponse *ns1__GetStudyAreaSetupDateTimeResponse)
{	struct soap *soap = this;
	struct __ns2__GetStudyAreaSetupDateTime soap_tmp___ns2__GetStudyAreaSetupDateTime;
	const char *soap_action = NULL;
	if (!soap_endpoint)
		soap_endpoint = "http://envws.bioe.orst.edu/EnvWebServices.asmx";
	soap_action = "http://envision.bioe.orst.edu/GetStudyAreaSetupDateTime";
	soap->encodingStyle = NULL;
	soap_tmp___ns2__GetStudyAreaSetupDateTime.ns1__GetStudyAreaSetupDateTime = ns1__GetStudyAreaSetupDateTime;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize___ns2__GetStudyAreaSetupDateTime(soap, &soap_tmp___ns2__GetStudyAreaSetupDateTime);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___ns2__GetStudyAreaSetupDateTime(soap, &soap_tmp___ns2__GetStudyAreaSetupDateTime, "-ns2:GetStudyAreaSetupDateTime", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___ns2__GetStudyAreaSetupDateTime(soap, &soap_tmp___ns2__GetStudyAreaSetupDateTime, "-ns2:GetStudyAreaSetupDateTime", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!ns1__GetStudyAreaSetupDateTimeResponse)
		return soap_closesock(soap);
	ns1__GetStudyAreaSetupDateTimeResponse->soap_default(soap);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	ns1__GetStudyAreaSetupDateTimeResponse->soap_get(soap, "ns1:GetStudyAreaSetupDateTimeResponse", "");
	if (soap->error)
		return soap_recv_fault(soap, 0);
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

int EnvWebServiceSoapProxy::GetStudyAreaVersion(_ns1__GetStudyAreaVersion *ns1__GetStudyAreaVersion, _ns1__GetStudyAreaVersionResponse *ns1__GetStudyAreaVersionResponse)
{	struct soap *soap = this;
	struct __ns2__GetStudyAreaVersion soap_tmp___ns2__GetStudyAreaVersion;
	const char *soap_action = NULL;
	if (!soap_endpoint)
		soap_endpoint = "http://envws.bioe.orst.edu/EnvWebServices.asmx";
	soap_action = "http://envision.bioe.orst.edu/GetStudyAreaVersion";
	soap->encodingStyle = NULL;
	soap_tmp___ns2__GetStudyAreaVersion.ns1__GetStudyAreaVersion = ns1__GetStudyAreaVersion;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize___ns2__GetStudyAreaVersion(soap, &soap_tmp___ns2__GetStudyAreaVersion);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___ns2__GetStudyAreaVersion(soap, &soap_tmp___ns2__GetStudyAreaVersion, "-ns2:GetStudyAreaVersion", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___ns2__GetStudyAreaVersion(soap, &soap_tmp___ns2__GetStudyAreaVersion, "-ns2:GetStudyAreaVersion", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!ns1__GetStudyAreaVersionResponse)
		return soap_closesock(soap);
	ns1__GetStudyAreaVersionResponse->soap_default(soap);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	ns1__GetStudyAreaVersionResponse->soap_get(soap, "ns1:GetStudyAreaVersionResponse", "");
	if (soap->error)
		return soap_recv_fault(soap, 0);
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}
/* End of client proxy code */
