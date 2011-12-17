/**************************************************************************
*   Copyright (C) 2010                                                    *
*   Michael Carpenter (malcom2073) <mcarpenter@interforcesystems.com>     *
*   Kevron Rees (tripzero) tripzero.kev@gmail.com                         *
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

#include "ObdThread.h"
#include <QVector>
#include <QDateTime>
#include <QMetaType>
#include <QStringList>
#define CR "\015"
#define LF "\012"
ObdThread::ObdThread(QObject *parent) : QThread(parent)
{
	qRegisterMetaType<QList<QString> >("QList<QString>");
	qRegisterMetaType<ObdThread::ObdError>("ObdThread::ObdError");
	m_obd = new obdLib();
	m_threadRunning = false;
	m_requestLoopRunning = false;
	loopType = 0;
	m_set = 0;
	//threadLockMutex.lock();
	m_obdConnected = false;
	m_baud = 0;
	m_port = "";
	m_obdInfo = new ObdInfo();
	start();
}
void ObdThread::connect()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = CONNECT;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}
void ObdThread::start()
{
	m_threadRunning = true;
	QThread::start();
}
void ObdThread::reqMonitorStatus()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = MONITOR_STATUS;
	req.repeat = false;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}

void ObdThread::disconnect()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = DISCONNECT;
	m_reqClassList.append(req);
	threadLockMutex.unlock();

}
void ObdThread::clearReqList()
{
	removePidMutex.lock();
	m_reqClassListThreaded.clear();
	removePidMutex.unlock();
}
void ObdThread::requestTroubleCodes()
{
	qDebug() << "Trouble codes request...";
	threadLockMutex.lock();
	RequestClass req;
	req.repeat = false;
	req.type = TROUBLE_CODES;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}
void ObdThread::switchBaud()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = SWITCH_BAUD;
	req.repeat = false;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}

void ObdThread::singleShotRequest(QByteArray request)
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = RAW_REQUEST;
	req.custom = request;
	req.repeat = false;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}
void ObdThread::reqVoltage()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = VOLTAGE;
	req.repeat = false;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}
void ObdThread::reqMfgString()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = MFG_STRING_ONE;
	req.repeat = false;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}

void ObdThread::reqSupportedModes()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = REQ_SUPPORTED_MODES;
	req.repeat = false;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}

void ObdThread::blindSingleShotRequest(QByteArray request)
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = RAW_BLIND_REQUEST;
	req.custom = request;
	req.repeat = false;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}
void ObdThread::findObdPort()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = FIND_PORT;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}
void ObdThread::removeRequest(int mode, int pid, int priority)
{
	threadLockMutex.lock();
	RequestClass req;
	req.pid = pid;
	req.mode = mode;
	req.priority = priority;
	req.type = MODE_PID;
	m_reqClassRemoveList.append(req);
	threadLockMutex.unlock();
}

void ObdThread::run()
{
	int cycleCounter =0;
	bool first = true;
	m_requestLoopRunning = true;
	std::vector<unsigned char> replyVectorString;
	std::vector<unsigned char> replyVector;
	while (m_threadRunning)
	{
		removePidMutex.lock();
		threadLockMutex.lock();
		for (int i=0;i<m_reqClassListThreaded.size();i++)
		{
			if (!m_reqClassListThreaded[i].repeat)
			{
				m_reqClassFailureMap.remove(&m_reqClassListThreaded[i]);
				m_reqClassListThreaded.removeAt(i);
				i--;
			}
		}
		for (int i=0;i<m_reqClassRemoveList.count();i++)
		{
			//qDebug() << "Removing pid:" << m_reqClassRemoveList[i].mode << m_reqClassRemoveList[i].pid;
			for (int j=0;j<m_reqClassListThreaded.count();j++)
			{
				if ((m_reqClassRemoveList[i].mode == m_reqClassListThreaded[j].mode) && (m_reqClassRemoveList[i].pid == m_reqClassListThreaded[j].pid) && (m_reqClassRemoveList[i].type == m_reqClassListThreaded[j].type))
				{
					m_reqClassFailureMap.remove(&m_reqClassListThreaded[i]);
					m_reqClassListThreaded.removeAt(j);
					m_reqClassRemoveList.removeAt(i);
					j--;
					i--;
				}
			}
		}
		for (int i=0;i<m_reqClassList.count();i++)
		{
			qDebug() << "Adding req" << m_reqClassList[i].type;
			m_reqClassListThreaded.prepend(m_reqClassList[i]);
			m_reqClassFailureMap[&m_reqClassList[i]] = 0;
			m_whiteList[&m_reqClassList[i]]=false;
		}
		m_reqClassList.clear();
		threadLockMutex.unlock();
		first = true;
		//qDebug() << "Size:" << m_reqClassListThreaded.size();
		for (int i=0;i<m_reqClassListThreaded.count();i++)
		{
			//Handle request here
			if (m_reqClassListThreaded[i].type == CONNECT)
			{
				if (!m_connect())
				{
					qDebug() << "Unable to connect";
					//emit liberror(UNABLE_TO_OPEN_COM_PORT);
					continue;
				}
				m_obdConnected = true;
			}
			else if (m_reqClassListThreaded[i].type == DISCONNECT)
			{
				qDebug() << "Disconnected from ELM";
				m_obd->closePort();
				m_obdConnected = false;
				emit disconnected();

			}
			else if (m_reqClassListThreaded[i].type == VOLTAGE)
			{
				if (!m_obdConnected)
				{
					if (!m_connect())
						continue;
				};
				std::vector<unsigned char> reply;
				QByteArray replyArray;
				if (!m_obd->sendObdRequestString("ATRV\r",5,&reply,100,5))
				{
					emit consoleMessage("Unable to send voltage request");
					qDebug() << "Error with voltage request";
				}
				else
				{
					for (unsigned int j=0;j<reply.size();j++)
					{
						if (reply[j] != '\r' && reply[j] != '\n' && reply[j] != ' ' && reply[j] != '>')
						{
							replyArray.append(reply[j]);
						}
					}
					//emit singleShotResponse(m_reqClassListThreaded[i].custom,replyArray);
					//emit voltage()
					//qDebug() << replyArray;
					emit voltage(QString(replyArray).toDouble());
				}
				if (!m_obdConnected)
				{
					emit consoleMessage(QString("Disconnected"));
					m_obd->closePort();
					emit disconnected();
				}

			}
			else if (m_reqClassListThreaded[i].type == REQ_SUPPORTED_MODES)
			{
				if (!m_obdConnected)
				{
					if (!m_connect())
						continue;
				}
				std::vector<unsigned char> reply;
				QByteArray replyArray;
				QList<QString> modelist;
				if (m_obd->sendObdRequestString("0500\r",5,&reply,100,5))
				{
					modelist.append("05");
				}
				if (m_obd->sendObdRequestString("0600\r",5,&reply,100,5))
				{
					modelist.append("06");
				}
				if (m_obd->sendObdRequestString("0700\r",5,&reply,100,5))
				{
					modelist.append("07");
				}
				if (m_obd->sendObdRequestString("0800\r",5,&reply,100,5))
				{
					modelist.append("08");
				}
				if (m_obd->sendObdRequestString("0900\r",5,&reply,100,5))
				{
					modelist.append("09");
				}
				for (unsigned int j=0;j<reply.size();j++)
				{
					replyArray.append(reply[j]);
				}
				//emit singleShotResponse(m_reqClassListThreaded[i].custom,replyArray);
				emit supportedModes(modelist);
				if (!m_obdConnected)
				{
					emit consoleMessage(QString("Disconnected"));
					m_obd->closePort();
					emit disconnected();
				}
			}
			else if (m_reqClassListThreaded[i].type == SWITCH_BAUD)
			{
				//Switch baud to 38400;
				if (m_obdConnected)
				{
					qDebug() << "Switching baud...";
					m_obd->sendObdRequest("ATBRD 68\r",9);
					qDebug() << "Sent request. Closing and reopening port";
					//usleep(10000);

					m_obd->closePort();
					msleep(100);
					m_obd->openPort(m_port.toStdString().c_str(),38400);
					std::vector<unsigned char> reply;
					qDebug() << "Sending confirmation";
					m_obd->sendObdRequestString("\r",1,&reply,1,1);
					QString vect = "";
					for (int i=0;i<reply.size();i++)
					{
						vect.append(reply.at(i));
					}
					qDebug() << "Vect:" << vect;
				}
			}
			else if (m_reqClassListThreaded[i].type == CLEAR_TROUBLE_CODES)
			{
				consoleMessage("Trouble Code Clear Requested...");
				if (!m_obdConnected)
				{
					qDebug() << "CONNECT - MONITOR_STATUS";
					if (!m_connect())
						continue;
				}
				m_obd->sendObdRequest("04\r",3);
				if (!m_obdConnected)
				{
					emit consoleMessage(QString("Disconnected"));
					m_obd->closePort();
					emit disconnected();
				}
			}
			else if (m_reqClassListThreaded[i].type == MONITOR_STATUS)
			{
				qDebug() << "MONITOR_STATUS";
				if (!m_obdConnected)
				{
					if (!m_connect())
					{
						continue;
					}
				}

				std::vector<unsigned char> replyVector;
				m_obd->sendObdRequestString("0101\r",5,&replyVector,100,5);
				QString vect2 = "";

				for (int j=0;j<replyVector.size();j++)
				{
					if (replyVector[j] != '\n' && replyVector[j] != '\r' && replyVector[j] != ' ')
					{
						vect2.append(replyVector[j]);
					}
					//vect2 += replyVector[j];
				}
				if (vect2.size() < 8)
				{
					qDebug() << "MONITOR_STATUS returned a result too short!" << vect2;
				}
				else
				{
					qDebug() << "MONITOR_STATUS size: " << vect2.size();
					unsigned char one = m_obd->byteArrayToByte(vect2[4].toAscii(),vect2[5].toAscii());
					unsigned char two = m_obd->byteArrayToByte(vect2[6].toAscii(),vect2[7].toAscii());
					unsigned char three = m_obd->byteArrayToByte(vect2[8].toAscii(),vect2[9].toAscii());
					unsigned char four = m_obd->byteArrayToByte(vect2[10].toAscii(),vect2[11].toAscii());
					QString onestr;
					QString twostr;
					QString threestr;
					for (int j=0;j<8;j++)
					{
						if ((two >> j) & 1)
						{
							onestr += "1";
						}
						else
						{
							onestr += "0";
						}
						if ((three >> j) & 1)
						{
							twostr += "1";
						}
						else
						{
							twostr += "0";
						}
						if ((four >> j) & 1)
						{
							threestr += "1";
						}
						else
						{
							threestr += "0";
						}
					}
					qDebug() << "Monitor:" << onestr << twostr << threestr;
					qDebug() << vect2;
					qDebug() << QString::number(two) << QString::number(three) << QString::number(four);
					QList<QString> resultlist;
					QString misfire = QString(((two >> 0) & 1) ? "1" : "0") + ":" + (((two >> 4) & 1) ? "1" : "0");
					QString fuelsystem = QString(((two >> 1) & 1) ? "1" : "0") + ":" + (((two >> 5) & 1) ? "1" : "0");
					QString component = QString(((two >> 2) & 1) ? "1" : "0") + ":" + (((two >> 6) & 1) ? "1" : "0");
					//	QString reserved = QString(((two >> 3) & 1) ? "1" : "0") + ":" + (((two >> 7) & 1) ? "1" : "0");
					QString catalyst = QString(((three >> 0) & 1) ? "1" : "0") + ":" + (((four >> 0) & 1) ? "1" : "0");
					QString heatedcat =QString (((three >> 1) & 1) ? "1" : "0") + ":" + (((four >> 1) & 1) ? "1" : "0");
					QString evapsys = QString(((three >> 2) & 1) ? "1" : "0") + ":" + (((four >> 2) & 1) ? "1" : "0");
					QString secondair = QString(((three >> 3) & 1) ? "1" : "0") + ":" + (((four >> 3) & 1) ? "1" : "0");
					QString acrefrig = QString(((three >> 4) & 1) ? "1" : "0") + ":" + (((four >> 4) & 1) ? "1" : "0");
					QString oxygensensor = QString(((three >> 5) & 1) ? "1" : "0") + ":" + (((four >> 5) & 1) ? "1" : "0");
					QString oxygenheater = QString(((three >> 6) & 1) ? "1" : "0") + ":" + (((four >> 6) & 1) ? "1" : "0");
					QString egrsystem = QString(((three >> 7) & 1) ? "1" : "0") + ":" + (((four >> 7) & 1) ? "1" : "0");

					resultlist.append(misfire);
					resultlist.append(fuelsystem);
					resultlist.append(component);
					//resultlist.append(reserved);
					resultlist.append(catalyst);
					resultlist.append(heatedcat);
					resultlist.append(evapsys);
					resultlist.append(secondair);
					resultlist.append(acrefrig);
					resultlist.append(oxygensensor);
					resultlist.append(oxygenheater);
					resultlist.append(egrsystem);
					emit monitorTestResults(resultlist);

				}
				if (!m_obdConnected)
				{
					qDebug() << "DISCONNECT - MONITOR_STATUS";
					emit consoleMessage(QString("Disconnected"));
					m_obd->closePort();
					emit disconnected();
				}
			}
			else if (m_reqClassListThreaded[i].type == MFG_STRING_ONE)
			{
				if (!m_obdConnected)
				{
					if (!m_connect())
					{
						continue;
					}
				}

				std::vector<unsigned char> replyVector;
				m_obd->sendObdRequestString("AT@1\r",5,&replyVector,100,5);

				QString vect2 = "";

				for (int j=0;j<replyVector.size();j++)
				{
					if (replyVector[j] != '\n' && replyVector[j] != '\r' && replyVector[j] != ' ')
					{
						vect2.append(replyVector[j]);
					}
					//vect2 += replyVector[j];
				}
				emit mfgString(vect2);
				if (!m_obdConnected)
				{
					emit consoleMessage(QString("Disconnected"));
					m_obd->closePort();
					emit disconnected();
				}

			}
			else if (m_reqClassListThreaded[i].type == TROUBLE_CODES)
			{
				consoleMessage("Trouble Codes Requested...");
				if (!m_obdConnected)
				{
					if (!m_connect())
						continue;
				}
				else
				{
				}

				setHeaders(true);
				std::vector<unsigned char> replyVector;
				//troubleCodes(QList<QString> codes)
				QString vect = "";
				if (!m_obd->sendObdRequestString("ATDPN\r",6,&replyVector))
				{
					qDebug() << "Error retreiving protocol";
				}
				else
				{

					for (int j=0;j<replyVector.size();j++)
					{
						vect += replyVector[j];
					}
					//qDebug() << "Protcol Number:" << vect;
				}
				if (!m_obd->sendObdRequestString("03\r",3,&replyVector))
				{
					//qDebug() << "Error retreiving trouble codes";
					if (m_obd->lastError() == obdLib::NODATA)
					{
						//Nodata on trouble codes means there are none available to read.
						emit consoleMessage("No trouble codes");
						emit troubleCodes(QList<QString>());
						m_reqClassFailureMap.remove(&m_reqClassListThreaded[i]);
						m_reqClassListThreaded.removeAt(i);
						//continue;
						//qDebug() << "NODATA";
					}
					//Error
				}
				vect.clear();
				QString vect2 = "";

				for (int j=0;j<replyVector.size();j++)
				{
					if (replyVector[j] != '\n' && replyVector[j] != '\r' && replyVector[j] != ' ')
					{
						vect.append(replyVector[j]);
					}
					//qDebug() << replyVector[i];
					vect2 += replyVector[j];
				}
				//qDebug() << vect2;
				//qDebug() << vect;
				QByteArray bytevect;
				for (int j=0;j<vect.size();j++)
				{
					bytevect.append(m_obd->byteArrayToByte(vect.toAscii().at(j),vect.toAscii().at(j+1)));
					//qDebug() << "Origional" << vect.toAscii().at(i) << vect.toAscii().at(i+1);
					//qDebug() << "Converted:" << m_obd->byteArrayToByte(vect.toAscii().at(i),vect.toAscii().at(i+1));
					j++;
				}

				//qDebug() << "Trouble: " << bytevect;
				if (vect == "A6")
				{
					bool inHeader = true;
					QString num = "";
					for (int j=0;j<replyVector.size()-1;j++)
					{
						num += QString((char)replyVector.at(j));
					}
					QStringList numsplit = num.split(" ");
					QString currentHeader;
					numsplit.removeAt(3);
					for (int j=0;j<numsplit.count()-1;j++)
					{
						if (inHeader)
						{
							currentHeader = numsplit[j];
							inHeader = false;
							numsplit.removeAt(j);
							numsplit.removeAt(j);
							numsplit.removeAt(j);
							j--;
							j--;
							//qDebug() << "Header:" << currentHeader;
						}
						else
						{
							if (numsplit[j+1].length() > 2)
							{
								//This is a header message;

								inHeader = true;
							}
							else
							{
								QString code = numsplit[j] + numsplit[j+1];
								j++;
								//qDebug() << "Code:" << code;
							}
						}
					}
					//7E8

					//11 bit can
				}
				/*QString rett = "";
				for (int i=0;i<replyVector.size();i++)
				{
					rett += (char)replyVector[i] + " ";
				}
				qDebug() << rett;
				*/
				if (!m_obdConnected)
				{
					emit consoleMessage(QString("Disconnected"));
					m_obd->closePort();
					emit disconnected();
				}
				//continue;
				//qDebug() << rett;
				QList<QString> codes;
				bool inCode = false;
				int count = 0;
				unsigned char ecu = 0;
				QString codetype;
				for (int j=0;j<vect.size();j++)
				{
					if (vect[j] == '4' && vect[j+1] == '3')
					{
						inCode = true;
						j+=2;
					}
					if (inCode)
					{
						count++;
						unsigned char onefirst = m_obd->byteArrayToByte(vect.toAscii().at(j),vect.toAscii().at(j+1));
						unsigned char onesecond = m_obd->byteArrayToByte(vect.toAscii().at(j+2),vect.toAscii().at(j+3));

						/*unsigned char twofirst = m_obd->byteArrayToByte(vect.toAscii().at(i+4),vect.toAscii().at(i+5));
						unsigned char twosecond = m_obd->byteArrayToByte(vect.toAscii().at(i+6),vect.toAscii().at(i+7));

						unsigned char threefirst = m_obd->byteArrayToByte(vect.toAscii().at(i+8),vect.toAscii().at(i+9));
						unsigned char threesecond = m_obd->byteArrayToByte(vect.toAscii().at(i+10),vect.toAscii().at(i+11));
						*/
						if (!(onefirst >> 7) & 1)
						{
							if (!((onefirst >> 6) & 1))
							{
								//qDebug() << "Powertrain code";
								codetype = "P";
							}
							else if ((onefirst >> 6) & 1)
							{
								//qDebug() << "Chassis code";
								codetype = "C";
							}
						}
						else
						{
							if (!((onefirst >> 6) & 1))
							{
								//qDebug() << "Body code";
								codetype = "B";
							}
							else
							{
								//qDebug() << "Network code";
								codetype = "N";
							}
						}
						unsigned char second = (onefirst >> 4) & 3;
						unsigned char third = (onefirst) & 0xF;
						//unsigned char fourth = vect.toAscii().at(i+2);
						//unsigned char fifth = vect.toAscii().at(i+3);
						unsigned char fourth = ((onesecond >> 4) & 0xF);
						unsigned char fifth = (onesecond & 0xF);
						//qDebug () << (int)second << (int) third << (int) fourth << (int)fifth;
						codes.append(codetype + QString::number(second) + QString::number(third) + QString::number(fourth) + QString::number(fifth));
						//unsigned char fourth = (bytevect[i+1] << 4) & 0xF;
						//unsigned char fifth = (bytevect[i+1]) & 0xF;


					}
					j += 3;
				}
				/*qDebug() << "OUTPUTTING CODES";
				for (int i=0;i<codes.size();i++)
				{
					qDebug() << codes[i];
				}*/
				/*
				for (int i=0;i<bytevect.size();i++)
				{
					if (bytevect[i] == 0x43)
					{
						inCode = true;
						i++;
					}
					if (inCode)
					{
						count++;
						qDebug() << "Val:" << bytevect[i];
						if (!((bytevect[i] >> 7) & 1))
						{
							if (!((bytevect[i] >> 6) & 1))
							{
								qDebug() << "Powertrain code";
								//codes.append(QString::number(ecu) + ":P");
								codetype = "P";
							}
							else if (((bytevect[i] >> 6) & 1))
							{
								//codes.append(QString::number(ecu) + ":C");
								codetype = "C";
								qDebug () << "Chassis Code";
							}
						}
						else if ((bytevect[i] >> 7) & 1)
						{
							if (!((bytevect[i] >> 6) & 1))
							{
								//codes.append(QString::number(ecu) + ":B");
								codetype = "B";
								qDebug() << "Body code";
							}
							else if (((bytevect[i] >> 6) & 1))
							{
								//codes.append(QString::number(ecu) + ":U");
								codetype = "U";
								qDebug () << "Network Code";
							}
						}
						unsigned char second = (bytevect[i] << 4) & 3;
						unsigned char third = (bytevect[i]) & 0xF;
						unsigned char fourth = (bytevect[i+1] << 4) & 0xF;
						unsigned char fifth = (bytevect[i+1]) & 0xF;
						qDebug () << (int)second << (int) third << (int) fourth << (int)fifth;
						for (int j=i;j<bytevect.size();j++)
						{
							qDebug() << QString::number(bytevect.at(j));
						}
						if (second == 0 && third == 0 && fourth == 0 && fifth == 0)
						{

						}
						else
						{
							codes.append(QString::number(ecu).append(":").append(codetype).append(QString::number(second)).append(QString::number(third)).append(QString::number(fourth)).append(QString::number(fifth)));

						}
						i++;
						if (count >= 3)
						{
							count = 0;
							inCode = false;
							i++; // Pop past the checksum byte
						}
					}
					else
					{
						//Read headers here
						qDebug() << "Header:" << QString::number(bytevect[i]) << QString::number(bytevect[i+1]) << QString::number(bytevect[i+2]);
						ecu = bytevect[i+2];
						i+=2;
					}
				}*/
				//qDebug() << "Done with trouble codes";
				troubleCodes(codes);
				//qDebug() <<
				/*if (!m_obdConnected)
				{
					emit consoleMessage(QString("Disconnected"));
					m_obd->closePort();
					emit disconnected();
				}*/
				/*QString replyString = QString((char*)replyVectorString.data());
				if (replyString.contains("NODATA"))
				{
					qDebug() << "NODATA";
				}
				*/
				/*QString rep = "";
				for (int i=0;i<replyVector.size();i++)
				{
					rep += QString::number(replyVector[i],16).toUpper() + " ";
				}
			qDebug() << rep;*/

			}
			else if (m_reqClassListThreaded[i].type == RAW_REQUEST)
			{
				if (!m_obdConnected)
				{
					if (!m_connect())
						continue;
				}
				emit consoleMessage("Raw request");
				qDebug() << "Raw Request";
				std::vector<unsigned char> reply;
				QByteArray replyArray;

				if (!m_obd->sendObdRequestString(m_reqClassListThreaded[i].custom,m_reqClassListThreaded[i].custom.length(),&reply,100,5))
				{
					emit consoleMessage("Unable to send custom request");
					qDebug() << "Error with custom request";
				}
				for (unsigned int j=0;j<reply.size();j++)
				{
					replyArray.append(reply[j]);
				}
				emit singleShotResponse(m_reqClassListThreaded[i].custom,replyArray);
				if (!m_obdConnected)
				{
					emit consoleMessage(QString("Disconnected"));
					m_obd->closePort();
					emit disconnected();
				}
			}
			else if (m_reqClassListThreaded[i].type == RAW_BLIND_REQUEST)
			{
				if (!m_obdConnected)
				{
					if (!m_connect())
						continue;
				}
				emit consoleMessage("Raw blind request");
				qDebug() << "Raw blind Request";
				std::vector<unsigned char> reply;
				QByteArray replyArray;

				if (!m_obd->sendObdRequestString(m_reqClassListThreaded[i].custom,m_reqClassListThreaded[i].custom.length(),&reply,0,0))
				{
					emit consoleMessage("Unable to send custom blind request");
					qDebug() << "Error with custom blind request";
				}
				if (!m_obdConnected)
				{
					emit consoleMessage(QString("Disconnected"));
					m_obd->closePort();
					emit disconnected();
				}
			}
			else if (m_reqClassListThreaded[i].type == START_REQ_LOOP)
			{

			}
			else if (m_reqClassListThreaded[i].type == STOP_REQ_LOOP)
			{

			}
			else if (m_reqClassListThreaded[i].type == SCAN_ALL)
			{
				//Full scan for supported pids
				qDebug() << "Beginning full pid scan";
				if (!m_obdConnected)
				{
					if (!m_connect())
						continue;
				}
				QString pid = "";
				std::vector<unsigned char> reply;
				QList<QString> pidList;
				for (int j=1;j<=9;j++)
				{
					//pidList.append(QList<int>());
					if (j != 3 && j != 4 && j != 2)
					{
						for (int j=0;j<0x20;j++)
						{
							pid = QString("0").append(QString::number(j)).append(((j < 16) ? "0" : "")).append(QString::number(j,16));
							if (!m_obd->sendObdRequest(QString(pid + "\r").toStdString().c_str(),pid.length()+1,&reply))
							{
								if (m_obd->lastError() == obdLib::NODATA)
								{
									//pidList[i-1].append(0);
								}
								else
								{
									//pidList[i-1].append(0);

								}
							}
							else
							{
								//pidList[i-1].append(1);
								pidList.append(pid);
							}
							msleep(100);
						}
					}
				}
				supportedPids(pidList);
				if (!m_obdConnected)
				{
					emit consoleMessage(QString("Disconnected"));
					qDebug() << "ELM Disconnected";
					m_obd->closePort();
					emit disconnected();
				}

			}
			else if (m_reqClassListThreaded[i].type == FIND_PORT)
			{
				qDebug() << "Finding OBD port...";
				QStringList portList;
				portList << "/dev/ttyS0";
				portList << "/dev/ttyS1";
				portList << "/dev/ttyUSB0";
				portList << "/dev/ttyUSB1";
				portList << "/dev/ttyUSB2";
				portList << "/dev/ttyUSB3";
				portList << "/dev/ttyUSB4";
				//portList << "/dev/pts/0";
				//portList << "/dev/pts/1";
				//portList << "/dev/pts/2";
				//portList << "/dev/pts/3";
				QList<int> baudList;
				baudList << -1;
				baudList << 9600;
				baudList << 38400;
				if (!m_obdConnected)
				{
					bool found = false;
					QString foundport = "";
					for (int j=0;j<baudList.count();j++)
					{
						for (int k=0;k<portList.count();k++)
						{
							if (baudList[j] != -1)
							{
								m_obd->openPort(portList[k].toStdString().c_str(),baudList[j]);
							}
							else
							{
								m_obd->openPort(portList[k].toStdString().c_str());
							}
							if (!initElm())
							{
								qDebug() << "Error";
								m_obd->closePort();
							}
							else
							{
								qDebug() << "Port Found!" << portList[k];
								m_obd->closePort();
								foundport = portList[k];
								k = portList.count();
								found = true;
							}
						}
					}
					if (found)
					{
						emit obdPortFound(foundport);
					}
				}

			}
			else if (m_reqClassListThreaded[i].type == REQ_SUPPORTED_PIDS)
			{
				if (!m_obdConnected)
				{
					if (!m_connect())
						continue;
				}
				emit consoleMessage("Requesting supported pids");

				std::vector<unsigned char> replyVector;


				if (!m_obd->sendObdRequest("0100\r",5,&replyVector))
				{
					qDebug() << "Error in requesting supported pids 0-20";
					continue;
				}
				QString rep = "Reply: ";
				for (unsigned int j=0;j<replyVector.size();j++)
				{
					rep += QString::number(replyVector[j],16).toUpper() + " ";
				}
				//qDebug() << "String reply:" << rep;
				unsigned var = (((unsigned int)replyVector[2] << 24) + ((unsigned int)replyVector[3] << 16) + ((unsigned int)replyVector[4] << 8) + (unsigned int)replyVector[5]);
				//qDebug() << "00 PID Reply:" << var;
				for (int j=2;j<6;j++)
				{
					//qDebug() << replyVector[i];
				}
				QList<QString> pids;
				for (int j=1;j<0x20;j++)
				{
					if (((var >> (0x20-j)) & 1))
					{
						//qDebug() << "Pid " << i << "Supported";
						pids.append(QString("01").append((j < 16) ? "0" : "").append(QString::number(j,16).toUpper()));
						//qDebug() << "Pid" << pids[pids.count()-1] << "supported";
						//pids[0].append(1);
					}
					else
					{
						//pids[0].append(0);
					}
				}


				if (!m_obd->sendObdRequest("0120\r",5,&replyVector))
				{
					qDebug() << "Error in requesting supported pids 20-24";
					continue;
				}
				rep = "Reply: ";
				for (unsigned int j=0;j<replyVector.size();j++)
				{
					rep += QString::number(replyVector[j],16).toUpper() + " ";
				}
				//qDebug() << "String reply:" << rep;
				var = (((unsigned int)replyVector[2] << 24) + ((unsigned int)replyVector[3] << 16) + ((unsigned int)replyVector[4] << 8) + (unsigned int)replyVector[5]);
				//qDebug() << "00 PID Reply:" << var;
				for (int j=2;j<6;j++)
				{
					//qDebug() << replyVector[i];
				}
				//QList<QString> pids;
				for (int j=1;j<0x20;j++)
				{
					if (((var >> (0x20-j)) & 1))
					{
						//qDebug() << "Pid " << i << "Supported";
						pids.append(QString("01").append(QString::number(j+(0x20),16).toUpper()));
						//qDebug() << "Pid" << pids[pids.count()-1] << "supported";
						//pids[0].append(1);
					}
					else
					{
						//pids[0].append(0);
					}
				}

				if (!m_obd->sendObdRequest("0140\r",5,&replyVector))
				{
					qDebug() << "Error in requesting supported pids 40-60";
					continue;
				}
				rep = "Reply: ";
				for (unsigned int j=0;j<replyVector.size();j++)
				{
					rep += QString::number(replyVector[j],16).toUpper() + " ";
				}
				//qDebug() << "String reply:" << rep;
				var = (((unsigned int)replyVector[2] << 24) + ((unsigned int)replyVector[3] << 16) + ((unsigned int)replyVector[4] << 8) + (unsigned int)replyVector[5]);
				//qDebug() << "00 PID Reply:" << var;
				for (int j=2;j<6;j++)
				{
					//qDebug() << replyVector[i];
				}
				for (int j=1;j<0x20;j++)
				{
					if (((var >> (0x20-j)) & 1))
					{
						//qDebug() << "Pid " << i << "Supported";
						pids.append(QString("01").append(QString::number(j+0x40,16).toUpper()));
						//qDebug() << "Pid" << pids[pids.count()-1] << "supported";
						//pids[0].append(1);
					}
					else
					{
						//pids[0].append(0);
					}
				}


				emit supportedPids(pids);

				if (!m_obdConnected)
				{
					m_obd->closePort();
					m_obdConnected = false;
					emit disconnected();
				}



			}
			else if (m_reqClassListThreaded[i].type == MODE_PID)
			{
				//qDebug() << "MODE_PID";
				if (m_obdConnected)
				{
					if (first)
					{
						first = false;
						cycleCounter++;
					}
					if ((cycleCounter % m_reqClassListThreaded[i].priority) == 0)
					{
						//qDebug() << "Making req" << m_reqList[currentReqNum].mid(0,m_reqList[currentReqNum].length()-1) << m_set << m_reqPriority[currentReqNum] << currentReqNum;
						//if (!m_obd->sendObdRequest(m_reqList[currentReqNum].toStdString().c_str(),m_reqList[currentReqNum].length(),&replyVector))
						QString req = (m_reqClassListThreaded[i].mode < 16) ? "0" : "";
						req += QString::number(m_reqClassListThreaded[i].mode,16).toUpper();
						req += (m_reqClassListThreaded[i].pid < 16) ? "0" : "";
						req += QString::number(m_reqClassListThreaded[i].pid,16).toUpper();
						//qDebug() << "Current request:" << req;
						QString pidreq = req;
						if (m_reqClassListThreaded[i].wait != 0)
						{
							req += " " + QString::number(m_reqClassListThreaded[i].wait);
						}
						req += "\r";
						if (!m_obd->sendObdRequest(req.toStdString().c_str(),req.length(),&replyVectorString))
						{
							if (m_obd->lastError() == obdLib::NODATA)
							{
								qDebug() << "OBD Request failed: NODATA for request" << req.mid(0,req.length()-1) << m_reqClassFailureMap[&m_reqClassListThreaded[i]];

								m_reqClassFailureMap[&m_reqClassListThreaded[i]]++;
								bool allislost=true;
								foreach(RequestClass* req,m_reqClassFailureMap.keys())
								{
									allislost &= m_reqClassFailureMap[req] > 0;
								}


								if (!allislost && !m_whiteList[&m_reqClassListThreaded[i]] && m_reqClassFailureMap[&m_reqClassListThreaded[i]] > 20)
								{
									qDebug() << "Reached Maximum errors for pid. Disabling: " << req.mid(0,req.length()-1);
									emit consoleMessage(QString("Reached maximum errors for pid ") + req.mid(0,req.length()-1) + QString(". Removing from request list."));
									m_reqClassFailureMap.remove(&m_reqClassListThreaded[i]);
									m_reqClassListThreaded.removeAt(i);
								}
								//m_errorCount[currentReqNum]++;
								//if (m_errorCount[currentReqNum] > 20)
								//{
								//	qDebug() << "Reached Maximum errors for pid" << m_reqList[currentReqNum].mid(0,m_reqList[currentReqNum].length()-1) << "shutting down requests for that pid";
								//	m_reqList.removeAt(currentReqNum);
								//	m_reqPriority.removeAt(currentReqNum);
								//	m_errorCount.removeAt(currentReqNum);
								//	emit consoleMessage(QString("Readed maximum errors for pid") + m_reqList[currentReqNum].mid(0,m_reqList[currentReqNum].length()-1) + "shutting down requests for that pid");
								//}

							}
							else if (m_obd->lastError() == obdLib::SERIALREADERROR || m_obd->lastError() == obdLib::SERIALWRITEERROR)
							{
								//errorCount++;
								qDebug() << "Serial read/write error on request" << req.mid(0,req.length()-1);
								emit consoleMessage("Serial read/write error on request" + req.mid(0,req.length()-1));
								//m_requestLoopRunning = false;

							}
							//Error here
						}
						else
						{
							m_reqClassFailureMap[&m_reqClassListThreaded[i]] = 0;
							m_whiteList[&m_reqClassListThreaded[i]] = true;
							//errorCount = 0;
							/*replyVector.clear();
							for (unsigned int i=0;i<replyVectorString.size()-1;i++)
							{
								replyVector.push_back(m_obd->byteArrayToByte(replyVectorString[i],replyVectorString[i+1]));
								i++;
							}
							*/
							ObdInfo::Pid *pid = m_obdInfo->getPidFromString(pidreq);
							if (pid)
							{
								/*QString rrtest="";
								for (int i=0;i<replyVectorString.size();i++)
								{
									rrtest += QString::number(replyVectorString.at(i),16).toUpper() + " ";
								}
								qDebug() << rrtest;
								*/
								QString func = pid->function;
								if (func.contains("A"))
								{
									func = func.replace(QString("A"),QString::number(replyVectorString[2]));
								}
								if (func.contains("B"))
								{
									func = func.replace(QString("B"),QString::number(replyVectorString[3]));
								}
								QString done = parse(func);
								//qDebug() <<"firing pid: "<<pidreq<<" val: "<<done;
								pidReceived(pidreq,done,cycleCounter,(((float)QDateTime::currentDateTime().toTime_t()) + ((float)QDateTime::currentDateTime().time().msec() / 1000.0)));
							}
							else
							{
								qDebug() << "Invalid Pid returned:" << req.mid(0,req.length()-1);
								emit consoleMessage(QString("Invalid PID returned from ObdInfo::getPidFromString(): ") + req.mid(0,req.length()-1));
							}
						}

					}
				}
			}
			if (!m_requestLoopRunning)
			{
				m_requestLoopRunning = true;
				i = m_reqClassListThreaded.count();
				continue;
			}
		}
		removePidMutex.unlock();
		if (m_reqClassListThreaded.size() == 0 || !m_obdConnected)
		{
			msleep(200);
		}
	}
}


