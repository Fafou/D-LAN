/**
  * D-LAN - A decentralized LAN file sharing software.
  * Copyright (C) 2010-2012 Greg Burri <greg.burri@gmail.com>
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */
  
#include <Tests.h>
using namespace FM;

#include <string>
using namespace std;

#include <QtDebug>
#include <QRegExp>
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QStringList>
#include <QDirIterator>

#include <Protos/core_settings.pb.h>

#include <Common/LogManager/Builder.h>
#include <Common/PersistentData.h>
#include <Common/Constants.h>
#include <Common/Global.h>
#include <Common/ProtoHelper.h>
#include <Common/Settings.h>
#include <Common/SharedDir.h>

#include <IChunk.h>
#include <IGetHashesResult.h>
#include <Exceptions.h>
#include <priv/Constants.h>
#include <priv/WordIndex/WordIndex.h>

#include <HashesReceiver.h>

Tests::Tests()
{
}

void Tests::initTestCase()
{
   LM::Builder::initMsgHandler();

   qDebug() << "===== initTestCase() =====";

   try
   {
      QString tempFolder = Common::Global::setCurrentDirToTemp("FileManagerTests");
      qDebug() << "Application folder path (where the persistent data is put) : " <<  Common::Global::getDataFolder(Common::Global::DataFolderType::LOCAL, false);
      qDebug() << "The file created during this test are put in : " << tempFolder;
   }
   catch(Common::Global::UnableToSetTempDirException& e)
   {
      QFAIL(e.errorMessage.toAscii().constData());
   }

   Common::PersistentData::rmValue(Common::Constants::FILE_CACHE, Common::Global::DataFolderType::LOCAL); // Reset the stored cache.

   SETTINGS.setFilename("core_settings_file_manager_tests.txt");
   SETTINGS.setSettingsMessage(new Protos::Core::Settings());
}

void Tests::testWordIndex()
{
   qDebug() << "===== testWordIndex() =====";

   WordIndex<int> index;
   int arbre = 1;
   int arbalete = 2;
   int ar = 2;
   int arbuste = 3;

   index.addItem("arbre", arbre);
   index.addItem("arbalete", arbalete);

   QList<int> result0 = WordIndex<int>::resultToList(index.search("arime"));
   QVERIFY(result0.size() == 0);

   index.addItem("ar", ar);
   index.addItem("arbuste", arbuste);

   qDebug() << index.toStringLog();

   QList<int> result1 = WordIndex<int>::resultToList(index.search("ar"));
   QVERIFY(result1.size() == 1);
   QVERIFY(result1.contains(ar));

   QList<int> result2 = WordIndex<int>::resultToList(index.search("arb"));
   QVERIFY(result2.size() == 3);
   QVERIFY(result2.contains(arbre));
   QVERIFY(result2.contains(arbalete));
   QVERIFY(result2.contains(arbuste));

   QList<int> result3 = WordIndex<int>::resultToList(index.search("arbr"));
   QVERIFY(result3.size() == 1);
   QVERIFY(result3.contains(arbre));

   QList<int> result4 = WordIndex<int>::resultToList(index.search("arbre"));
   QVERIFY(result4.size() == 1);
   QVERIFY(result4.contains(arbre));

   QList<int> result5 = WordIndex<int>::resultToList(index.search("arbres"));
   QVERIFY(result5.size() == 0);

   index.rmItem("arbuste", arbuste);

   QList<int> result6 = WordIndex<int>::resultToList(index.search("arb"));
   QVERIFY(result6.size() == 2);
   QVERIFY(result6.contains(arbre));
   QVERIFY(result6.contains(arbalete));

   QList<int> result7 = WordIndex<int>::resultToList(index.search("arbuste"));
   QVERIFY(result7.size() == 0);

   index.rmItem("arbalete", arbalete);

   QList<int> result8 = WordIndex<int>::resultToList(index.search("arb"));
   QVERIFY(result8.size() == 1);
   QVERIFY(result8.contains(arbre));

   QList<int> result9 = WordIndex<int>::resultToList(index.search("arbalete"));
   QVERIFY(result9.size() == 0);

   index.rmItem("arbre", arbre);

   QList<int> result10 = WordIndex<int>::resultToList(index.search("arb"));
   QVERIFY(result10.size() == 0);
}

