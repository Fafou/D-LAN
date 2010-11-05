#include <SearchModel.h>
using namespace GUI;

#include <Common/Settings.h>
#include <Common/Global.h>

const int SearchModel::NB_SIGNAL_PROGRESS(50);

/**
  * @class SearchModel
  * The result of a search. A search can only be performed once and for a limited period of time, see the setting "search_time".
  * The directories from the result can be browsed, thus this model inherits from the 'BrowseModel'.
  */

SearchModel::SearchModel(CoreConnection& coreConnection, PeerListModel& peerListModel)
   : BrowseModel(coreConnection, Common::Hash()), peerListModel(peerListModel), currentProgress(0)
{
   delete this->root;
   this->root = new SearchNode();

   this->timerProgress.setInterval(SETTINGS.get<quint32>("search_time") / NB_SIGNAL_PROGRESS);
   connect(&this->timerProgress, SIGNAL(timeout()), this, SLOT(sendNextProgress()));

   this->timerTimeout.setInterval(SETTINGS.get<quint32>("search_time"));
   this->timerTimeout.setSingleShot(true);
   connect(&this->timerTimeout, SIGNAL(timeout()), this, SLOT(stopSearching()));
}

SearchModel::~SearchModel()
{
}

Common::Hash SearchModel::getPeerID(const QModelIndex& index)
{
   SearchNode* node = static_cast<SearchNode*>(index.internalPointer());
   return node->getPeerID();
}

void SearchModel::search(const QString& terms)
{
   // Unable to perform more than one search.
   if (!this->searchResult.isNull())
      return;

   this->searchResult = this->coreConnection.search(terms);
   connect(this->searchResult.data(), SIGNAL(result(const Protos::Common::FindResult&)), this, SLOT(result(const Protos::Common::FindResult&)));
   this->searchResult->start();

   this->timerProgress.start();
   this->timerTimeout.start();
}

int SearchModel::columnCount(const QModelIndex& parent) const
{
   return 4;
}

void SearchModel::loadChildren(const QPersistentModelIndex &index)
{
   this->peerID = static_cast<const SearchNode*>(index.internalPointer())->getPeerID();
   BrowseModel::loadChildren(index);
}

/**
  * This method is called several times, one by received entries. The entries are inserted into the model.
  * The given entries are sorted by their level, we will keep the sort when inserting the entry but with some modifications :
  * - The directories are put first.
  * - All entries with the same level are sorted first by their path (prefixed with the shared directory name) and then by their name.
  * - All file entries with the same chunks (identical data) are grouped. They can be owned by different peer.
  */
void SearchModel::result(const Protos::Common::FindResult& findResult)
{
   SearchNode* root = this->getRoot();

   int currentDirNode = -1; // Will point to the first directory of a group of same level directory.
   if (root->getNbChildren() != 0 && root->getChild(0)->getEntry().type() == Protos::Common::Entry_Type_DIR)
      currentDirNode = 0;

   int currentFileNode = -1; // Same as the directories.
   if (root->getNbChildren() != 0 && root->getChild(0)->getEntry().type() == Protos::Common::Entry_Type_FILE)
      currentFileNode = 0;

   for (int i = 0; i < findResult.entry_size(); i++)
   {
      const Protos::Common::FindResult_EntryLevel entry = findResult.entry(i);

      if (entry.entry().type() == Protos::Common::Entry_Type_DIR)
      {
         currentDirNode = this->insertNode(entry, findResult.peer_id().hash().data(), currentDirNode);
         currentFileNode += 1;
      }
      else // It's a file.
      {
         // Search if a similar entry already exists. If so then insert the new node as child.
         if (entry.entry().chunk_size() > 0)
         {
            Common::Hash firstChunk = entry.entry().chunk(0).hash().data();
            SearchNode* similarNode = 0;
            if ((similarNode = this->indexedFile.value(firstChunk)) && similarNode->isSameAs(entry.entry()))
            {
               if (similarNode->getNbChildren() == 0)
               {
                  this->beginInsertRows(this->createIndex(0, 0, similarNode), 0, 0);
                  similarNode->insertChild(similarNode);
                  this->endInsertRows();
               }

               // Search the better name (node with the lowest level) to display it on the top.
               for (int i = 0; i <= similarNode->getNbChildren(); i++)
               {
                  if (i == similarNode->getNbChildren() || dynamic_cast<SearchNode*>(similarNode->getChild(i))->getLevel() > entry.level())
                  {
                     this->beginInsertRows(this->createIndex(0, 0, similarNode), i, i);
                     Common::Hash peerID = findResult.peer_id().hash().data();
                     SearchNode* newNode = similarNode->insertChild(i, entry, peerID, this->peerListModel.getNick(peerID));
                     this->endInsertRows();

                     if (entry.level() < similarNode->getLevel())
                     {
                        const int row = similarNode->getRow();
                        similarNode->copyFrom(newNode);
                        emit dataChanged(this->createIndex(row, 0, similarNode), this->createIndex(row, 3, similarNode));
                     }

                     break;
                  }
               }

               continue;
            }
         }

         currentFileNode = this->insertNode(entry, findResult.peer_id().hash().data(), currentFileNode);
      }
   }
}

