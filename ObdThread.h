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


#ifndef OBDTHREAD_H
#define OBDTHREAD_H
#include <QThread>
#include <QString>
#include <QList>
#include <QDebug>
#include <QMutex>
#include "ObdInfo.h"
#include "obdlib.h"
#include <QByteArray>
#include <QVector>



#define LIBOBD_VERSION_MAJOR 0
#define LIBOBD_VERSION_MINOR 9
#define LIBOBD_VERSION_PATCH 0


void debugCallback(char *text,void *ptr);

class ObdThread : public QThread
{
	Q_OBJECT
public:
	//! OBD Library Errors
	/*! Thie enum is used to describe obd errors */
	enum ObdError
	{
		UNABLE_TO_OPEN_COM_PORT,
		READ_ERROR,
		WRITE_ERROR
	};

	//! Types of requests in the ObdThread event loop
	/*! These are generally only used internally, however they are available for use
for custom requests. */
	enum RequestType
	{
		TROUBLE_CODES,
		CLEAR_TROUBLE_CODES,
		MODE_PID,
		RAW_REQUEST,
		RAW_BLIND_REQUEST,
		CUSTOM,
		FREEZE_FRAME,
		NONE,
		REQ_SUPPORTED_PIDS,
		SCAN_ALL,
		CONNECT,
		DISCONNECT,
		START_REQ_LOOP,
		STOP_REQ_LOOP,
		FIND_PORT,
		VOLTAGE,
		SWITCH_BAUD,
		MONITOR_STATUS,
		MFG_STRING_ONE,
		ON_BOARD_MONITORS,
		REQ_SUPPORTED_MODES
	};
	enum CONTINUOUS_MONITOR
	{
		MISFIRE,
		FUEL_SYSTEM,
		COMPONENTS,
		CATALYST,
		HEATED_CATALYST,
		EVAPORATIVE_SYSTEM,
		SECONDARY_AIR_SYSTEM,
		AC_REFRIGERANT,
		OXYGEN_SENSOR,
		OXYGEN_SENSOR_HEATER,
		EGR_SYSTEM,
		NMHC_CAT,
		NOX_SCR_MONITOR,
		BOOST_PRESSURE,
		EXHAUST_GAS_SENSOR,
		PM_FILTER_MONITORING,
		EGR_VVT_SYSTEM
	};
	enum MONITOR_COMPLETE_STATUS
	{
		COMPLETE,
		INCOMPLETE,
		UNAVAILABLE
	};

	class RequestClass
	{
	public:
		RequestClass()
		{
			mode = 0;
			pid = 0;
			type = NONE;
			time = 0;
			priority=0;
			repeat = false;
			wait=0;
		}
		RequestClass(int pmode, int ppid, int ppriority, int pwait)
		{
			mode = pmode;
			pid = ppid;
			type = MODE_PID;
			wait = pwait;
			time = 0;
			priority = ppriority;
			repeat = true;
		}

		RequestType type;
		int mode;
		int wait;
		int pid;
		unsigned long time;
		int priority;
		bool repeat;
		QByteArray custom;
	};
	//! Constructor
	/*! Setting a parent is optional, since ObdThread generally lives for the
	entire life of the application */

	ObdThread(QObject *parent=0);


	void debug(QString msg,obdLib::DebugLevel level);
	void commsDebug(QString msg);
	void setDebugLevel(obdLib::DebugLevel level);

	//! Set the port on which ObdThread operatoes
	/*! This should be something like COM1, COM5, or //./COM20 on windows. /dev/ttyS0 or /dev/ttyUSB0 on *nix systems */

	void setPort(QString port);

	//! Set the baud rate which ObdThread opens the serial port with
	/*! Valid baud rates include 9600, 38400, 115200, and a few others. These three are the primary ones
	that most tools use. */

	void setBaud(int baud);

	//! Adds a repeat mode/pid request to the queue
	/*! This will add a request to the queue to be repeaidly requested. Example: obdThread->addRequest(0x01,0x0D,1,1);

	\param mode integer mode, 0x01 or 0x05 for instance
	\param pid integer pid, 0x0D for vehicle speed
	\param priority how often the pid gets requested. 1 is every cycle, 3 is every third cycle, etc
	\param wait This should be set to 0 unless you know what you're doing.*/