void Tests::createFileManager()
{
   qDebug() << "===== createFileManager() =====";

   this->createInitialFiles();
   this->fileManager = Builder::newFileManager();
}

void Tests::addASharedDirectoryIncoming()
{
   qDebug() << "===== addASharedDirectoryIncoming() =====";

   this->sharedDirs << QDir::currentPath().append("/incoming/");
   this->fileManager->setSharedDirs(this->sharedDirs);
   QList<Common::SharedDir> paths = this->fileManager->getSharedDirs();
   QVERIFY(paths.size() == 1);
   QCOMPARE(paths.at(0).path, this->sharedDirs.at(0));
}

void Tests::addASharedDirectory()
{
   qDebug() << "===== addASharedDirectory() =====";

   this->sharedDirs << QDir::currentPath().append("/sharedDirs/share1/");
   this->fileManager->setSharedDirs(this->sharedDirs);
   QList<Common::SharedDir> paths = this->fileManager->getSharedDirs();
   QVERIFY(paths.size() == 2);
   QCOMPARE(paths.at(1).path, this->sharedDirs.at(1));
}

void Tests::addAnAlreadySharedDirectory()
{
   qDebug() << "===== addAnAlreadySharedDirectory() =====";

   this->fileManager->setSharedDirs(this->sharedDirs);
   QList<Common::SharedDir> paths = this->fileManager->getSharedDirs();
   QVERIFY(paths.size() == 2);
   QCOMPARE(paths.at(1).path, this->sharedDirs.at(1));
}

void Tests::swapTwoDirectories()
{
   this->sharedDirs.move(1, 0);
   this->fileManager->setSharedDirs(this->sharedDirs);
   QList<Common::SharedDir> paths = this->fileManager->getSharedDirs();
   QCOMPARE(paths.at(0).path, this->sharedDirs.at(0));
   QCOMPARE(paths.at(1).path, this->sharedDirs.at(1));

   this->sharedDirs.move(1, 0);
   this->fileManager->setSharedDirs(this->sharedDirs);
   QList<Common::SharedDir> paths2 = this->fileManager->getSharedDirs();
   QCOMPARE(paths2.at(0).path, this->sharedDirs.at(0));
   QCOMPARE(paths2.at(1).path, this->sharedDirs.at(1));
}

void Tests::addInexistingSharedDirectory()
{
   qDebug() << "===== addInexistingSharedDirectory() =====";

   this->sharedDirs << QDir::currentPath().append("/this_is_spartaaaaaa/"); // This directory doesn't exit.
   try
   {
      this->fileManager->setSharedDirs(this->sharedDirs);
      QFAIL("An exception must be thrown");
   }
   catch (DirsNotFoundException& e)
   {
      QVERIFY(e.paths.size() == 1);
      QCOMPARE(e.paths.at(0), this->sharedDirs.last());
      qDebug() << "This directory hasn't been found: " << e.paths.at(0) << " (Exception thrown)";
   }
   this->sharedDirs.removeLast();
}

void Tests::addSubSharedDirectories()
{
   qDebug() << "===== addSubSharedDirectories() =====";

   this->sharedDirs << QDir::currentPath().append("/sharedDirs/share1/subdir/");
   this->sharedDirs << QDir::currentPath().append("/sharedDirs/share1/another subdir/");

   this->fileManager->setSharedDirs(this->sharedDirs);

   QCOMPARE(this->fileManager->getSharedDirs().size(), 2);

   this->sharedDirs.removeLast();
   this->sharedDirs.removeLast();
}

/**
  * The subs directories of each subdirectory must be merged into the super directory.
  */
void Tests::addSuperSharedDirectories()
{
   qDebug() << "===== addSuperSharedDirectories() =====";

   this->sharedDirs << QDir::currentPath().append("/sharedDirs/");
   this->fileManager->setSharedDirs(this->sharedDirs);

   QTest::qSleep(100);
}