void SearchModel::sendNextProgress()
{
   this->currentProgress += 100 / NB_SIGNAL_PROGRESS;
   emit progress(this->currentProgress > 100 ? 100 : this->currentProgress);
}

void SearchModel::stopSearching()
{
   this->searchResult.clear();
   this->timerProgress.stop();
   emit progress(100);
}

SearchModel::SearchNode* SearchModel::getRoot()
{
   return dynamic_cast<SearchNode*>(this->root);
}

/**
  * Create a new node, it can be a directory or a file. It will be inserted in the structure depending its level and its path+name.
  */
int SearchModel::insertNode(const Protos::Common::FindResult_EntryLevel& entry, const Common::Hash& peerID, int currentIndex)
{
   SearchNode* root = this->getRoot();
   SearchNode* newNode = 0;

   // We skip directories to keep them first when inserting a file.
   if (entry.entry().type() == Protos::Common::Entry_Type_FILE && root->getNbChildren() > 0)
   {
      if (currentIndex == -1)
         currentIndex = 0;
      while (currentIndex < root->getNbChildren() && root->getChild(currentIndex)->getEntry().type() == Protos::Common::Entry_Type_DIR)
         currentIndex += 1;
   }

   if (currentIndex == -1)
   {
      currentIndex = 0;
      this->beginInsertRows(QModelIndex(), 0, 0);
      newNode = this->getRoot()->insertChild(currentIndex, entry, peerID, this->peerListModel.getNick(peerID));
      this->endInsertRows();
   }
   else
   {
      // Search the first entry with the same level or, at least, a greater level.
      if (currentIndex < root->getNbChildren())
         while (dynamic_cast<SearchNode*>(root->getChild(currentIndex))->getLevel() < entry.level())
         {
            currentIndex += 1;
            if (currentIndex == root->getNbChildren() || root->getChild(currentIndex)->getEntry().type() != entry.entry().type())
               break;
         }

      // Search a place to insert the new directory node among nodes of the same level, the alphabetic order must be kept.
      int nodeToInsertBefore = currentIndex;
      while (nodeToInsertBefore < root->getNbChildren())
      {
         QString entryPath = SearchNode::entryPath(entry.entry()).toLower();
         QString nodePath = SearchNode::entryPath(root->getChild(nodeToInsertBefore)->getEntry()).toLower();

         if (root->getChild(nodeToInsertBefore)->getEntry().type() != entry.entry().type() || entryPath <= nodePath || (entryPath == nodePath && Common::ProtoHelper::getStr(entry.entry(), &Protos::Common::Entry::name).toLower() <= Common::ProtoHelper::getStr(root->getChild(nodeToInsertBefore)->getEntry(), &Protos::Common::Entry::name).toLower()))
            break;

         nodeToInsertBefore += 1;
         if (nodeToInsertBefore == root->getNbChildren() || dynamic_cast<SearchNode*>(root->getChild(nodeToInsertBefore))->getLevel() > entry.level())
            break;
      }

      this->beginInsertRows(QModelIndex(), nodeToInsertBefore, nodeToInsertBefore);
      newNode = root->insertChild(nodeToInsertBefore, entry, peerID, this->peerListModel.getNick(peerID));
      this->endInsertRows();
   }

   if (newNode && newNode->getEntry().type() == Protos::Common::Entry_Type_FILE && newNode->getEntry().chunk_size() > 0)
      this->indexedFile.insert(newNode->getEntry().chunk(0).hash().data(), newNode);

   return currentIndex;
}

