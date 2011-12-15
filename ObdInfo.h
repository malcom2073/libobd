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

#ifndef OBDINFO_H
#define OBDINFO_H
#include <QString>
#include <QList>
class ObdInfo
{
public:
	ObdInfo();
	struct Pid
	{
		QString stringRep;
		QString name;
		QString description;
		QString unit;
		QString function;
		float min;
		float max;
		unsigned char mode;
		unsigned char pid;
		int labels;
		int step;
	};
	QList<Pid*> pidList;
	Pid *getPidFromString(QString str);
	Pid *getPidFromBytes(int mode,int pid);
	static int intFromHex(QString num);
};
#endif //OBDINFO_H