void Tests::createAFile()
{
   qDebug() << "===== createAFile() =====";

   Common::Global::createFile("sharedDirs/x.txt");
   QTest::qSleep(100);
}

void Tests::moveAFile()
{
   qDebug() << "===== moveAFile() =====";

   QDir::current().rename("sharedDirs/x.txt", "sharedDirs/share1/x.txt");
   QTest::qSleep(100);
}

void Tests::renameAFile()
{
   qDebug() << "===== renameAFile() =====";

   QDir::current().rename("sharedDirs/share1/x.txt", "sharedDirs/share1/y.txt");
   QTest::qSleep(100);
}

void Tests::modifyAFile()
{
   qDebug() << "===== modifyAFile() =====";

   {
      QFile file("sharedDirs/share1/y.txt");
      file.open(QIODevice::Append);
      QTextStream stream(&file);
      stream << "12345";
   }
   QTest::qSleep(100);
}

void Tests::removeAFile()
{
   qDebug() << "===== removeAFile() =====";

   QFile("sharedDirs/share1/y.txt").remove();
   QTest::qSleep(100);
}

void Tests::createASubFile()
{
   qDebug() << "===== createASubFile() =====";

   Common::Global::createFile("sharedDirs/share1/v.txt");
   QTest::qSleep(100);

   // TODO: check if cache own v.txt (only for watchable shared dir).
}

void Tests::createABigFile()
{
   qDebug() << "===== createABigFile() =====";

   QFile file("sharedDirs/big.bin");
   file.open(QIODevice::WriteOnly);
   file.resize(128 * 1024 * 1024); // 128Mo
   QTest::qSleep(1000);
}

void Tests::modifyABigFile()
{
   qDebug() << "===== modifyABigFile() =====";

   {
      const QString filePath("sharedDirs/big.bin");
      QFile file(filePath);
      if (!file.open(QIODevice::ReadWrite))
      {
         qDebug() << "Can't create the file " << filePath;
         return;
      }
      QDataStream stream(&file);
      stream.skipRawData(32 * 1024 * 1024 - 3);
      QByteArray data("XXXXXX");
      stream.writeRawData(data.constData(), data.size());
   }

   QTest::qSleep(1000);
}

void Tests::removeABigFile()
{
   qDebug() << "===== removeABigFile() =====";

   while(!QFile("sharedDirs/big.bin").remove())
      QTest::qWait(100); // We use qWait because some events must be processed (timers).
   QTest::qWait(100);
}

void Tests::createADirectory()
{
   qDebug() << "===== createADirectory() =====";

   Common::Global::createFile("sharedDirs/a/");
   QTest::qSleep(100);
}

void Tests::renameADirectory()
{
   qDebug() << "===== renameADirectory() =====";

   QDir("sharedDirs").rename("a", "b");
   QTest::qSleep(100);
}

void Tests::moveAnEmptyDirectory()
{
   qDebug() << "===== moveAnEmptyDirectory() =====";

   QDir("sharedDirs").rename("b", "share1/b");
   QTest::qSleep(100);
}

void Tests::moveADirectoryContainingFiles()
{
   qDebug() << "===== moveADirectoryContainingFiles() =====";

   QDir("sharedDirs").rename("share2", "share1/share2");
   QTest::qSleep(100);
}

void Tests::removeADirectory()
{
   qDebug() << "===== removeADirectory() =====";

   Common::Global::recursiveDeleteDirectory("sharedDirs/share1/share2");
   QTest::qSleep(100);
}

void Tests::createAnEmptyFile()
{
   qDebug() << "===== createAnEmptyFile() =====";

   Protos::Common::Entry remoteEntry;
   remoteEntry.set_path("/remoteShare1/");
   remoteEntry.set_name("remoteFile.txt");
   remoteEntry.set_size(1 * 1024 * 1024); // 1Mo.

   try
   {
      QList<QSharedPointer<IChunk>> chunks = this->fileManager->newFile(remoteEntry);
      for (int i = 0; i < chunks.size(); i++)
         QVERIFY(chunks[i]->getHash().isNull());
   }
   catch(NoWriteableDirectoryException&)
   {
      QFAIL("NoWriteableDirectoryException");
   }
   catch(InsufficientStorageSpaceException&)
   {
      QFAIL("InsufficientStorageSpaceException");
   }
   catch(UnableToCreateNewFileException&)
   {
      QFAIL("UnableToCreateNewFileException");
   }
}

