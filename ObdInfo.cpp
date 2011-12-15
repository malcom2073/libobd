/**************************************************************************
*   Copyright (C) 2010 by Michael Carpenter (malcom2073)                  *
*   mcarpenter@interforcesystems.com                                      *
*                                                                         *
*   This file is a part of libobd                                         *
*                                                                         *
*   libobd is free software: you can redistribute it and/or modify        *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   libobd is distributed in the hope that it will be useful,             *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with libobd.  If not, see <http://www.gnu.org/licenses/>.       *
***************************************************************************/

#include "ObdInfo.h"
#include <QXmlStreamReader>
#include <QFile>
#include <QDebug>
#include <math.h>
ObdInfo::ObdInfo()
{
	QString filename = "";
	if (QFile::exists("obd2data.dat"))
	{
		filename = "obd2data.dat";
	}
	else if (QFile::exists("/usr/share/libobd/obd2data.dat"))
	{
		filename = "/usr/share/libobd/obd2data.dat";
	}
	else
	{
		qDebug() << "Error! No obd2data.dat file found! Expect a segfault";
		return;
	}
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
	{
		qDebug() << "Unable to open obd2data.dat file! Expect a segfault";
		return;
	}
	QXmlStreamReader reader(file.readAll());
	file.close();
	int mode;
	while (!reader.atEnd())
	{
		if (reader.name() == "mode" && reader.isStartElement())
		{
			mode = reader.attributes().value("value").toString().toInt();
			reader.readNext();
			while (reader.name() != "mode")
			{
				if (reader.name() == "pids" && reader.isStartElement())
				{
					reader.readNext();
					while (reader.name() != "pids")
					{
						if (reader.name() == "pid" && reader.isStartElement())
						{
							Pid *pid = new Pid();
							pid->name = reader.attributes().value("name").toString();
							pid->description = reader.attributes().value("desc").toString();
							pid->unit = reader.attributes().value("unit").toString();
							pid->mode = mode;
							pid->pid = intFromHex(reader.attributes().value("value").toString());
							pid->function = reader.attributes().value("function").toString();
							pid->min = reader.attributes().value("min").toString().toFloat();
							pid->max = reader.attributes().value("max").toString().toFloat();
							pid->labels = reader.attributes().value("labels").toString().toInt();
							pid->step = reader.attributes().value("step").toString().toInt();
							pid->stringRep = QString((mode < 16) ? "0" : "").append(QString::number(mode,16).toUpper()).append((pid->pid < 16) ? "0" : "").append(QString::number(pid->pid,16).toUpper());
							pidList.append(pid);
						}
						reader.readNext();
					}
				}
				reader.readNext();
			}
		}
		reader.readNext();
	}
}

ObdInfo::Pid* ObdInfo::getPidFromString(QString str)
{
	for (int i=0;i<pidList.count();i++)
	{
		if (pidList[i]->stringRep == str)
		{
			return pidList[i];
		}
	}
	return 0;
}
ObdInfo::Pid *ObdInfo::getPidFromBytes(int mode,int pid)
{
	for (int i=0;i<pidList.count();i++)
	{
		if (pidList[i]->mode == mode && pidList[i]->pid == pid)
		{
			return pidList[i];
		}
	}
	return 0;
}
int ObdInfo::intFromHex(QString num)
{
	int final = 0;
	for (int i=0;i<num.length();i++)
	{
		if (num.at(i).toAscii() >=48 && num.at(i).toAscii() <=57)
		{
			if ((16 << (int)(4*((num.length()-2)-i))) == 0)
			{
				final += (num.at(i).toAscii() - 48);
			}
			else
			{
				final += (num.at(i).toAscii() - 48) * ((16 << (int)(4 * ((num.length()-2) - i))));
			}
		}
		else if (num.at(i).toUpper().toAscii() >= 65 && num.at(i).toUpper().toAscii() <= 70)
		{
			final += 10 + (num.at(i).toUpper().toAscii() - 65) * pow(16.0,(num.length()-1) - i);
		}
	}
	return final;
}
