/**************************************************************************
*   Copyright (C) 2010 by Michael Carpenter (malcom2073)                  *
*   mcarpenter@interforcesystems.com                                      *
*                                                                         *
*   This file is a part of libobd                                         *
*                                                                         *
*   libobd is free software: you can redistribute it and/or modify        *
*   it under the terms of the GNU Lesser General Public License as        *
*   published by the Free Software Foundation, either version 2 of        *
*   the License, or (at your option) any later version.                   *
*                                                                         *
*   libobd is distributed in the hope that it will be useful,             *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with libobd.  If not, see <http://www.gnu.org/licenses/>.       *
***************************************************************************/

#include "obdlib.h"
#include <time.h>
#include <iostream>
#include <fstream>
#include <string.h>

obdLib::obdLib()
{
	m_lastError = NONE;	
}

int obdLib::openPort(const char *portName)
{
	return openPort(portName,-1);
}

int obdLib::openPort(const char *portName,int baudrate)
{
#ifdef WINVER
	portHandle=CreateFileA(portName, GENERIC_READ|GENERIC_WRITE,0, NULL, OPEN_EXISTING, 0, NULL);
	if (portHandle == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	COMMCONFIG Win_CommConfig;
	COMMTIMEOUTS Win_CommTimeouts;
	unsigned long confSize = sizeof(COMMCONFIG);
	Win_CommConfig.dwSize = confSize;
	GetCommConfig(portHandle, &Win_CommConfig, &confSize);
	Win_CommConfig.dcb.Parity = 0;
	Win_CommConfig.dcb.fRtsControl = RTS_CONTROL_DISABLE;
	Win_CommConfig.dcb.fOutxCtsFlow = FALSE;
	Win_CommConfig.dcb.fOutxDsrFlow = FALSE;
	Win_CommConfig.dcb.fDtrControl = DTR_CONTROL_DISABLE;
	Win_CommConfig.dcb.fDsrSensitivity = FALSE;
	Win_CommConfig.dcb.fNull=FALSE;
	Win_CommConfig.dcb.fTXContinueOnXoff = FALSE;
	Win_CommConfig.dcb.fInX=FALSE;
	Win_CommConfig.dcb.fOutX=FALSE;
	Win_CommConfig.dcb.fBinary=TRUE;
	Win_CommConfig.dcb.DCBlength = sizeof(DCB);
	if (baudrate != -1)
	{
		Win_CommConfig.dcb.BaudRate = baudrate;
	}
	Win_CommConfig.dcb.ByteSize = 8;
	Win_CommTimeouts.ReadIntervalTimeout = 50;
	Win_CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	Win_CommTimeouts.ReadTotalTimeoutConstant = 110;
	Win_CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	Win_CommTimeouts.WriteTotalTimeoutConstant = 110;
	SetCommConfig(portHandle, &Win_CommConfig, sizeof(COMMCONFIG));
	SetCommTimeouts(portHandle,&Win_CommTimeouts);
	return 0;
#else
	//NEED TO USE BAUD RATE HERE!!!: baudrate
	//printf("Attempting to open COM port\n");
	portHandle = open(portName,O_RDWR | O_NOCTTY | O_NDELAY);
	if (portHandle < 0)
	{
		printf("Error opening Com: %s\n",portName);
		return -1;
	}
	printf("Com Port Opened %i\n",portHandle);
	fcntl(portHandle, F_SETFL, 0); //Set it to blocking. This is required? Wtf?
	struct termios oldtio;
	struct termios newtio;
	//bzero(&newtio,sizeof(newtio));
	tcgetattr(portHandle,&newtio);
	long BAUD = B9600;  
	switch (baudrate)
	{
		case 38400:
		default:
			BAUD = B38400;
			break;
		case 19200:
			BAUD  = B19200;
			break;
		case 9600:
			BAUD  = B9600;
			break;
		case 4800:
			BAUD  = B4800;
			break;
	}  //end of switch baud_rate
	if (strspn("/dev/pts",portName) >= 8)
	{
		printf("PTS detected... disabling baud rate selection: %s\n",portName);
		baudrate = -1;
	}
	else
	{
	}

	newtio.c_cflag |= (CLOCAL | CREAD);
	newtio.c_lflag &= !(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_oflag &= !(OPOST);
	newtio.c_cc[VMIN] = 0;
	newtio.c_cc[VTIME] = 100;
/*
	newtio.c_cflag &= ~CSIZE; //Disable byte size
	newtio.c_cflag &= ~PARENB; //Disable parity
	newtio.c_cflag &= ~CSTOPB; //Disable stop bits
	newtio.c_cflag |= (CLOCAL | CREAD | CS8); //Set local mode, reader, and 8N1.

	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //Disable CANON, echo, and signals

	newtio.c_oflag &= ~(OPOST); //Disable post processing
*/
	if (baudrate != -1)
	{
		if(cfsetispeed(&newtio, BAUD))
		{
			perror("cfsetispeed");
		}

		if(cfsetospeed(&newtio, BAUD))
		{
			perror("cfsetospeed");
		}

		printf("Setting baud rate to %i on port %s\n",baudrate,portName);
	}
	tcsetattr(portHandle,TCSANOW,&newtio);
	//newtio.c_cc[VMIN] = 0; //Minimum number of bytes to read
	//newtio.c_cc[VTIME] = 100; //Read Timeout (10.0 seconds)

	
	//tcflush(portHandle,TCIFLUSH);
	return 0;
#endif
}

void obdLib::setPortHandle(HANDLE hdnl)
{
	portHandle = hdnl;
}

int obdLib::closePort()
{
	#ifdef WINVER
	CloseHandle(portHandle);
	#else
//	tcsetattr(portHandle,TCSANOW,&oldtio);
	close(portHandle);
	#endif
	return 0;
}
int obdLib::initPort()
{
	sendObdRequest("atz\r",4);
	sendObdRequest("ati\r",4);
	sendObdRequest("ate0\r",5);
#ifdef WINVER
	Sleep(3000);
#else
	usleep(3000000);
	tcflush(portHandle,TCIFLUSH);
#endif	
	sendObdRequest("atl0\r",5);
	sendObdRequest("ath0\r",5);
	sendObdRequest("010C\r",5);
	return 1;
}
byte obdLib::byteArrayToByte(byte b1, byte b2)
{
	byte newB1 = 0;
	byte newB2 = 0;
	if ((b1 >= 48) && (b1 <= 57))
	{
		newB1 = ((b1 - 48) * 16);
	}
	else if ((b1 >=65) && (b1 <= 90))
	{
		newB1 = ((b1 - 55) * 16);
	}
	else
	{
		newB1 = ((b1 - 87) * 16);
	}
	if ((b2 >= 48) && (b2 <= 57))
	{
		newB2 = (b2 - 48);
	}
	else if ((b2 >= 65) && (b2 <= 90))
	{
		newB2 = (b2 - 55);
	}
	else
	{
		newB2 = (b2 - 87);
	}
	byte retVal = (newB1 + newB2);
	return retVal;
}

bool obdLib::sendObdRequest(const char *req,int len)
{
	//Blind request
	std::vector<byte> reply;
	return sendObdRequestString(req,len,&reply,-1,-1);
}
bool obdLib::sendObdRequest(const char *req,int len,int sleeptime)
{
	std::vector<byte> reply;
	return sendObdRequestString(req,len,&reply,sleeptime,-1);
}

bool obdLib::sendObdRequestString(const char *req,int length,std::vector<byte> *reply)
{
	return sendObdRequestString(req,length,reply,20,3);
}
bool obdLib::sendObdRequestString(const char *req,int length,std::vector<byte> *reply,int sleeptime)
{
	return sendObdRequestString(req,length,reply,sleeptime,3);
}

void obdLib::flush()
{
#ifdef WINVER

#else
	tcflush(portHandle,TCIFLUSH);	
#endif
}
bool obdLib::sendObdRequestString(const char *req,int length,std::vector<byte> *reply,int sleeptime, int timeout)
{
	reply->clear();
	//std::vector<byte> tmpReply;
	char *tmp = new char[1024];
	char *totalReply = new char[1024];
	int loc = 0;
	int len = 0;
	time_t seconds = time(NULL);
#ifdef WINVER
	if (!::WriteFile(portHandle, (void*)req, (DWORD)length, (LPDWORD)&len, NULL)) {
		//DWORD error = GetLastError();
		//int i = 2;
		//An error happened, I should probably handle this sometime.
		delete[] totalReply;
		delete[] tmp;
		m_lastError = SERIALWRITEERROR;
		return false;
	}
#else
	len = write(portHandle,req,length);
#endif
	if (len < 0)
	{
		printf("No Write\n");
		//delete tmp;
		//delete totalReply;
		//m_lastError = SERIALWRITEERROR;
		//return false;
	}
	if (sleeptime == -1)
	{
		//Not expecting a reply.
		delete[] totalReply;
		delete[] tmp;
		return true;

	}
#ifdef WINVER
	Sleep(sleeptime);
#else
	usleep(sleeptime * 1000);
#endif
	bool continueLoop = true;
	while (continueLoop)
	{
#ifdef WINVER
		if (!ReadFile(portHandle,(LPVOID)tmp,1024,(LPDWORD)&len,NULL))
		{
			delete[] tmp;
			delete[] totalReply;
			m_lastError = SERIALREADERROR;
			return false;
		}
#else
		len = read(portHandle,tmp,1024);
#endif

		if (len < 0)
		{
			printf("No Read\n");
			perror("Error");
			delete[] tmp;
			delete[] totalReply;
			m_lastError = SERIALREADERROR;

			return false;
		}
		else if (len == 0)
		{
		#ifdef WINVER
			Sleep(10);
		#else
			usleep(10000);
		#endif
		}
		else
		{
			for (int i=0;i<len;i++)
			{
				if (/*(tmp[i] != 0x20) && (tmp[i] != '\r') && /*(tmp[i] != '\n') && */(tmp[i] != '>'))
				{
					//printf("Byte: %c %i ",tmp[i],tmp[i]);
					totalReply[loc++] = tmp[i];
				}
				if (tmp[i] == '>')
				{
					/*printf("\n> returned. Current: %i, Len: %i\n",i,len);
					printf("\n\nResponse: ");
					for (int j=i;j<len;j++)
					{
						printf("%c",tmp[j]);
					}
					printf(" :End\n\n");
					*/
					continueLoop = false;
				}	
			}
		}
		if (timeout > 0)
		{
			if ((time(NULL) - seconds)-(sleeptime / 1000.0) > timeout)
			{
				//printf("Time:%i:%i:%i\n",time(NULL) - seconds,(time(NULL) - seconds) - (sleeptime/1000.0),time(NULL));
				continueLoop = false;

				m_lastError = TIMEOUT;
				printf("Timeout, current reply state:");
				for (int i=0;i<loc;i++)
				{
					printf("%c",totalReply[i]);
				}

				printf(":\n");
				printf("Current reply length: %i\n",loc);
				delete[] tmp;
				delete[] totalReply;
				return false;
			}
		}
		if (timeout == -1)
		{
			//Not waiting for a reply.
			delete[] tmp;
			delete[] totalReply;
			return true;
		}
	}

	//Perform checking on totalReply
	for (int i=0;i<loc;i++)
	{
		reply->push_back(totalReply[i]);
	}
	//printf("Total Reply Size: %i\n",loc);
	totalReply[loc] = '\0';
	int errorlen = strspn("NODATA",totalReply);
	if (errorlen == 6)
	{
		//Error
		//printf("Error\n");
		//Nodata here
		m_lastError = NODATA;
		delete[] tmp;
		delete[] totalReply;
		return false;
	}
	delete[] totalReply;
	delete[] tmp;
	//tcflush(portHandle,TCIFLUSH);
	return true;
}
obdLib::ObdError obdLib::lastError()
{
	return m_lastError;
}
bool obdLib::sendObdRequest(const char *req,int length,std::vector<byte> *reply)
{
	reply->clear();
	std::vector<byte> tmpReply;
	if (!sendObdRequestString(req,length,&tmpReply))
	{
		return false;
	}
	for (unsigned int i=0;i<tmpReply.size()-1;i++)
	{
		if ((tmpReply[i] != 0x20) && (tmpReply[i] != '\r') && (tmpReply[i] != '\n'))
		{
			reply->push_back(byteArrayToByte(tmpReply[i],tmpReply[i+1]));
			i++;
		}
	}
	return true;
}