void Tests::getAnExistingChunk()
{
   qDebug() << "===== getAExistingChunk() =====";

   QSharedPointer<IChunk> chunk = this->fileManager->getChunk(Common::Hash::fromStr("97d464813598e2e4299b5fe7db29aefffdf2641d"));
   if (chunk.isNull())
      QFAIL("Chunk not found");
   else
      qDebug() << "Chunk found: " << chunk->getHash().toStr();
}

void Tests::getAnUnexistingChunk()
{
   qDebug() << "===== getAnUnexistingChunk() =====";

   QSharedPointer<IChunk> chunk = this->fileManager->getChunk(Common::Hash::fromStr("928e1bd85c0957c4af0cf69cf76f6ed6898cfd2d"));
   if (chunk.isNull())
      qDebug() << "Chunk not found: OK" << Common::Hash::rand().toStr();
   else
      QFAIL("No chunk must be found");
}

void Tests::getHashesFromAFileEntry1()
{
   qDebug() << "===== getHashesFromAFileEntry1() =====";

   // Find the id of the first shared directory.
   Protos::Common::Entries sharedDirs = this->fileManager->getEntries();
   const string sharedDirId = sharedDirs.entry(1).shared_dir().id().hash();

   Protos::Common::Entry entry;
   entry.set_path("/share1/");
   entry.set_name("r.txt");
   entry.mutable_shared_dir()->mutable_id()->set_hash(sharedDirId);
   entry.add_chunk();

   QSharedPointer<IGetHashesResult> result = this->fileManager->getHashes(entry);

   HashesReceiver hashesReceiver;
   connect(result.data(), SIGNAL(nextHash(Protos::Core::HashResult)), &hashesReceiver, SLOT(nextHash(Protos::Core::HashResult)));

   Protos::Core::GetHashesResult res = result->start();

   QCOMPARE(res.status(), Protos::Core::GetHashesResult::OK);
   QVERIFY(hashesReceiver.waitToReceive(QList<Common::Hash>() << Common::Hash::fromStr("97d464813598e2e4299b5fe7db29aefffdf2641d"), 500));
}

void Tests::getHashesFromAFileEntry2()
{
   qDebug() << "===== getHashesFromAFileEntry2() =====";

   {
      QFile file1("sharedDirs/big2.bin");
      file1.open(QIODevice::WriteOnly);

      QFile file2("sharedDirs/big3.bin");
      file2.open(QIODevice::WriteOnly);

      file1.resize(128 * 1024 * 1024); // 128 MiB.
      file2.resize(128 * 1024 * 1024); // 128 MiB.
   }

   QTest::qWait(2000); // Begin the computing of the big2.bin hashes.

   Protos::Common::Entries sharedDirs = this->fileManager->getEntries();
   const string sharedDirId = sharedDirs.entry(1).shared_dir().id().hash();

   Protos::Common::Entry entry;
   entry.set_path("/");
   entry.set_name("big3.bin");
   entry.mutable_shared_dir()->mutable_id()->set_hash(sharedDirId);
   for (int i = 0; i < 2; i++) // 128 MiB -> 2 chunks.
      entry.add_chunk();
   QSharedPointer<IGetHashesResult> result = this->fileManager->getHashes(entry);

   HashesReceiver hashesReceiver;
   connect(result.data(), SIGNAL(nextHash(Protos::Core::HashResult)), &hashesReceiver, SLOT(nextHash(Protos::Core::HashResult)));
   Protos::Core::GetHashesResult res = result->start(); // Should stop the computing of 'big2.bin' and switch to 'big3.bin'.
   qDebug() << res.status();
   QCOMPARE(res.status(), Protos::Core::GetHashesResult::OK);

   QTest::qWait(4000);
}

