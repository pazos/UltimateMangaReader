#include "mangainfo.h"
#include "configs.h"


MangaInfo::MangaInfo(QObject *parent, AbstractMangaSource *mangasource):
    QObject(parent),
    currentindex(0, 0),
    numchapters(0),
    mangasource(mangasource),
    preloadqueue(parent, mangasource),
    updating(false)
{
    QObject::connect(&preloadqueue, SIGNAL(completedDownload(QString)), this, SLOT(completedImagePreload(QString)));
}


MangaInfo::~MangaInfo()
{

}

QString MangaInfo::getImageLink(MangaIndex index)
{
    if (index.chapter < 0 || index.chapter >= numchapters)
        return "";

    if (!chapters[index.chapter].pagesloaded)
        chapters[index.chapter].loadPages();

//    qDebug() << "chapter" << index.chapter << "page" << index.page << "of" << chapters[index.chapter].pagelinks->count();
//    qDebug() << chapters.count();

    if (chapters[index.chapter].imagelinks[index.page] == "")
        chapters[index.chapter].imagelinks[index.page] =
            mangasource->getImageLink(chapters[index.chapter].pagelinks.at(index.page));

    return chapters[index.chapter].imagelinks[index.page];
}



QString MangaInfo::getCurrentImage()
{
    QString imagelink = getImageLink(currentindex);


    return mangasource->downloadAwaitImage(imagelink, title, currentindex.chapter, currentindex.page);
}

QString MangaInfo::goChapterPage(MangaIndex index)
{
    QString imagelink = getImageLink(index);
    if (imagelink == "")
        return "";

    currentindex = index;

    return mangasource->downloadAwaitImage(imagelink, title, currentindex.chapter, currentindex.page);
}


QString MangaInfo::goNextPage()
{
    MangaIndex nextpage = currentindex.nextPageIndex(&chapters);
    if (nextpage.illegal)
        return "";

    currentindex = nextpage;

    return getCurrentImage();
}

QString MangaInfo::goPrevPage()
{
    MangaIndex prevpage = currentindex.prevPageIndex(&chapters);
    if (prevpage.illegal)
        return "";

    currentindex = prevpage;

    return getCurrentImage();
}

QString MangaInfo::goLastChapter()
{
    currentindex = MangaIndex(numchapters - 1, 0);

    return getCurrentImage();
}

QString MangaInfo::goFirstChapter()
{
    currentindex = MangaIndex(0, 0);

    return getCurrentImage();
}

void MangaInfo::preloadImage(MangaIndex index)
{
//    return;
//    qDebug() << "preloading" << index.chapter << index.page;
    if (!index.checkLegal(&chapters))
    {
//        qDebug() << "illegal";
        return;
    }

    preloadqueue.addJob(DownloadImageInfo(getImageLink(index), title, index.chapter, index.page));
//    mangasource->downloadImage(getImageLink(index), title, index.chapter, index.page);
}

void MangaInfo::preloadPopular()
{
    if (numchapters == 0)
        return;
    preloadImage(currentindex);
    if (numchapters > 1 && currentindex.chapter != numchapters - 1)
        preloadImage(MangaIndex(numchapters - 1, 0));
}


void MangaInfo::preloadNeighbours(int distance)
{
    MangaIndex ind = currentindex;
    ind = ind.nextPageIndex(&chapters);
    if (!ind.illegal)
        preloadImage(ind);

    ind = currentindex.prevPageIndex(&chapters);
    if (!ind.illegal)
        preloadImage(ind);

    ind = currentindex.nextPageIndex(&chapters);
    for (int i = 1; i < distance; i++)
    {
        ind = ind.nextPageIndex(&chapters);
        if (ind.illegal)
            break;
        preloadImage(ind);
    }
}

void MangaInfo::cancelAllPreloads()
{
    preloadqueue.clearQuene();
}

void MangaInfo::completedImagePreload(const QString &path)
{
//    qDebug() << path;
    emit completedImagePreloadSignal(path);
}



MangaInfo *MangaInfo::deserialize(QObject *parent, AbstractMangaSource *mangasource, const QString &path)
{
    MangaInfo *mi = new MangaInfo(parent, mangasource);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return mi;

    QDataStream in(&file);
    in >> mi->hostname >> mi->title >> mi->link >> mi->author >> mi->artist >> mi->releaseyear >> mi->status >> mi->genres >> mi->summary >> mi->coverpath >>
       mi->numchapters;
    in >> mi->chapertitlesreversed >> mi->chapters;

    file.close();

    QMutableListIterator<MangaChapter> i(mi->chapters);
    while (i.hasNext())
    {
        MangaChapter &c = i.next();
        c.source = mangasource;
    }


    QString progresspath(path);
    progresspath.replace("mangainfo", "progress");

    QFile file2(progresspath);
    if (!file2.open(QIODevice::ReadOnly))
        return mi;

    QDataStream in2(&file2);
    in2 >> mi->currentindex;

    file2.close();

    return mi;
}

void MangaInfo::serialize()
{
    QFile file(mangainfodir(hostname, title) + "mangainfo.dat");
    if (!file.open(QIODevice::WriteOnly))
        return;

    QDataStream out(&file);
    out << hostname << title << link << author << artist << releaseyear << status << genres << summary << coverpath << (qint32) numchapters;
    out << chapertitlesreversed << chapters;

    file.close();

    if (!QFileInfo(mangainfodir(hostname, title) + "progress.dat").exists())
        serializeProgress();
}


void MangaInfo::serializeProgress()
{
    QFile file(mangainfodir(hostname, title) + "progress.dat");
    if (!file.open(QIODevice::WriteOnly))
        return;

//    qDebug() << file.fileName();

    QDataStream out(&file);
    out << currentindex << (qint32) numchapters;

    file.close();
}


void MangaInfo::sendUpdated(bool changed)
{
    updating = false;

    if (changed)
        emit updated();
    else
        emit updatedNoChanges();
}