void ObdThread::setPort(QString port)
{
	m_port = port;
}
void ObdThread::setBaud(int baud)
{
	m_baud = baud;
}
void ObdThread::addRequest(int mode, int pid, int priority,int wait)
{
	threadLockMutex.lock();
	RequestClass req;
	req.mode = mode;
	req.pid = pid;
	req.priority = priority;
	req.repeat = true;
	req.type = MODE_PID;
	req.wait = wait;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}

void ObdThread::getSupportedPids()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = REQ_SUPPORTED_PIDS;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}
void ObdThread::fullPidScan()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = SCAN_ALL;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}
void ObdThread::clearTroubleCodes()
{
	threadLockMutex.lock();
	RequestClass req;
	req.type = CLEAR_TROUBLE_CODES;
	m_reqClassList.append(req);
	threadLockMutex.unlock();
}

QString ObdThread::parse(QString str)
{
	if ((str[0] != '(' && str[str.length()-1] != ')'))
	{
		str = QString("(") + str + ")";
	}
	QString done = calc(str);
	//qDebug() << "Final Answer:" << done;
	return done;
}
bool ObdThread::initElm()
{
	//This tries to connect twice. The first time without a reset, the second time with a reset before all inits.
	std::vector<unsigned char> replyVector;
	for (int i=0;i<2;i++)
	{
		//These are to fix issues with certain devices like obdpro's tool.
		if (!m_obd->sendObdRequestString(CR,1,&replyVector,100,5));
		if (!m_obd->sendObdRequestString(CR,1,&replyVector,100,5));

		if (i == 1)
		{
			if (!resetElm())
			{
				emit consoleMessage("Error resetting ELM Device");
				qDebug() << "Error resetting ELM device";
				if (i == 1) return false;
			}
		}
		//Ensure echo, headers, and linefeeds are off.
		if (!echoOff())
		{
			emit consoleMessage("Error turning echo off");
			qDebug() << "Error turning echo off";
			if (i == 1) return false;
			continue;
		}
		if (!setHeaders(false))
		{
			emit consoleMessage("Error turning headers off");
			qDebug() << "Error turning headers off";
			if (i == 1) return false;
			continue;
		}
		if (!lineFeedOff())
		{
			emit consoleMessage("Error turning linefeeds off");
			qDebug() << "Error turning linefeeds off";
			if (i == 1) return false;
			continue;
		}

		if (!m_obd->sendObdRequestString("0100\r",5,&replyVector,100,10))
		{
			emit consoleMessage("Error in detecting protocol");
			qDebug() << "Error in finding protocol";
			if (i == 1) return false;
			continue;
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool ObdThread::resetElm()
{
	std::vector<unsigned char> replyVector;
	QString reply="";
	reply = "";
	replyVector.clear();
	if (!m_obd->sendObdRequestString("atz" CR,4,&replyVector,100,5))
	{
		qDebug() << "Error in reset sent";
	}
	for (unsigned int i=0;i<replyVector.size();i++)
	{
		reply += replyVector[i];
	}
	//qDebug() << "Reset Reply:" << reply;
	if (reply.contains("ELM"))
	{
		return true;
	}
	else if (reply.contains("atz"))
	{
		//Echoing here?
		qDebug() << "Reset echoed:" << reply;
		m_obd->flush();
		usleep(10000);
		return true;
	}
	else
	{
		qDebug() << "Reset:" << reply;
		return false;
	}
}
bool ObdThread::echoOff()
{
	std::vector<unsigned char> replyVector;
	QString reply="";
	m_obd->sendObdRequestString("ate0\r",5,&replyVector,20,3);
	reply = "";
	for (unsigned int i=0;i<replyVector.size();i++)
	{
		reply += replyVector[i];
	}
	if (reply.contains("OK"))
	{
		return true;
	}
	qDebug() << "Bad Echo:" << reply;
	return false;
}
bool ObdThread::setHeaders(bool on)
{
	std::vector<unsigned char> replyVector;
	QString reply="";
	if (on)
	{
		m_obd->sendObdRequestString("ath1\r",5,&replyVector,20,3);
	}
	else
	{
		m_obd->sendObdRequestString("ath0\r",5,&replyVector,20,3);
	}
	reply = "";
	for (unsigned int i=0;i<replyVector.size();i++)
	{
		reply += replyVector[i];
	}
	if (reply.contains("OK"))
	{
		return true;
	}
	return false;
}
bool ObdThread::lineFeedOff()
{
	std::vector<unsigned char> replyVector;
	QString reply="";
	m_obd->sendObdRequestString("atl0\r",5,&replyVector,20,3);
	reply = "";
	for (unsigned int i=0;i<replyVector.size();i++)
	{
		reply += replyVector[i];
	}
	if (reply.contains("OK"))
	{
		return true;
	}
	return false;
}
QString ObdThread::getElmVersion()
{
	std::vector<unsigned char> replyVector;
	QString reply="";
	m_obd->sendObdRequestString("ati\r",4,&replyVector,20,3);
	reply = "";
	for (unsigned int i=0;i<replyVector.size();i++)
	{
		reply += replyVector[i];
	//qDebug() << "Reply String:" << reply.replace("ati","");
	}
	return reply;
}
QString ObdThread::calc(QString str)
{
	//This is where the magic happens...
	//qDebug() << str;
	int start=-1;
	int stop=-1;
	for (int i=0;i<str.length();i++)
	{
		if (str[i] == '(')
		{
			if (start == -1)
				start = i+1;
		}
		if (str[i] == ')')
		{
				stop = i;
			//i = str.length();
		}
	}
	if (start != -1 && stop != -1)
	{
		QString tmp = calc(str.mid(start,stop-start));
		//qDebug() << "Old:" << str;
		str.replace(start-1,(stop-start)+2,tmp);
		//qDebug() << "New:" << str;
	}
	else
	{
		//qDebug() << "Inner function:" << str;
	}
	start = 0;
	stop = -1;
	//int op = 0;
	//int current;
	//"5+3*2+2-5*4/8;
	//5 3*2 2-5*4/8   + split
	//2 5*4/8     - split
	//5*4 8    / split
	//5 4   * split

	//"5+3*2+2-5*4/8;
	//5+3*2+2 5*4/8  - split
	//5 3*2 2-5*4/8   + split
	//5+3 2+2-5 4/8  * split

	for (int k=0;k<2;k++)
	{
	for (int i=0;i<str.length();i++)
	{
		if (k == 1)
		{
		if (str[i] == '+')
		{
			stop = -1;
			start = i;
			for (int j=i-1;j>=0;j--)
			{
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
					stop = j;
					j=0;
				}
			}
			if (stop == -1)
			{
				stop = 0;
			}
			int replacer = stop;
			QString tmp = str.mid(stop,start-stop);
			//qDebug() << "Found:" <<i<<stop<< tmp;
			stop = -1;
			for (int j=i+1;j<str.length();j++)
			{
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
					stop = j;
					j = str.length();
				}
			}
			if (stop == -1)
			{
				stop = str.length();
			}
			QString tmp2 = str.mid(start+1,stop-(start+1));
			//qDebug() << "Found2:" <<start<<stop<< tmp2;
			float first = tmp.toFloat();
			float second = tmp2.toFloat();
			str = str.replace(replacer,tmp.length()+tmp2.length()+1,QString::number(first+second));
			//qDebug() << str;
			i = 0;
		}
		if (str[i] == '-')
		{
			stop = -1;
			start = i;
			for (int j=i-1;j>=0;j--)
			{
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
					stop = j;
					j=0;
				}
			}
			if (stop == -1)
			{
				stop = 0;
			}
			int replacer = stop;
			QString tmp = str.mid(stop,start-stop);
			//qDebug() << "Found:" <<i<<stop<< tmp;
			stop = -1;
			for (int j=i+1;j<str.length();j++)
			{
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
					stop = j;
					j = str.length();
				}
			}
			if (stop == -1)
			{
				stop = str.length();
			}
			QString tmp2 = str.mid(start+1,stop-(start+1));
			//qDebug() << "Found2:" << tmp2;
			float first = tmp.toFloat();
			float second = tmp2.toFloat();
			str = str.replace(replacer,tmp.length()+tmp2.length()+1,QString::number(first-second));
			//qDebug() << str;
			i = 0;
		}
		}
		if (k == 0)
		{
		if (str[i] == '*')
		{
			stop = -1;
			start = i;
			for (int j=i-1;j>=0;j--)
			{
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
					stop = j;
					j=0;
				}
			}
			if (stop == -1)
			{
				stop = 0;
			}
			int replacer = stop;
			QString tmp = str.mid(stop,start-stop);
			//qDebug() << "Found:" << tmp;
			stop = -1;
			for (int j=i+1;j<str.length();j++)
			{
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
					stop = j;
					j = str.length();
				}
			}
			if (stop == -1)
			{
				stop = str.length();
			}
			QString tmp2 = str.mid(start+1,stop-(start+1));
			//qDebug() << "Found2:" << tmp2;
			float first = tmp.toFloat();
			float second = tmp2.toFloat();
			str = str.replace(replacer,tmp.length()+tmp2.length()+1,QString::number(first*second));
			//qDebug() << str;
			i = 0;
		}
		if (str[i] == '/')
		{
			stop = -1;
			start = i;
			for (int j=i-1;j>=0;j--)
			{
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
					stop = j;
					j=0;
				}
			}
			if (stop == -1)
			{
				stop = 0;
			}
			int replacer = stop;
			QString tmp = str.mid(stop,start-stop);
			//qDebug() << "Found:" << tmp;
			stop = -1;
			for (int j=i+1;j<str.length();j++)
			{
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
					stop = j;
					j = str.length();
				}
			}
			if (stop == -1)
			{
				stop = str.length();
			}
			QString tmp2 = str.mid(start+1,stop-(start+1));
			//qDebug() << "Found2:" << tmp2;
			float first = tmp.toFloat();
			float second = tmp2.toFloat();
			str = str.replace(replacer,tmp.length()+tmp2.length()+1,QString::number(first/second));
			//qDebug() << str;
			i = 0;
		}
		}
		//usleep(100000);
	}
	}
	return str;
}
bool ObdThread::m_connect()
{
	qDebug() << "Connecting...";
	if (m_obd->openPort(m_port.toStdString().c_str(),m_baud) < 0)
	{
		qDebug() << "Error opening OBD Port";
		emit liberror(ObdThread::UNABLE_TO_OPEN_COM_PORT);
		return false;
	}
	if (!initElm())
	{
		emit consoleMessage("Error in ELM init, port not opened");
		m_obd->closePort();
		//emit disconnected();
		//continue;
		return false;
	}
	//m_obdConnected = true;
	QString version = getElmVersion().replace("\r","").replace("\n","");
	emit consoleMessage(QString("Elm found. Version: ").append(version));
	qDebug() << "Connected to ELM version" << version;
	setProtocol(0,false);
	QString protocol = getProtocolName().replace("\r","").replace("\n","");
	qDebug() << "Connected protocol:" << protocol;
	emit protocolFound(protocol);
	emit connected(version);
	return true;
}
QString ObdThread::getProtocolName()
{
	std::vector<unsigned char> replyVector;
	QString reply="";
	m_obd->sendObdRequestString("0100\r",5,&replyVector,20,5);
	m_obd->sendObdRequestString("ATDP\r",5,&replyVector,100);
	reply = "";
	for (unsigned int i=0;i<replyVector.size();i++)
	{
		reply += replyVector[i];
	}
	//qDebug() << "Reply String:" << reply.replace("ati","");
	return reply;

}
void ObdThread::setProtocol(int num, bool autosearch)
{

	if (!m_obd->sendObdRequest("ATSP00\r",7,20))
	{
		qDebug() << "Error setting auto-protocol";
	}
}