void Tests::browseSomedirectories()
{
   qDebug() << "===== browseSomedirectories() =====";

   // TODO: active the regexp comparison.

   // Get the shared directories.
   Protos::Common::Entries entries1 = this->fileManager->getEntries();
   /*QString entries1Str = Common::ProtoHelper::getDebugStr(entries1);
   Tests::compareStrRegexp(
      "dir\\s*\\{\n\\s*shared_dir\\s*\\{\n\\s*id\\s*\\{\n\\s*hash:\\s*\".+\"\n\\s*\\}\n\\s*\\}\n\\s*path:\\s*\"\"\n\\s*name:\\s*\"sharedDirs\"\n\\s*size:\\s*\\d+\n\\}\ndir\\s*\\{\n\\s*shared_dir\\s*\\{\n\\s*id\\s*\\{\n\\s*hash:\\s*\".+\"\n\\s*\\}\n\\s*\\}\n\\s*path:\\s*\"\"\n\\s*name:\\s*\"incoming\"\n\\s*size:\\s*\\d+\n\\}.*",
      entries1Str
   );
   QVERIFY(entries1Str != "");
   qDebug() << entries1Str;*/
   QVERIFY(entries1.entry_size() != 0);
   qDebug() << Common::ProtoHelper::getDebugStr(entries1);

   // Ask for the files and directories of the first shared directory
   Protos::Common::Entries entries2 = this->fileManager->getEntries(entries1.entry(0));
   qDebug() << Common::ProtoHelper::getDebugStr(entries2);

   // Ask for the files and directores of the first directory of the first shared directory
   Protos::Common::Entries entries3 = this->fileManager->getEntries(entries2.entry(0));
   qDebug() << Common::ProtoHelper::getDebugStr(entries3);
}

void Tests::findExistingFilesWithOneWord()
{
   qDebug() << "===== findExistingFilesWithOneWord() =====";

   QString terms("aaaa");

   FindResult expectedResult;
   expectedResult[0] << "aaaa cccc.txt" << "aaaa bbbb.txt" << "aaaa bbbb cccc.txt" << "aaaa dddddd.txt";
   expectedResult[1] << "aaaaaa dddddd.txt" << "aaaaaa bbbb.txt" << "aaaaaa bbbbbb.txt";

   QList<Protos::Common::FindResult> results = this->fileManager->find(terms, 10000, 65536);
   QVERIFY(!results.isEmpty());
   this->printSearch(terms, results.first());
   this->compareExpectedResult(results.first(), expectedResult);
}

void Tests::findUnexistingFilesWithOneWord()
{
   qDebug() << "===== findUnexistingFilesWithOneWord() =====";

   QString terms("mmmm");
   QList<Protos::Common::FindResult> results = this->fileManager->find(terms, 10000, 65536);
   QVERIFY(results.isEmpty());
}

void Tests::findFilesWithSomeWords1()
{
   qDebug() << "===== findFilesWithSomeWords1() =====";

   QString terms("aaaa bbbb cccc");

   FindResult expectedResult;
   expectedResult[0] << "aaaa bbbb cccc.txt";
   expectedResult[4] << "aaaa bbbb.txt";
   expectedResult[5] << "aaaa cccc.txt";
   expectedResult[6] << "cccc bbbb.nfo" << "bbbb cccc.nfo";
   expectedResult[7] << "aaaaaa bbbb.txt";
   expectedResult[9] << "cccc bbbbbb.txt";
   expectedResult[10] << "aaaaaa bbbbbb.txt";
   expectedResult[13] << "aaaa dddddd.txt";
   expectedResult[14] << "bbbb.txt" <<  "bbbb dddd.nfo";
   expectedResult[16] << "aaaaaa dddddd.txt";

   QList<Protos::Common::FindResult> results = this->fileManager->find(terms, 10000, 65536);
   QVERIFY(!results.isEmpty());
   this->printSearch(terms, results.first());
   this->compareExpectedResult(results.first(), expectedResult);
}

