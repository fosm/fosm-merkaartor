//***************************************************************
// CLass: NavitBin
//
// Description:
//
//
// Author: Chris Browet <cbro@semperpax.com> (C) 2010
//
// Copyright: See COPYING file that comes with this distribution
//
//******************************************************************

#include "NavitBin.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>

#include <QDataStream>
#include <QPair>

#include <math.h>

NavitBin::NavitBin()
    : zip(NULL)
{
}

NavitBin::~NavitBin()
{
    if (zip) {
        zip_close(zip);
        zip = NULL;
    }
}

bool NavitBin::setFilename(const QString& filename)
{
    zip = zip_open(filename.toUtf8().data(), 0, NULL);
    if(!zip) {
        QMessageBox::critical(0,QCoreApplication::translate("NavitBackground","Not a valid file"),QCoreApplication::translate("NavitBackground","Cannot open file."));
        return false;
    }
    qDebug() << "num files in zip: " << zip_get_num_files(zip);
    int idx = locateTile("index");
    if (idx == -1) {
        QMessageBox::critical(0,QCoreApplication::translate("NavitBackground","Not a valid file"),QCoreApplication::translate("NavitBackground","Cannot locate index."));
        return false;
    }
    if (!readTile(idx)) {
        QMessageBox::critical(0,QCoreApplication::translate("NavitBackground","Not a valid file"),QCoreApplication::translate("NavitBackground","Cannot read index."));
        return false;
    }
    indexTile = theTiles[idx];

    return true;
}

int NavitBin::locateTile(QString fn) const
{
    return zip_name_locate(zip, fn.toUtf8().data(), ZIP_FL_NOCASE | ZIP_FL_NODIR);
}

bool NavitBin::readTile(int aIndex) const
{
//    qDebug() << "Reading: "  << aIndex;

    struct zip_stat fs;
    zip_stat_index(zip, aIndex, ZIP_FL_NOCASE | ZIP_FL_NODIR, &fs);
    struct zip_file* file = zip_fopen_index(zip, aIndex, ZIP_FL_NOCASE | ZIP_FL_NODIR);
    if (!file) {
        QMessageBox::critical(0,QCoreApplication::translate("NavitBackground","Not a valid file"),QCoreApplication::translate("NavitBackground","Cannot load file."));
        return false;
    }

    NavitTile aTile;

    qint32 len;
    quint32 type;
    qint32 coordLen;
    qint32 x, y;
    qint32 attrLen;
    quint32 attrType;
    qint8 attr;
    quint16 orderMin;
    quint16 orderMax;

    QByteArray ba(fs.size, 0);
    zip_fread(file, ba.data(), fs.size);
    QDataStream data(ba);
    data.setByteOrder(QDataStream::LittleEndian);
    while (!data.atEnd()) {
        NavitFeature aFeat;
        data >> len;
        data >> type;
//        qDebug() << "-- type: " << QString("%1").arg(type, 0, 16);
        aFeat.type = type;
        data >> coordLen;
        for (int i=0; i<coordLen/2; ++i) {
            data >> x >> y;
            aFeat.coordinates << QPoint(x, y);
        }
        for (int j=2+1+coordLen; j<len; j+=2) {
            QByteArray attribute;
            data >> attrLen;
            data >> attrType;
//            qDebug() << "-- attrType: " << QString("%1").arg(attrType, 0, 16);
            switch (type) {
            case type_submap: {
                NavitPointer ptr;

                switch (attrType) {
                case attr_zipfile_ref: {
                        quint32 zipref;
                        data >> zipref;
                        if (coordLen >= 2) {
                            ptr.box = QRect(aFeat.coordinates[0], aFeat.coordinates[1]);
                            ptr.zipref = zipref;
                        } else {
                            ptr.box = QRect();
                            ptr.zipref = zipref;
                        }
                        break;
                    }

                case attr_order: {
                    data >> orderMin;
                    data >> orderMax;
                    ptr.orderMin = orderMin;
                    ptr.orderMax = orderMax;
                    break;
                }

                default:
                    for (int i=0; i<(attrLen-1)*sizeof(qint32); ++i) {
                        data >> attr;
                        attribute.append(attr);
                    }
                    aFeat.attributes << NavitAttribute(attrType, attribute);
                    break;
                }
                aTile.pointers.append(ptr);

                break;
            }

            case type_countryindex: {
                NavitPointer ptr;

                switch (attrType) {
                case attr_zipfile_ref: {
                        quint32 zipref;
                        data >> zipref;
                        if (coordLen >= 2) {
                            ptr.box = QRect(aFeat.coordinates[0], aFeat.coordinates[1]);
                            ptr.zipref = zipref;
                        } else {
                            ptr.box = QRect();
                            ptr.zipref = zipref;
                        }
                        break;
                    }

                case attr_order: {
                    data >> orderMin;
                    data >> orderMax;
                    ptr.orderMin = orderMin;
                    ptr.orderMax = orderMax;
                    break;
                }

                case attr_country_id: {
                    quint32 ctry_id;
                    data >> ctry_id;
                    qDebug() << "Country id: " << ctry_id;
                }

                default:
                    for (int i=0; i<(attrLen-1)*sizeof(qint32); ++i) {
                        data >> attr;
                        attribute.append(attr);
                    }
                    aFeat.attributes << NavitAttribute(attrType, attribute);
                    break;
                }
                aTile.pointers.append(ptr);

                break;
            }

            default:
                for (int i=0; i<(attrLen-1)*sizeof(qint32); ++i) {
                    data >> attr;
                    attribute.append(attr);
                }
                aFeat.attributes << NavitAttribute(attrType, attribute);
                break;
            }
            j += attrLen-1;
        }
        aTile.features.append(aFeat);
    }
    zip_fclose(file);

    theTiles[aIndex] = aTile;

    return true;
}

