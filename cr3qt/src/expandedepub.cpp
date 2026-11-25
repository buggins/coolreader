/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2026 CoolReader contributors                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 ***************************************************************************/

#include "expandedepub.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMap>

#include <zlib.h>

#include <limits.h>

namespace {

void append16(QByteArray &target, quint16 value)
{
    target.append(static_cast<char>(value & 0xFF));
    target.append(static_cast<char>((value >> 8) & 0xFF));
}

void append32(QByteArray &target, quint32 value)
{
    target.append(static_cast<char>(value & 0xFF));
    target.append(static_cast<char>((value >> 8) & 0xFF));
    target.append(static_cast<char>((value >> 16) & 0xFF));
    target.append(static_cast<char>((value >> 24) & 0xFF));
}

bool skipFile(const QFileInfo &info)
{
    const QString name = info.fileName();
    return name == ".DS_Store" || name.startsWith("._");
}

QMap<QString, QString> epubFiles(const QString &source)
{
    QMap<QString, QString> result;
    QDirIterator it(source,
                    QStringList(),
                    QDir::Files | QDir::NoSymLinks | QDir::Readable | QDir::Hidden,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QFileInfo info(path);
        if (!info.isFile() || skipFile(info))
            continue;
        const QString relative = QDir(source).relativeFilePath(path).replace('\\', '/');
        if (relative == ".." || relative.startsWith("../"))
            continue;
        result.insert(relative, path);
    }
    return result;
}

QString directorySignature(const QString &source, const QMap<QString, QString> &files)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (QMap<QString, QString>::const_iterator it = files.constBegin(); it != files.constEnd(); ++it) {
        const QFileInfo info(it.value());
        hash.addData(it.key().toUtf8());
        hash.addData(QByteArray::number(info.size()));
        hash.addData(QByteArray::number(info.lastModified().toMSecsSinceEpoch()));
    }
    return QString::fromLatin1(hash.result().toHex());
}

quint16 dosTime(const QDateTime &dateTime)
{
    if (!dateTime.isValid())
        return 0;
    const QTime time = dateTime.time();
    return static_cast<quint16>((time.hour() << 11) | (time.minute() << 5) | (time.second() / 2));
}

quint16 dosDate(const QDateTime &dateTime)
{
    if (!dateTime.isValid())
        return 0;
    const QDate date = dateTime.date();
    const int year = qBound(1980, date.year(), 2107);
    return static_cast<quint16>(((year - 1980) << 9) | (date.month() << 5) | date.day());
}

bool deflateRaw(const QByteArray &input, QByteArray &output)
{
    if (static_cast<qulonglong>(input.size()) > UINT_MAX) {
        output.clear();
        return false;
    }

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS,
                     8, Z_DEFAULT_STRATEGY) != Z_OK)
        return false;

    const uLong bound = deflateBound(&stream, static_cast<uLong>(input.size()));
    if (bound > static_cast<uLong>(INT_MAX)) {
        deflateEnd(&stream);
        output.clear();
        return false;
    }

    output.resize(static_cast<int>(bound));
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.constData()));
    stream.avail_in = static_cast<uInt>(input.size());
    stream.next_out = reinterpret_cast<Bytef *>(output.data());
    stream.avail_out = static_cast<uInt>(output.size());

    const bool ok = deflate(&stream, Z_FINISH) == Z_STREAM_END;
    if (ok)
        output.resize(static_cast<int>(stream.total_out));
    else
        output.clear();
    deflateEnd(&stream);
    return ok;
}

bool writeFile(QFile &archive, const QByteArray &data, QString &error)
{
    if (data.isEmpty())
        return true;
    const qint64 written = archive.write(data.constData(), data.size());
    if (written != data.size()) {
        error = QObject::tr("Cannot write EPUB cache file");
        return false;
    }
    return true;
}

