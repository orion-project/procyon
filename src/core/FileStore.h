#ifndef FILE_STORE_H
#define FILE_STORE_H

#include <QFileInfo>

class FileStore
{
public:
    QFileInfo attachedFile(const QString& fileName);
    bool isSupportedImg(const QString& path);
};

namespace Store
{
FileStore* files();
}

#endif // FILE_STORE_H