//bool NavitBin::getFeatures(const QString& tileRef, QList <NavitFeature>& theFeats) const
//{
//    readTile(tileRef);
//    NavitTile t = theTiles[tileRef];
//    foreach(NavitFeature f, t.features) {
//        if ((f.type & 0x00010000) == 0x00010000) { // POI
//            theFeats.append(f);
//        } else if ((f.type & 0xc0000000) == 0xc0000000) { // Area
//            theFeats.append(f);
//        } else if ((f.type & 0x80000000) == 0x80000000) { // Line
//            theFeats.append(f);
//        }
//    }

//}

bool NavitBin::walkTiles(const QRect& pBox, const NavitTile& theTile, QList <NavitFeature>& theFeats) const
{
    qreal r = 40030174. / pBox.width();
    int order = log2(r);
    qDebug() << "order: " << order;

    NavitTile t = theTile;
    foreach(NavitFeature f, t.features) {
        if ((f.type & 0x00010000) == 0x00010000) { // POI
            theFeats.append(f);
        } else if ((f.type & 0xc0000000) == 0xc0000000) { // Area
            theFeats.append(f);
        } else if ((f.type & 0x80000000) == 0x80000000) { // Line
            theFeats.append(f);
        }
    }
    for (int i=t.pointers.size()-1; i>=0; --i) {
        if (t.pointers[i].box.intersects(pBox) && order>t.pointers[i].orderMin && order<t.pointers[i].orderMax) {
            readTile(t.pointers[i].zipref);
            NavitTile ti = theTiles[t.pointers[i].zipref];
            walkTiles(pBox, ti, theFeats);
        }
    }
    return true;
}

bool NavitBin::getFeatures(const QRect& pBox, QList <NavitFeature>& theFeats) const
{
    NavitTile t = indexTile;
    return walkTiles(pBox, t, theFeats);
}

//bool NavitBin::getFeatures(const QRect& pBox, QList <NavitFeature>& theFeats) const
//{
//    QString tileRef;
//    tileRef.fill('_', 14);
//    QRect tileRect = QRect(QPoint(-20015087, -20015087), QPoint(20015087, 20015087));

//    int lvl = -1;
//    bool ok = false;
//    while (lvl < 13) {
//        ++lvl;
//        QSize tmpSize = tileRect.size() /2;
//        //        qDebug() << "ref: " << tileRef << "; rect: " << tileRect << "; sz: " << tmpSize;
//        QRect c = QRect(tileRect.topLeft().x() + tmpSize.width(), tileRect.topLeft().y(), tmpSize.width(), tmpSize.height());
//        if (c.intersects(pBox)) {
//            tileRect = c;
//            tileRef.replace(lvl, 1, 'c');
//            getFeatures(tileRef, theFeats);
//            continue;
//        }
//        QRect d = QRect(tileRect.topLeft().x(), tileRect.topLeft().y(), tmpSize.width(), tmpSize.height());
//        if (d.intersects(pBox)) {
//            tileRect = d;
//            tileRef.replace(lvl, 1, 'd');
//            getFeatures(tileRef, theFeats);
//            continue;
//        }
//        QRect a = QRect(tileRect.topLeft().x() + tmpSize.width(), tileRect.topLeft().y() + tmpSize.height(), tmpSize.width(), tmpSize.height());
//        if (a.intersects(pBox)) {
//            tileRect = a;
//            tileRef.replace(lvl, 1, 'a');
//            getFeatures(tileRef, theFeats);
//            continue;
//        }
//        QRect b = QRect(tileRect.topLeft().x(), tileRect.topLeft().y() + tmpSize.height(), tmpSize.width(), tmpSize.height());
//        if (b.intersects(pBox)) {
//            tileRect = b;
//            tileRef.replace(lvl, 1, 'b');
//            getFeatures(tileRef, theFeats);
//            continue;
//        }
//    }
//    qDebug() << "lvl: " << lvl << "; tile: " << tileRef << "; pbox: " << pBox;
//}