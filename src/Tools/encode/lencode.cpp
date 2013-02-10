/*
   (c) 2006 Werner Mayer LGPL
*/

#include <qtextcodec.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdom.h>

void encodeFile(QTextCodec *codec, const char *fileName);

int main(int argc, char *argv[])
{
  if (argc < 3) {
    qWarning("Usage: lencode encoding file1.ts...");
    return 1;
  }

  QTextCodec *codec = QTextCodec::codecForName(argv[1]);
  if (!codec) {
    qWarning("Unknown encoding: %s", argv[1]);
    return 1;
  }

  for (int i = 2; i < argc; ++i)
    encodeFile(codec, argv[i]);

  return 0;
}

void encodeFile(QTextCodec *codec, const char *fileName)
{
  QFile file(fileName);
  QDomDocument doc;

  if (!file.open(IO_ReadOnly | IO_Translate))
    ; // handle error
  if (!doc.setContent(&file, true))
    ; // handle error

  if (doc.firstChild().isProcessingInstruction() && doc.firstChild().nodeName() == "xml")
    doc.removeChild(doc.firstChild());

  QDomNode node = doc.createProcessingInstruction("xml",QString("version=\"1.0\" encoding=\"") + codec->mimeName() + "\"");
  doc.insertBefore(node, doc.firstChild());

  file.close();
  if (!file.open(IO_WriteOnly | IO_Translate))
    ; // handle error
  QTextStream out(&file);
  doc.save(out, 4);
}