void Tests::findFilesWithSomeWords2()
{
   qDebug() << "===== findFilesWithSomeWords2() =====";

   QString terms("aaaa bbbb cccc dddd");

   FindResult expectedResult;
   expectedResult[5] << "aaaa bbbb cccc.txt";
   expectedResult[21] << "aaaa bbbb.txt";
   expectedResult[22] << "aaaa cccc.txt";
   expectedResult[24] << "bbbb cccc.nfo" << "cccc bbbb.nfo";
   expectedResult[25] << "bbbb dddd.nfo";
   expectedResult[27] << "aaaaaa bbbb.txt";
   expectedResult[29] << "aaaa dddddd.txt";
   expectedResult[30] << "cccc bbbbbb.txt";
   expectedResult[33] << "aaaaaa bbbbbb.txt";
   expectedResult[35] << "aaaaaa dddddd.txt";
   expectedResult[40] << "bbbb.txt";
   expectedResult[42] << "dddd.txt";

   QList<Protos::Common::FindResult> results = this->fileManager->find(terms, 10000, 65536);
   QVERIFY(!results.isEmpty());
   this->printSearch(terms, results.first());
   this->compareExpectedResult(results.first(), expectedResult);
}

void Tests::findFilesWithResultFragmentation()
{
   qDebug() << "===== findFilesWithResultFragmentation() =====";

   const int FRAGMENT_MAX_SIZE = 200;

   QString terms("bbb");
   QList<Protos::Common::FindResult> results = this->fileManager->find(terms, 10000, FRAGMENT_MAX_SIZE);
   qDebug() << "Nb fragment: " << results.size();
   for (int i = 0; i < results.size(); i++)
   {
      qDebug() << "Fragment number " << i << ", size = " << results[i].ByteSize();
      QVERIFY(results[i].ByteSize() <= FRAGMENT_MAX_SIZE);
      this->printSearch(terms, results[i]);
   }
}

void Tests::findFilesWithSomeWordsAndExtensions()
{
   qDebug() << "===== findFilesWithSomeWordsAndExtensions() =====";

   QString terms("aaaa bbbb cccc");

   FindResult expectedResult;
   expectedResult[0] << "aaaa bbbb cccc.txt";
   expectedResult[4] << "aaaa bbbb.txt";
   expectedResult[5] << "aaaa cccc.txt";
   expectedResult[7] << "aaaaaa bbbb.txt";
   expectedResult[9] << "cccc bbbbbb.txt";
   expectedResult[10] << "aaaaaa bbbbbb.txt";
   expectedResult[13] << "aaaa dddddd.txt";
   expectedResult[14] << "bbbb.txt";
   expectedResult[16] << "aaaaaa dddddd.txt";

   QList<Protos::Common::FindResult> results = this->fileManager->find(terms, QList<QString> { "txt" }, 0, std::numeric_limits<qint64>::max(), Protos::Common::FindPattern::FILE_DIR, 10000, 65536);
   QVERIFY(!results.isEmpty());
   this->printSearch(terms, results.first());
   this->compareExpectedResult(results.first(), expectedResult);
}

void Tests::findFilesWithSomeWordsAndExtensionsAndSizeRange()
{
   qDebug() << "===== findFilesWithSomeWordsAndSizeRange() =====";

   QString terms("aaaa bbbb cccc");

   FindResult expectedResult;
   //expectedResult[0] << "aaaa bbbb cccc.txt"; // 18
   //expectedResult[4] << "aaaa bbbb.txt"; // 13
   //expectedResult[5] << "aaaa cccc.txt"; // 13
   expectedResult[7] << "aaaaaa bbbb.txt";  // 15
   expectedResult[9] << "cccc bbbbbb.txt"; // 15
   //expectedResult[10] << "aaaaaa bbbbbb.txt"; // 17
   expectedResult[13] << "aaaa dddddd.txt"; // 16
   //expectedResult[14] << "bbbb.txt"; // 8
   //expectedResult[16] << "aaaaaa dddddd.txt"; // 17

   QList<Protos::Common::FindResult> results = this->fileManager->find(terms, QList<QString> { "txt" }, 15, 16, Protos::Common::FindPattern::FILE_DIR, 10000, 65536);
   QVERIFY(!results.isEmpty());
   this->printSearch(terms, results.first());
   this->compareExpectedResult(results.first(), expectedResult);
}

