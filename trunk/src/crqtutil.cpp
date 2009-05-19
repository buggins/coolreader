#include "crqtutil.h"

lString16 qt2cr(QString str)
{
    return lString16( str.toUtf8().constData() );
}

QString cr2qt(lString16 str)
{
    lString8 s8 = UnicodeToUtf8(str);
    return QString::fromUtf8( s8.c_str(), s8.length() );
}
