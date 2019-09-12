#ifndef CATALOG_STORE_H
#define CATALOG_STORE_H

#include "MemoManager.h"
#include "FolderManager.h"
#include "SettingsManager.h"

namespace CatalogStore {

MemoManager* memoManager();
FolderManager* folderManager();
SettingsManager* settingsManager();

QString openDatabase(const QString fileName);

} // namespace CatalogStore

#endif // CATALOG_STORE_H