void Tests::findFilesByExtensions()
{
   qDebug() << "===== findFilesByExtensions() =====";

   FindResult expectedResult;
   expectedResult[0] << "bbbb cccc.nfo" << "cccc bbbb.nfo" << "bbbb dddd.nfo";

   QList<Protos::Common::FindResult> results = this->fileManager->find("", QList<QString> { "nfo" }, 0, std::numeric_limits<qint64>::max(), Protos::Common::FindPattern::FILE_DIR, 10000, 65536);
   QVERIFY(!results.isEmpty());
   this->compareExpectedResult(results.first(), expectedResult);
}

void Tests::findFilesByExtensionsAndSizeRange()
{
   qDebug() << "===== findFilesByExtensionsAndSizeRange() =====";

   FindResult expectedResult;
   expectedResult[0] << "aaaa bbbb.txt" << "aaaa cccc.txt" << "bbbb.txt";

   QList<Protos::Common::FindResult> results = this->fileManager->find("", QList<QString> { "txt" }, 12, 13, Protos::Common::FindPattern::FILE_DIR, 10000, 65536);
   QVERIFY(!results.isEmpty());
   this->compareExpectedResult(results.first(), expectedResult);
}

void Tests::findFilesBySizeRange()
{
   qDebug() << "===== findFilesBySizeRange() =====";

   // TODO
   FindResult expectedResult;
   expectedResult[0] << "aaaaaa bbbb.txt" << "cccc bbbbbb.txt" << "aaaa dddddd.txt" << "bbbb cccc.nfo" << "cccc bbbb.nfo" << "bbbb dddd.nfo";

   QList<Protos::Common::FindResult> results = this->fileManager->find("", QList<QString>(), 15, 16, Protos::Common::FindPattern::FILE_DIR, 10000, 65536);
   QVERIFY(!results.isEmpty());
   this->compareExpectedResult(results.first(), expectedResult);
}

void Tests::haveChunks()
{
   qDebug() << "===== haveChunks() =====";

   QList<Common::Hash> hashes;
   hashes
      << Common::Hash::fromStr("f6126deaa5e1d9692d54e3bef0507721372ee7f8") // "/sharedDirs/share3/aaaa bbbb cccc.txt"
      << Common::Hash::fromStr("4c24e58c47746ea04296df9342185d9b3a447899") // "/sharedDirs/share1/v.txt"
      << Common::Hash::fromStr("954531aef8ac193ad62f4de783da9d7e6ebd59dd") // "/sharedDirs/share1/y.txt" (deleted)
      << Common::Hash::fromStr("8374d82e993012aa23b293f319eef2c21d2da3b9"); // Random hash

   QBitArray expectedResult(hashes.size());
   expectedResult[0] = true;
   expectedResult[1] = true;
   expectedResult[2] = false;
   expectedResult[3] = false;

   QBitArray result = this->fileManager->haveChunks(hashes);
   QCOMPARE(result.size(), hashes.size());

   for (int i = 0; i < result.size(); i++)
   {
      QVERIFY(result[i] == expectedResult[i]);
      qDebug() << hashes[i].toStr() << ":" << (result[i] ? "Yes" : "No");
   }
}

void Tests::printAmount()
{
   qDebug() << "===== printAmount() =====";

   qDebug() << "Sharing amount: " << this->fileManager->getAmount() << " bytes";
}

void Tests::rmSharedDirectory()
{
   qDebug() << "===== rmSharedDirectory() =====";

   this->sharedDirs.clear();
   this->fileManager->setSharedDirs(this->sharedDirs);
}

/**
  * The following case tests the Bloom filter performance
  * used in the class 'Chunks'.
  * The bloom filter can be enable in "Chunks.h".
  */