	void addRequest(int mode, int pid, int priority,int wait);

	//! Removes a repeat mode/pid request to the queue
	/*! This will remove a previously added mode/pid request that matches mode, pid, and priority.
	\param mode integer mode, 0x01 or 0x05 for instance
	\param pid integer pid, 0x0D for vehicle speed
	\param priority hot often the pid gets requested */

	void removeRequest(int mode, int pid, int priority);
	void addRequest(RequestClass req);
	void removeRequest(RequestClass req);
	void sendReqSupportedPids();
	//void setInfo(ObdInfo *info) { m_obdInfo = info; }
	ObdInfo *getInfo() { return m_obdInfo; }
	void connect();
	void disconnect();
	void clearReqList();
	void sendClearTroubleCodes();
	void sendReqOnBoardMonitors();
	void sendReqVoltage();
	void sendReqSupportedModes();
	void sendReqMfgString();
	void sendReqFullPidScan();
	void switchBaud();
	void sendSingleShotRequest(QByteArray request);
	void sendSingleShotBlindRequest(QByteArray request);
	void stopThread() { m_threadRunning = false; }
	void sendReqTroubleCodes();
	void start();
	void sendReqMonitorStatus();
	QString port() { return m_port; }
	void findObdPort();
	QString version() { return QString::number(LIBOBD_VERSION_MAJOR) + "." + QString::number(LIBOBD_VERSION_MINOR) + "." + QString::number(LIBOBD_VERSION_PATCH); }

protected:
	void run();
	void run2();
private:
	QStringList parseCode(QString code,QString type);
	obdLib::DebugLevel m_dbgLevel;
	QMutex threadLockMutex;
	QMutex loopTypeMutex;
	QMutex removePidMutex;
	bool m_obdConnected;
	ObdInfo *m_obdInfo;
	int loopType;
	QList<RequestClass> m_reqClassList;
	QList<RequestClass> m_reqClassListThreaded;
	QList<RequestClass> m_reqClassRemoveList;
	QMap<RequestClass*,int> m_reqClassFailureMap;
	QMap<RequestClass*,bool> m_whiteList;
	QList<QString> m_reqList;
	QList<int> m_reqPriority;
	QList<int> m_errorCount;
	QString m_port;
	QByteArray m_singleShotReqBytes;
	int m_baud;
	bool m_threadRunning;
	bool m_requestLoopRunning;
	obdLib *m_obd;
	int m_set;
	QString parse(QString str);
	QString calc(QString str);
	bool m_connect();
	bool initElm();
	bool resetElm();
	bool echoOff();
	bool setHeaders(bool on);
	bool lineFeedOff();
	QString getElmVersion();
	QString getProtocolName();
	void setProtocol(int num, bool autosearch);

signals:
	//void monitorTestReply(QList<QString> list);
	void monitorTestReply(QMap<ObdThread::CONTINUOUS_MONITOR,ObdThread::MONITOR_COMPLETE_STATUS> monitorlist);
	void onBoardMonitoringReply(QList<unsigned char> midlist,QList<unsigned char> tidlist,QList<QString> vallist,QList<QString> minlist,QList<QString> maxlist,QList<QString> passlist);
	void mfgStringReply(QString string);
	void liberror(ObdThread::ObdError err);
	void voltageReply(double volts);
	void supportedModesReply(QList<QString> list);
	void connected(QString version);
	void disconnected();
	void reqLoopStarted();
	void reqLoopStopped();
	void pidReply(QString pid,QString val,int set,double time);
	void singleShotReply(QByteArray request, QByteArray list);
	void supportedPidsReply(QList<QString> list);
	void troubleCodesReply(QString ecu, QList<QString> codes);
	void consoleMessage(QString message);
	void obdPortFound(QString portname);
	void protocolReply(QString protocol);
	void rawCommsMessage(QString msg);
	void debugMessage(QString msg,obdLib::DebugLevel lvl);
};
#endif //OBDTHREAD_H
