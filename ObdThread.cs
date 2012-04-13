/**************************************************************************
*   Copyright (C) 2012 by Michael Carpenter (malcom2073)                  *
*   malcom2073@gmail.com                                                  *
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.Threading;
namespace ObdLibSharp
{
    class ObdThread
    {

	#region Dll Calls to obdlib
	const string obdlibloc = "obdlib.dll";
	[DllImport(obdlibloc, EntryPoint = "obdLibNew")]
	public static extern IntPtr obdLibNew();
	[DllImport(obdlibloc, CallingConvention = CallingConvention.Cdecl)]
	public static extern int obdLibOpenPort(IntPtr ptr, String portname, Int32 baudrate);
	[DllImport(obdlibloc, CallingConvention = CallingConvention.Cdecl)]
	public static extern int obdLibInitPort(IntPtr ptr);
	[DllImport(obdlibloc, CallingConvention = CallingConvention.Cdecl)]
	public static extern void obdLibDelete(IntPtr ptr);
	[DllImport(obdlibloc, EntryPoint = "obdLibSendObdRequestString", CallingConvention = CallingConvention.Cdecl)]
	public static extern bool obdLibSendObdRequestString(IntPtr ptr, String req, int length, StringBuilder reply, int replylength, out int returnlength, int sleeptime, int timeout);
	[DllImport(obdlibloc, EntryPoint = "obdLibSendObdRequestString", CallingConvention = CallingConvention.Cdecl)]
	public static extern bool obdLibSendObdRequestStringTwo(IntPtr ptr, String req, Int32 length, byte[] reply, Int32 replylength, out Int32 returnlength, Int32 sleeptime, Int32 timeout);
	[DllImport(obdlibloc, CallingConvention = CallingConvention.Cdecl)]
	public static extern bool obdLibSendObdRequest(IntPtr ptr, String req, int length, StringBuilder reply, int replylength, int sleeptime, int timeout);
	[DllImport(obdlibloc, EntryPoint = "setEcho", CallingConvention = CallingConvention.Cdecl)]
	public static extern void setEcho(IntPtr ptr, bool on);
	[DllImport(obdlibloc, EntryPoint = "obdLibClosePort", CallingConvention = CallingConvention.Cdecl)]
	public static extern int obdLibClosePort(IntPtr ptr);
	#endregion

	class Pid
	{
	    public Pid(byte mode, byte pid, String shortname, String longname, String units, String calc, String strrep, int min, int max, bool bitencoded)
	    {
		modeInt = mode;
		pidInt = pid;
		calcStr = calc;
	    }
	    public byte modeInt;
	    public byte pidInt;
	    public string calcStr;

	}
	List<Pid> pidList;
	Thread m_internalThread;
	public ObdThread()
	{
	    m_internalThread = new Thread(new ThreadStart(threadloop));

	    pidList = new List<Pid>();
	    reqListMutex = new Mutex();
	    reqList = new List<ReqClass>();
	    m_threadReqList = new List<ReqClass>();
	    reqRmList = new List<ReqClass>();
	    m_threadRmReqList = new List<ReqClass>();
	    pidList.Add(new Pid(0x01, 0x00, "SupportedPids", "Supported Pids 0x00-0x1F", "", "", "0100", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x01, "MonitorStatus", "Monitor Status since DTC Cleared", "", "", "0101", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x02, "FreezeDTC", "Freeze Frames DTC Codes", "", "", "0102", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x03, "Fuel Status", "Fuel System Status", "", "", "0103", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x04, "Load", "Calculated Engine Load", "%", "A*(100/255)", "0104", 0, 100, false));
	    pidList.Add(new Pid(0x01, 0x05, "Temp", "Coolant Temperature", "C", "A-40", "0105", -40, 215, false));
	    pidList.Add(new Pid(0x01, 0x06, "STFT-B1", "Short Term Fuel Trim - Bank 1", "%", "(A-128)*100/128", "0106", -100, 100, false));
	    pidList.Add(new Pid(0x01, 0x07, "LTFT-B1", "Long Term Fuel Trim - Bank 1", "%", "(A-128)*100/128", "0107", -100, 100, false));
	    pidList.Add(new Pid(0x01, 0x08, "STFT-B2", "Short Term Fuel Trim - Bank 2", "%", "(A-128)*100/128", "0108", -100, 100, false));
	    pidList.Add(new Pid(0x01, 0x09, "LTFT-B2", "Long Term Fuel Trim - Bank 2", "%", "(A-128)*100/128", "0109", -100, 100, false));
	    pidList.Add(new Pid(0x01, 0x0A, "Fuel Pressure", "Fuel Pressure", "kPa", "A*3", "010A", 0, 765, false));
	    pidList.Add(new Pid(0x01, 0x0B, "MAP", "Manifold Absolute Pressure", "kPa", "A", "010B", 0, 255, false));
	    pidList.Add(new Pid(0x01, 0x0C, "RPM", "Engine Speed", "rpm", "((A*256) + B) / 4", "010C", 0, 17000, false));
	    pidList.Add(new Pid(0x01, 0x0D, "Speed", "Vehicle Speed", "kph", "A", "010D", 0, 255, false));
	    pidList.Add(new Pid(0x01, 0x0E, "Timing", "Timing Advance", "degrees", "(A/2)-64", "010E", -64, 64, false));
	    pidList.Add(new Pid(0x01, 0x0F, "Intake Air", "Intake Air Temp", "C", "A-40", "010F", -40, 215, false));
	    pidList.Add(new Pid(0x01, 0x10, "MAF", "Mass Air Flow", "g/s", "((A*256) + B) / 100", "0110", 0, 656, false));
	    pidList.Add(new Pid(0x01, 0x11, "Throttle", "Throttle Position", "%", "A*(100/255)", "0111", 0, 100, false));
	    pidList.Add(new Pid(0x01, 0x12, "Secondary Air", "Commanded Secondary Air Status", "b", "", "0112", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x13, "O2 Sensors Present", "Oxygen Sensors Present", "", "", "0113", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x14, "O2B1S1Voltage", "Bank 1 Sensor 1 O2 Voltage", "", "", "0114", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x15, "O2B1S2Voltage", "Bank 1 Sensor 2 O2 Voltage", "", "", "0115", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x16, "O2B1S3Voltage", "Bank 1 Sensor 3 O2 Voltage", "", "", "0116", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x17, "O2B1S4Voltage", "Bank 1 Sensor 4 O2 Voltage", "", "", "0117", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x18, "O2B2S1Voltage", "Bank 2 Sensor 1 O2 Voltage", "", "", "0118", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x19, "O2B2S2Voltage", "Bank 2 Sensor 2 O2 Voltage", "", "", "0119", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x1A, "O2B2S3Voltage", "Bank 2 Sensor 3 O2 Voltage", "", "", "011A", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x1B, "O2B2S4Voltage", "Bank 2 Sensor 4 O2 Voltage", "", "", "011B", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x1C, "OBDStandards", "OBD Standards this vehicle conforms to", "", "", "011C", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x1D, "O2Sensors", "Oxygen Sensors Present", "", "", "011D", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x1E, "AuxInput", "Auxillary Input Status", "", "", "011E", 0, 0, true));
	    pidList.Add(new Pid(0x01, 0x2F, "FuelLevel", "Fuel Level Input", "%", "A*(100/255)", "012F", 0, 100, false));
	    pidList.Add(new Pid(0x01, 0x46, "AmbientAir", "Ambient Air Temp", "C", "A-40", "0146", -40, 215, false));
	    m_internalThread.Start();
	}


	class ReqClass
	{
	    public enum RequestType
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
	    }
	    public ReqClass()
	    {
		repeat = false;
	    }
	    public RequestType type;
	    public String mode;
	    public String pid;
	    public bool repeat;
	}
	byte[] retBuffer = new byte[1024];
	public void setPort(String portname)
	{
	    m_portName = portname;
	}
	public void setBaud(int baud)
	{
	    m_baudRate = baud;
	}
	int m_baudRate;
	string m_portName;
	void threadloop()
	{
	    ptr = obdLibNew();
	    while (true)
	    {
		reqListMutex.WaitOne();
		for (int i = 0; i < reqList.Count; i++)
		{
		    m_threadReqList.Add(reqList[i]);
		}
		for (int i = 0; i < reqRmList.Count; i++)
		{
		    m_threadRmReqList.Add(reqRmList[i]);
		}
		reqRmList.Clear();
		reqList.Clear();
		reqListMutex.ReleaseMutex();

		for (int i = 0; i < m_threadReqList.Count; i++)
		{
		    if (m_threadReqList[i].type == ReqClass.RequestType.CONNECT)
		    {
			//int rep1 = obdLibOpenPort(ptr, "\\\\.\\CNCB0", 115200);
			int rep1 = obdLibOpenPort(ptr, m_portName, m_baudRate);
			obdLibSendObdRequest(ptr, "atz\r", 4, new StringBuilder(), 1024, 150, 2000);
			obdLibSendObdRequest(ptr, "ate0\r", 5, new StringBuilder(), 1024, 20, 1000);
			obdLibSendObdRequest(ptr, "atl0\r", 5, new StringBuilder(), 1024, 20, 1000);
			obdLibSendObdRequest(ptr, "ath0\r", 5, new StringBuilder(), 1024, 20, 1000);
			//int rep2 = obdLibInitPort(ptr);
			//StringBuilder reply = new StringBuilder();

			//bool rep = obdLibSendObdRequestString(ptr, "0100\r", 5, reply, 1024, 100, 5);
		    }
		    else if (m_threadReqList[i].type == ReqClass.RequestType.DISCONNECT)
		    {
			obdLibClosePort(ptr);
		    }
		    else if (m_threadReqList[i].type == ReqClass.RequestType.MODE_PID)
		    {
			//StringBuilder reply = new StringBuilder();
			//reply.Clear();
			//Clear things out.
			String reqstring = m_threadReqList[i].mode + m_threadReqList[i].pid + "\r";
			//bool rep = obdLibSendObdRequest(ptr, reqstring, 5, reply, 1024, 200, 500);
			int len = 0;
			bool rep = false;
			rep = obdLibSendObdRequestStringTwo(ptr, reqstring, 5, retBuffer, 1024, out len, 10, 1000);
			if (!rep)
			{
			    //Bad response
			}
			else
			{
			    //Good response

			}
			String val = "";
			if (len > 4)
			{
			    String reply = Encoding.Default.GetString(retBuffer, 0, len);
			    reply = reply.Replace("\r", "").Replace("\n", "").Replace(" ", "");
			    if (reply[0] == '4' && reply[1] == '1')
			    {
				//Chances are it's a good string.

				String modepid = m_threadReqList[i].mode + m_threadReqList[i].pid;
				byte modebyte = byteArrayToByte((byte)modepid[0], (byte)modepid[1]);
				byte pidbyte = byteArrayToByte((byte)modepid[2], (byte)modepid[3]);
				Pid curpid = null;
				for (int j = 0; j < pidList.Count; j++)
				{
				    if (pidList[j].modeInt == modebyte && pidList[j].pidInt == pidbyte)
				    {
					curpid = pidList[j];
				    }
				}
				if (curpid != null)
				{
				    String calcstr = curpid.calcStr;
				    if (calcstr.Contains("A"))
				    {
					calcstr = calcstr.Replace("A", byteArrayToByte((byte)reply[4], (byte)reply[5]).ToString());
				    }
				    if (calcstr.Contains("B"))
				    {
					calcstr = calcstr.Replace("B", byteArrayToByte((byte)reply[6], (byte)reply[7]).ToString());
				    }
				    if (calcstr.Contains("C"))
				    {
					calcstr = calcstr.Replace("C", byteArrayToByte((byte)reply[8], (byte)reply[9]).ToString());
				    }
				    if (calcstr.Contains("D"))
				    {
					calcstr = calcstr.Replace("D", byteArrayToByte((byte)reply[10], (byte)reply[11]).ToString());
				    }
				    String retval = calc(calcstr);

				    pidReply(modepid, retval, 0, 0);
				}
			    }
			}
			//rep = obdLibSendObdRequest(ptr, "0100\r", 5, reply, 1024, 100, 5);
		    }
		}
		for (int i = 0; i < m_threadReqList.Count; i++)
		{
		    for (int j = 0; j < m_threadRmReqList.Count; j++)
		    {
			if (m_threadRmReqList[j].mode == m_threadReqList[i].mode && m_threadRmReqList[j].pid == m_threadReqList[i].pid)
			{

			    m_threadReqList.RemoveAt(i);
			    i--;
			}
		    }
		    if (m_threadReqList.Count == 0)
		    {
			break;
		    }
		    if (!m_threadReqList[i].repeat)
		    {
			m_threadReqList.RemoveAt(i);
			i--;
		    }
		}
		m_threadRmReqList.Clear();
	    }
	}
	Mutex reqListMutex;
	List<ReqClass> reqList;
	List<ReqClass> reqRmList;
	List<ReqClass> m_threadReqList;
	List<ReqClass> m_threadRmReqList;
	public void addModePidReq(String mode, String pid)
	{
	    ReqClass req = new ReqClass();
	    req.type = ReqClass.RequestType.MODE_PID;
	    req.mode = mode;
	    req.pid = pid;
	    req.repeat = true;
	    reqListMutex.WaitOne();
	    reqList.Add(req);
	    reqListMutex.ReleaseMutex();
	}
	public void delModePidReq(String mode, String pid)
	{
	    ReqClass req = new ReqClass();
	    req.type = ReqClass.RequestType.MODE_PID;
	    req.mode = mode;
	    req.pid = pid;
	    req.repeat = true;
	    reqListMutex.WaitOne();
	    reqRmList.Add(req);
	    reqListMutex.ReleaseMutex();
	}
	public void connect()
	{
	    ReqClass req = new ReqClass();
	    req.type = ReqClass.RequestType.CONNECT;
	    reqListMutex.WaitOne();
	    reqList.Add(req);
	    reqListMutex.ReleaseMutex();
	}
	public void disconnect()
	{
	    ReqClass req = new ReqClass();
	    req.type = ReqClass.RequestType.DISCONNECT;
	    reqListMutex.WaitOne();
	    reqList.Add(req);
	    reqListMutex.ReleaseMutex();
	}
	IntPtr ptr;
	public delegate void pidReplyDelegate(String mode, String val, int set, double time);
	public event pidReplyDelegate pidReply;

	#region Here there be monsters
	public string calc(string str)
	{
	    //This is where the magic happens...
	    //Don't EVER touch this function. It has no bugs, and is very painful.
	    //qDebug() << str;
	    int start = -1;
	    int stop = -1;
	    for (int i = 0; i < str.Length; i++)
	    {
		if (str[i] == '(')
		{
		    if (start == -1)
			start = i + 1;
		}
		if (str[i] == ')')
		{
		    stop = i;
		    //i = str.length();
		}
	    }
	    if (start != -1 && stop != -1)
	    {
		String tmp = calc(str.Substring(start, stop - start));
		//qDebug() << "Old:" << str;
		//str = str.Replacestr.replace(start-1,(stop-start)+2,tmp);
		str = str.Remove(start - 1, (stop - start) + 2);
		str = str.Insert(start - 1, tmp);
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

	    for (int k = 0; k < 2; k++)
	    {
		for (int i = 0; i < str.Length; i++)
		{
		    if (k == 1)
		    {
			if (str[i] == '+')
			{
			    stop = -1;
			    start = i;
			    for (int j = i - 1; j >= 0; j--)
			    {
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
				    stop = j;
				    j = 0;
				}
			    }
			    if (stop == -1)
			    {
				stop = 0;
			    }
			    int replacer = stop;
			    String tmp = str.Substring(stop, start - stop);
			    //qDebug() << "Found:" <<i<<stop<< tmp;
			    stop = -1;
			    for (int j = i + 1; j < str.Length; j++)
			    {
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
				    stop = j;
				    j = str.Length;
				}
			    }
			    if (stop == -1)
			    {
				stop = str.Length;
			    }
			    String tmp2 = str.Substring(start + 1, stop - (start + 1));
			    //qDebug() << "Found2:" <<start<<stop<< tmp2;
			    float first = float.Parse(tmp);
			    float second = float.Parse(tmp2);
			    //float first = tmp.toFloat();
			    //float second = tmp2.toFloat();
			    //str = str.replace(replacer,tmp.length()+tmp2.length()+1,QString::number(first+second));
			    str = str.Remove(replacer, tmp.Length + tmp2.Length + 1);
			    str = str.Insert(replacer, (first + second).ToString());
			    //qDebug() << str;
			    i = 0;
			}
			if (str[i] == '-')
			{
			    stop = -1;
			    start = i;
			    for (int j = i - 1; j >= 0; j--)
			    {
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
				    stop = j;
				    j = 0;
				}
			    }
			    if (stop == -1)
			    {
				stop = 0;
			    }
			    int replacer = stop;
			    String tmp = str.Substring(stop, start - stop);
			    //qDebug() << "Found:" <<i<<stop<< tmp;
			    stop = -1;
			    for (int j = i + 1; j < str.Length; j++)
			    {
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
				    stop = j;
				    j = str.Length;
				}
			    }
			    if (stop == -1)
			    {
				stop = str.Length;
			    }
			    //QString tmp2 = str.mid(start+1,stop-(start+1));
			    String tmp2 = str.Substring(start + 1, stop - (start + 1));
			    //qDebug() << "Found2:" << tmp2;
			    float first = float.Parse(tmp);
			    float second = float.Parse(tmp2);
			    //float first = tmp.toFloat();
			    //float second = tmp2.toFloat();
			    //str = str.replace(replacer,tmp.length()+tmp2.length()+1,QString::number(first-second));
			    str = str.Remove(replacer, tmp.Length + tmp2.Length + 1);
			    str.Insert(replacer, (first - second).ToString());
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
			    for (int j = i - 1; j >= 0; j--)
			    {
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
				    stop = j;
				    j = 0;
				}
			    }
			    if (stop == -1)
			    {
				stop = 0;
			    }
			    int replacer = stop;
			    //QString tmp = str.mid(stop,start-stop);
			    String tmp = str.Substring(stop, start - stop);
			    //qDebug() << "Found:" << tmp;
			    stop = -1;
			    for (int j = i + 1; j < str.Length; j++)
			    {
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
				    stop = j;
				    j = str.Length;
				}
			    }
			    if (stop == -1)
			    {
				stop = str.Length;
			    }
			    //QString tmp2 = str.mid(start+1,stop-(start+1));
			    String tmp2 = str.Substring(start + 1, stop - (start + 1));
			    //qDebug() << "Found2:" << tmp2;
			    float first = float.Parse(tmp);
			    float second = float.Parse(tmp2);
			    //float first = tmp.toFloat();
			    //float second = tmp2.toFloat();
			    //str = str.replace(replacer,tmp.length()+tmp2.length()+1,QString::number(first*second));
			    str = str.Remove(replacer, tmp.Length + tmp2.Length + 1);
			    str = str.Insert(replacer, (first * second).ToString());
			    //qDebug() << str;
			    i = 0;
			}
			if (str[i] == '/')
			{
			    stop = -1;
			    start = i;
			    for (int j = i - 1; j >= 0; j--)
			    {
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
				    stop = j;
				    j = 0;
				}
			    }
			    if (stop == -1)
			    {
				stop = 0;
			    }
			    int replacer = stop;
			    //QString tmp = str.mid(stop,start-stop);
			    String tmp = str.Substring(stop, start - stop);
			    //qDebug() << "Found:" << tmp;
			    stop = -1;
			    for (int j = i + 1; j < str.Length; j++)
			    {
				if (str[j] == '+' || str[j] == '-' || str[j] == '/' || str[j] == '*')
				{
				    stop = j;
				    j = str.Length;
				}
			    }
			    if (stop == -1)
			    {
				stop = str.Length;
			    }
			    //QString tmp2 = str.mid(start+1,stop-(start+1));
			    String tmp2 = str.Substring(start + 1, stop - (start + 1));
			    //qDebug() << "Found2:" << tmp2;
			    float first = float.Parse(tmp);
			    float second = float.Parse(tmp2);
			    //float first = tmp.toFloat();
			    //float second = tmp2.toFloat();
			    //str = str.replace(replacer,tmp.length()+tmp2.length()+1,QString::number(first/second));
			    str = str.Remove(replacer, tmp.Length + tmp2.Length + 1);
			    str = str.Insert(replacer, (first / second).ToString());
			    //qDebug() << str;
			    i = 0;
			}
		    }
		    //usleep(100000);
		}
	    }
	    return str;
	}

	public byte byteArrayToByte(byte b1, byte b2)
	{
	    byte newB1 = 0;
	    byte newB2 = 0;
	    if ((b1 >= 48) && (b1 <= 57))
	    {
		newB1 = (byte)((b1 - 48) * 16);
	    }
	    else if ((b1 >= 65) && (b1 <= 90))
	    {
		newB1 = (byte)((b1 - 55) * 16);
	    }
	    else
	    {
		newB1 = (byte)((b1 - 87) * 16);
	    }
	    if ((b2 >= 48) && (b2 <= 57))
	    {
		newB2 = (byte)(b2 - 48);
	    }
	    else if ((b2 >= 65) && (b2 <= 90))
	    {
		newB2 = (byte)(b2 - 55);
	    }
	    else
	    {
		newB2 = (byte)(b2 - 87);
	    }
	    byte retVal = (byte)(newB1 + newB2);
	    return retVal;
	}
	#endregion
    }
}