#include <priv/ChunkIndex/Chunks.h>
#include <priv/Cache/Chunk.h>
#include <priv/Cache/Directory.h>
#include <priv/Cache/File.h>
void Tests::chunksPerformance()
{
   qDebug() << "===== chunksPerformance() =====";
     
   const int HASH_POOL_SIZE = 100000;
   const int NB_HASHES_TO_CHECK = 10000000;
 
   Chunks chunks;
  
   for (int i = 0; i < HASH_POOL_SIZE; i++)
   {
      QSharedPointer<Chunk> chunk(new Chunk(nullptr, 0, 0));
      chunk->setHash(Common::Hash::rand());
      chunks.add(chunk);
   }

   Common::Hashes hashes;
   const int nbHashes = 100;
   for (int i = 0; i < nbHashes; i++)
      hashes << Common::Hash::rand();

   QElapsedTimer timer;
   timer.start();

   for (int i = 0; i < NB_HASHES_TO_CHECK; i++)
   {
      if(chunks.contains(hashes[i % nbHashes]))
         QFAIL("chunks cannot contains a random chunk");
   }

   qDebug() << "Time to check if" << NB_HASHES_TO_CHECK << "hashes exist among a pool of" << HASH_POOL_SIZE << "hashes:" << timer.elapsed() << "ms";
}

void Tests::cleanupTestCase()
{
   qDebug() << "===== cleanupTestCase() =====";
   // This call is only used to stop the fileUpdater and wait for it to finish.
   // It's should not be used in a normal code.
   this->fileManager.clear();
   QTest::qWait(200);
}

void Tests::createInitialFiles()
{
   this->deleteAllFiles();

   QVERIFY(Common::Global::createFile("sharedDirs/share1/subdir/o.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share1/subdir/p.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share1/another subdir/q.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share1/empty subdir/"));
   QVERIFY(Common::Global::createFile("sharedDirs/share1/r.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share1/s.txt"));

   QVERIFY(Common::Global::createFile("sharedDirs/share2/t.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share2/u.txt"));

   // "share3" is dedicated to the search feature.
   QVERIFY(Common::Global::createFile("sharedDirs/share3/aaaa bbbb cccc.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/aaaa bbbb.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/aaaaaa bbbb.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/aaaaaa bbbbbb.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/aaaa cccc.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/aaaa dddddd.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/aaaaaa dddddd.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/bbbb cccc.nfo"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/cccc bbbb.nfo"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/bbbb dddd.nfo"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/bbbb.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/cccc bbbbbb.txt"));
   QVERIFY(Common::Global::createFile("sharedDirs/share3/dddd.txt"));

   QVERIFY(Common::Global::createFile("incoming/"));
}

void Tests::deleteAllFiles()
{
   Common::Global::recursiveDeleteDirectory("sharedDirs");
   Common::Global::recursiveDeleteDirectory("incoming");
}

void Tests::printSearch(const QString& terms, const Protos::Common::FindResult& result)
{
   qDebug() << "Search: " << terms;
   for (int i = 0; i < result.entry_size(); i++)
      qDebug() << "[" << result.entry(i).level() << "] " << Common::ProtoHelper::getStr(result.entry(i).entry(), &Protos::Common::Entry::name);
}

void Tests::compareExpectedResult(const Protos::Common::FindResult& result, const FindResult& expectedResult)
{
   for (int i = 0; i < result.entry_size(); i++)
   {
      QVERIFY(expectedResult.contains(result.entry(i).level()));
      QVERIFY(expectedResult[result.entry(i).level()].contains(Common::ProtoHelper::getStr(result.entry(i).entry(), &Protos::Common::Entry::name)));
   }
}

void Tests::compareStrRegexp(const QString& regexp, const QString& str)
{
   QRegExp expected(regexp);
   if (!expected.exactMatch(str))
   {
      int l = expected.matchedLength();
      QByteArray message = QString("This string doesn't match the expected regular expression from character %1: \n%2").arg(l).arg(str).toUtf8();
      QFAIL(message.data());
   }
}