/////

/**
  * Will append the shared directory name to the relative path.
  */
QString SearchModel::SearchNode::entryPath(const Protos::Common::Entry& entry)
{
   QString path = Common::ProtoHelper::getStr(entry, &Protos::Common::Entry::path);
   QString sharedName = Common::ProtoHelper::getStr(entry.shared_dir(), &Protos::Common::SharedDir::shared_name);

   QString completePath;
   if (path.isEmpty())
      completePath.append("/");
   else
      completePath.append("/").append(sharedName).append(path);

   return completePath;
}

SearchModel::SearchNode::SearchNode()
   : level(0)
{
}

SearchModel::SearchNode::SearchNode(const Protos::Common::Entry& entry, int level, const Common::Hash& peerID, const QString& peerNick, Node* parent)
   : Node(entry, parent), level(level), peerID(peerID), peerNick(peerNick)
{
}

SearchModel::SearchNode::SearchNode(const Protos::Common::Entry& entry, const Common::Hash& peerID,  Node* parent)
   : Node(entry, parent), level(0), peerID(peerID)
{
}

SearchModel::SearchNode* SearchModel::SearchNode::insertChild(const Protos::Common::FindResult_EntryLevel& entry, const Common::Hash& peerID, const QString& peerNick)
{
   SearchNode* searchNode = new SearchNode(entry.entry(), entry.level(), peerID, peerNick, this);
   this->children << searchNode;
   return searchNode;
}

SearchModel::SearchNode* SearchModel::SearchNode::insertChild(int index, const Protos::Common::FindResult_EntryLevel& entry, const Common::Hash& peerID, const QString& peerNick)
{
   SearchNode* searchNode = new SearchNode(entry.entry(), entry.level(), peerID, peerNick, this);
   this->children.insert(index, searchNode);
   return searchNode;
}

SearchModel::SearchNode* SearchModel::SearchNode::insertChild(SearchModel::SearchNode* node)
{
   SearchNode* searchNode = new SearchNode(node->getEntry(), node->getLevel(), node->getPeerID(), node->peerNick, this);
   this->children << searchNode;
   return searchNode;
}

int SearchModel::SearchNode::getLevel() const
{
   return this->level;
}

Common::Hash SearchModel::SearchNode::getPeerID() const
{
   return this->peerID;
}

QVariant SearchModel::SearchNode::getData(int column) const
{
   switch(column)
   {
   case 0: return Common::ProtoHelper::getStr(this->entry, &Protos::Common::Entry::name);
   case 1:
      if (this->parent->getParent() != 0 && this->parent->getEntry().type() == Protos::Common::Entry_Type_DIR)
         return QVariant();

      if (this->entry.type() == Protos::Common::Entry_Type_FILE && this->getNbChildren() > 0)
         return QVariant();

      return entryPath(this->entry);

   case 2:
      if (this->entry.type() == Protos::Common::Entry_Type_FILE && this->getNbChildren() > 0)
      {
         for (int i = 0; i < this->getNbChildren(); i++)
            if (this->peerNick != dynamic_cast<const SearchNode*>(this->getChild(i))->peerNick)
               return QVariant();
      }
      return this->peerNick;

   case 3: return Common::Global::formatByteSize(this->entry.size());
   default: return QVariant();
   }
}

SearchModel::Node* SearchModel::SearchNode::newNode(const Protos::Common::Entry& entry)
{
   this->children << new SearchNode(entry, this->peerID, this);
   return this->children.last();
}

void SearchModel::SearchNode::copyFrom(const SearchModel::SearchNode* otherNode)
{
   this->entry.CopyFrom(otherNode->getEntry());
   this->level = otherNode->getLevel();
   this->peerID = otherNode->getPeerID();
   this->peerNick = otherNode->peerNick;
}

bool SearchModel::SearchNode::isSameAs(const Protos::Common::Entry& otherEntry) const
{
   if (otherEntry.chunk_size() != this->entry.chunk_size())
      return false;

   for (int i = 0; i < otherEntry.chunk_size(); i++)
      if (Common::Hash(otherEntry.chunk(i).hash().data()) != Common::Hash(this->entry.chunk(i).hash().data()))
         return false;

   return true;
}