bool writeZip(const QString &source, const QString &target, QString &error)
{
    const QMap<QString, QString> files = epubFiles(source);
    if (!files.contains("mimetype") && !QFile::exists(source + "/META-INF/container.xml")) {
        error = QObject::tr("Not a valid EPUB folder: %1").arg(source);
        return false;
    }

    QFile archive(target);
    if (!archive.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = QObject::tr("Cannot create EPUB cache file: %1").arg(target);
        return false;
    }

    QByteArray central;
    quint32 entryCount = 0;
    qint64 offset = 0;

    const auto addEntry = [&](const QString &relativeName,
                              const QByteArray &nameBytes,
                              const QByteArray &data,
                              quint16 compression,
                              const QDateTime &timestamp) -> bool {
        if (static_cast<qulonglong>(data.size()) > UINT_MAX || offset > 0xFFFFFFFFLL
                || nameBytes.size() > 0xFFFF) {
            error = QObject::tr("EPUB folder is too large: %1").arg(source);
            return false;
        }

        const uLong initialCrc = crc32(0L, Z_NULL, 0);
        const quint32 crc = static_cast<quint32>(crc32(initialCrc,
                            reinterpret_cast<const Bytef *>(data.constData()),
                            static_cast<uInt>(data.size())));
        QByteArray compressed = data;
        if (compression == Z_DEFLATED && !deflateRaw(data, compressed)) {
            error = QObject::tr("Cannot compress EPUB entry: %1").arg(relativeName);
            return false;
        }

        const quint16 flags = 0x0800; // UTF-8 entry names
        QByteArray local;
        local.reserve(30 + nameBytes.size());
        append32(local, 0x04034b50);
        append16(local, 20);
        append16(local, flags);
        append16(local, compression);
        append16(local, dosTime(timestamp));
        append16(local, dosDate(timestamp));
        append32(local, crc);
        append32(local, static_cast<quint32>(compressed.size()));
        append32(local, static_cast<quint32>(data.size()));
        append16(local, static_cast<quint16>(nameBytes.size()));
        append16(local, 0);
        local.append(nameBytes);

        if (!writeFile(archive, local, error) || !writeFile(archive, compressed, error))
            return false;

        append32(central, 0x02014b50);
        append16(central, 20);
        append16(central, 20);
        append16(central, flags);
        append16(central, compression);
        append16(central, dosTime(timestamp));
        append16(central, dosDate(timestamp));
        append32(central, crc);
        append32(central, static_cast<quint32>(compressed.size()));
        append32(central, static_cast<quint32>(data.size()));
        append16(central, static_cast<quint16>(nameBytes.size()));
        append16(central, 0); // extra field
        append16(central, 0); // comment
        append16(central, 0); // disk number
        append16(central, 0); // internal attributes
        append32(central, 0); // external attributes
        append32(central, static_cast<quint32>(offset));
        central.append(nameBytes);

        offset += local.size() + compressed.size();
        ++entryCount;
        return true;
    };

    // EPUB requires "mimetype" to be the first entry and stored uncompressed.
    const QString mimePath = source + "/mimetype";
    QByteArray mimeData("application/epub+zip");
    QDateTime mimeTime = QDateTime::currentDateTime();
    if (QFile::exists(mimePath)) {
        QFile mimeFile(mimePath);
        if (mimeFile.open(QIODevice::ReadOnly)) {
            mimeData = mimeFile.readAll();
            mimeTime = QFileInfo(mimePath).lastModified();
        }
    }
    if (!addEntry("mimetype", QByteArray("mimetype"), mimeData, 0, mimeTime))
        return false;

    for (QMap<QString, QString>::const_iterator it = files.constBegin(); it != files.constEnd(); ++it) {
        if (it.key() == "mimetype")
            continue;
        QFile input(it.value());
        if (!input.open(QIODevice::ReadOnly)) {
            error = QObject::tr("Cannot read EPUB entry: %1").arg(it.value());
            return false;
        }
        const QByteArray data = input.readAll();
        input.close();
        const QDateTime timestamp = QFileInfo(it.value()).lastModified();
        if (!addEntry(it.key(), it.key().toUtf8(), data, Z_DEFLATED, timestamp))
            return false;
    }

    const quint32 centralSize = static_cast<quint32>(central.size());
    if (offset > 0xFFFFFFFFLL || centralSize > 0xFFFFFFFFU) {
        error = QObject::tr("EPUB folder is too large: %1").arg(source);
        return false;
    }
    QByteArray end;
    append32(end, 0x06054b50);
    append16(end, 0);
    append16(end, 0);
    append16(end, static_cast<quint16>(entryCount));
    append16(end, static_cast<quint16>(entryCount));
    append32(end, centralSize);
    append32(end, static_cast<quint32>(offset));
    append16(end, 0);

    if (!writeFile(archive, central, error) || !writeFile(archive, end, error))
        return false;
    if (archive.error() != QFile::NoError) {
        error = QObject::tr("Cannot write EPUB cache file: %1").arg(target);
        return false;
    }
    archive.close();
    return true;
}

} // namespace

bool cr3IsExpandedEpub(const QString &fileName)
{
    const QFileInfo info(fileName);
    return info.isDir()
            && info.suffix().compare("epub", Qt::CaseInsensitive) == 0;
}

QString cr3PrepareEpubFile(const QString &fileName, QString &error)
{
    error.clear();
    if (!cr3IsExpandedEpub(fileName))
        return fileName;

    const QString source = QFileInfo(fileName).absoluteFilePath();
    if (!QFile::exists(source + "/META-INF/container.xml")) {
        error = QObject::tr("Not a valid EPUB folder: %1").arg(source);
        return QString();
    }

    const QMap<QString, QString> files = epubFiles(source);
    const QString signature = directorySignature(source, files);

    const QString key = QString::fromLatin1(
        QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha1).toHex()).left(16);
    const QString cacheDir = QDir::homePath() + "/.cr3/applebooks/" + key;
    const QString cacheFile = cacheDir + "/" + QFileInfo(source).fileName();
    const QString signatureFile = cacheDir + "/.source-signature";

    if (QFile::exists(cacheFile) && QFile::exists(signatureFile)) {
        QFile sig(signatureFile);
        if (sig.open(QIODevice::ReadOnly) && QString::fromUtf8(sig.readAll().trimmed()) == signature)
            return cacheFile;
    }

    if (!QDir().mkpath(cacheDir)) {
        error = QObject::tr("Cannot create EPUB cache directory: %1").arg(cacheDir);
        return QString();
    }

    const QString tempFile = QString("%1/.cr3-pack-%2-%3.tmp")
        .arg(cacheDir)
        .arg(QCoreApplication::applicationPid())
        .arg(QDateTime::currentMSecsSinceEpoch());
    if (!writeZip(source, tempFile, error))
        return QString();

    if (QFile::exists(cacheFile) && !QFile::remove(cacheFile)) {
        error = QObject::tr("Cannot replace EPUB cache file: %1").arg(cacheFile);
        QFile::remove(tempFile);
        return QString();
    }
    if (!QFile::rename(tempFile, cacheFile)) {
        error = QObject::tr("Cannot create EPUB cache file: %1").arg(cacheFile);
        QFile::remove(tempFile);
        return QString();
    }

    QFile sig(signatureFile);
    if (!sig.open(QIODevice::WriteOnly | QIODevice::Truncate)
            || sig.write(signature.toUtf8()) != signature.toUtf8().size()) {
        error = QObject::tr("Cannot write EPUB cache signature: %1").arg(signatureFile);
        return QString();
    }
    sig.close();
    return cacheFile;
}
