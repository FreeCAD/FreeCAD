
#include "PreCompiled.h"
#include "mergedata.h"
#include <QFileDialog>

#include <QTextStream>
#include <QMessageBox>



MergeData::MergeData():
m_howmanypoints(0)
{

}

MergeData::~MergeData()
{

}





bool MergeData::Einlesen (const QStringList &dateinamen)
{
	for (int i=0;i<dateinamen.size();i++)
	{
		QFileInfo aFileInfo(dateinamen.at(i));
		QString path = aFileInfo.absolutePath();
		QDir::setCurrent(path);
		QFile input(dateinamen.at(i));

		if ( input.open ( QFile::ReadOnly ) )
		{
			QTextStream in ( &input );
			//Check the first line for a string-flag
			QString firstline = in.readLine();
			if(m_howmanypoints==0)
			{
				m_howmanypoints = firstline.toLong();
			}
			else
			{
				if(!(m_howmanypoints == firstline.toLong()))
				{
					return true;
				
				}
			}

			if(i==0)
			{
				m_mergedvalues.resize(m_howmanypoints);
				for(int j=0; j<m_howmanypoints; j++)
					m_mergedvalues[j].resize(6);
			}
			int zeile = 0;
			while ( !in.atEnd() )
			{
				QString line = in.readLine();
				QStringList fields = line.split ( QLatin1Char(','),QString::SkipEmptyParts );
				if(fields.size()<5)
					return true;

					m_mergedvalues[zeile][0] = fields[0].toFloat();
					m_mergedvalues[zeile][1] = fields[1].toFloat();
					m_mergedvalues[zeile][2] = fields[2].toFloat();
					m_mergedvalues[zeile][3] += fields[3].toFloat();
					m_mergedvalues[zeile][4] += fields[4].toFloat();
					m_mergedvalues[zeile][5] += fields[5].toFloat();

				zeile++;
			}
		}
		input.close();
	}

	return true;
}


bool MergeData::WriteOutput(const QString &dateiname)
{
	QFile anOutputFile(dateiname);
	if (!anOutputFile.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	QTextStream out(&anOutputFile);


	for(unsigned int i=0; i<m_mergedvalues.size(); i++)
	{
		out <<  m_mergedvalues[i][0]  << " " 
			<<  m_mergedvalues[i][1]  << " " 
			<<  m_mergedvalues[i][2]  << " " 
			<<  m_mergedvalues[i][3]  << " " 
			<<  m_mergedvalues[i][4]  << " " 
			<<  m_mergedvalues[i][5]  << endl;
	}

	anOutputFile.close();
	return true;
}


