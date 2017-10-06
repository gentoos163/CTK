/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.commontk.org/LICENSE

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

// Qt includes
#include <QApplication>
#include <QTest>

// CTK includes
#include "ctkDICOMDatabase.h"
#include "ctkDICOMBrowser.h"
#include "ctkScopedCurrentDir.h"
#include "ctkTest.h"
#include "ctkUtils.h"

//-----------------------------------------------------------------------------
class ctkDICOMBrowserTester: public QObject
{
  Q_OBJECT
private slots:

  void initTestCase();
  void init();

  void testDefaults();

  void testDatabaseDirectory();

  void testOnImportDirectories();
  void testOnImportDirectories_data();

  void testOnImportDirectory();
  void testOnImportDirectory_data();

private:
  void _testImportCommon(ctkDICOMBrowser& browser);

  QString TemporaryDatabaseDirectoryName;

  QDir DICOMDir;
};

// ----------------------------------------------------------------------------
void ctkDICOMBrowserTester::initTestCase()
{
  // Get data directory from environment
  QDir dataDir = QDir(
        QProcessEnvironment::systemEnvironment().value("CTKData_DIR", ""));
  QVERIFY(dataDir.exists());

  this->DICOMDir = dataDir.filePath("Data/DICOM");
  QVERIFY(this->DICOMDir.exists());
}

// ----------------------------------------------------------------------------
void ctkDICOMBrowserTester::init()
{
  // If any, remove default ./ctkDICOM-Database directory
  {
  ctkScopedCurrentDir scopedCurrentDir(QDir::tempPath());
  ctk::removeDirRecursively("./ctkDICOM-Database");
  QCOMPARE(QFileInfo("./ctkDICOM-Database").isDir(), false);
  }

  // Generate a new database directory name
  this->TemporaryDatabaseDirectoryName =
      QString("ctkDICOMBrowserTest.%1.DICOM-Database").arg(QTime::currentTime().toString("hhmmsszzz"));
}

// ----------------------------------------------------------------------------
void ctkDICOMBrowserTester::testDefaults()
{
  // Clear left over settings
  QSettings().remove(ctkDICOMBrowser::databaseDirectorySettingsKey());

  ctkDICOMBrowser browser;

  QVERIFY(QFileInfo("./ctkDICOM-Database").isDir());
  QVERIFY(QFileInfo("./ctkDICOM-Database/ctkDICOM.sql").isFile());
  QVERIFY(QFileInfo("./ctkDICOM-Database/ctkDICOMTagCache.sql").isFile());

  QCOMPARE(ctkDICOMBrowser::databaseDirectorySettingsKey(), QString("DatabaseDirectory"));

  QCOMPARE(browser.databaseDirectory(), QString("./ctkDICOM-Database"));
  QVERIFY(browser.database() != 0);

  QCOMPARE(browser.tagsToPrecache(), QStringList());

  QVERIFY(browser.dicomTableManager() != 0);

  QCOMPARE(browser.displayImportSummary(), true);
  QCOMPARE(browser.patientsAddedDuringImport(), 0);
  QCOMPARE(browser.studiesAddedDuringImport(), 0);
  QCOMPARE(browser.seriesAddedDuringImport(), 0);
  QCOMPARE(browser.instancesAddedDuringImport(), 0);

  QCOMPARE(browser.importDirectoryMode(), ctkDICOMBrowser::ImportDirectoryAddLink);
  QCOMPARE(browser.skipConfirmImportDirectory(), false);
}

// ----------------------------------------------------------------------------
void ctkDICOMBrowserTester::testDatabaseDirectory()
{
  // Check that value from setting is picked up
  {
  QSettings().setValue(ctkDICOMBrowser::databaseDirectorySettingsKey(), this->TemporaryDatabaseDirectoryName);
  QCOMPARE(QFileInfo(this->TemporaryDatabaseDirectoryName).isDir(), false);

  ctkDICOMBrowser browser;

  QCOMPARE(QFileInfo(browser.databaseDirectory()).absoluteFilePath(),
           QFileInfo(this->TemporaryDatabaseDirectoryName).absoluteFilePath());
  QVERIFY(QDir(browser.databaseDirectory()).exists());
  }
}

// ----------------------------------------------------------------------------
void ctkDICOMBrowserTester::testOnImportDirectories()
{
  QFETCH(QStringList, directories);
  QFETCH(ctkDICOMBrowser::ImportDirectoryMode, importDirectoryMode);

  QSettings().setValue(ctkDICOMBrowser::databaseDirectorySettingsKey(), this->TemporaryDatabaseDirectoryName);

  ctkDICOMBrowser browser;

  browser.setDisplayImportSummary(false);
  browser.onImportDirectories(directories, /* mode= */ importDirectoryMode, /* confirm= */ false);

  this->_testImportCommon(browser);
}

// ----------------------------------------------------------------------------
void ctkDICOMBrowserTester::_testImportCommon(ctkDICOMBrowser& browser)
{
  QFETCH(int, expectedImporedPatients);
  QFETCH(int, expectedImporedStudies);
  QFETCH(int, expectedImporedSeries);
  QFETCH(int, expectedImporedInstances);

  QCOMPARE(browser.patientsAddedDuringImport(), expectedImporedPatients);
  QCOMPARE(browser.studiesAddedDuringImport(), expectedImporedStudies);
  QCOMPARE(browser.seriesAddedDuringImport(), expectedImporedSeries);
  QCOMPARE(browser.instancesAddedDuringImport(), expectedImporedInstances);

  QFETCH(int, expectedTotalPatients);
  QFETCH(int, expectedTotalStudies);
  QFETCH(int, expectedTotalSeries);
  QFETCH(int, expectedTotalInstances);

  int currentPatientCount = 0;
  int currentStudyCount = 0;
  int currentSerieCount = 0;
  int currentInstanceCount = 0;

  QStringList patients =  browser.database()->patients();
  currentPatientCount += patients.count();
  foreach(const QString& patient, patients)
    {
    QStringList studies = browser.database()->studiesForPatient(patient);
    currentStudyCount += studies.count();
    foreach(const QString& study, studies)
      {
      QStringList series = browser.database()->seriesForStudy(study);
      currentSerieCount += series.count();
      foreach(const QString& serie, series)
        {
        QStringList instances = browser.database()->instancesForSeries(serie);
        currentInstanceCount += instances.count();
        }
      }
    }

  QCOMPARE(currentPatientCount, expectedTotalPatients);
  QCOMPARE(currentStudyCount, expectedTotalStudies);
  QCOMPARE(currentSerieCount, expectedTotalSeries);
  QCOMPARE(currentInstanceCount, expectedTotalInstances);
}

// ----------------------------------------------------------------------------
void ctkDICOMBrowserTester::testOnImportDirectories_data()
{
  this->testOnImportDirectory_data();
}

// ----------------------------------------------------------------------------
void ctkDICOMBrowserTester::testOnImportDirectory()
{
  QFETCH(QStringList, directories);
  QFETCH(ctkDICOMBrowser::ImportDirectoryMode, importDirectoryMode);

  QSettings().setValue(ctkDICOMBrowser::databaseDirectorySettingsKey(), this->TemporaryDatabaseDirectoryName);

  ctkDICOMBrowser browser;

  browser.setDisplayImportSummary(false);
  browser.onImportDirectory(directories[0], /* mode= */ importDirectoryMode, /* confirm= */ false);

  this->_testImportCommon(browser);
}

// ----------------------------------------------------------------------------
void ctkDICOMBrowserTester::testOnImportDirectory_data()
{
  QTest::addColumn<QStringList>("directories");
  QTest::addColumn<ctkDICOMBrowser::ImportDirectoryMode>("importDirectoryMode");
  QTest::addColumn<int>("expectedImporedPatients");
  QTest::addColumn<int>("expectedImporedStudies");
  QTest::addColumn<int>("expectedImporedSeries");
  QTest::addColumn<int>("expectedImporedInstances");
  QTest::addColumn<int>("expectedTotalPatients");
  QTest::addColumn<int>("expectedTotalStudies");
  QTest::addColumn<int>("expectedTotalSeries");
  QTest::addColumn<int>("expectedTotalInstances");

  QTest::newRow("1-MRHEAD")
      << /* directories */ (QStringList() << this->DICOMDir.filePath("MRHEAD"))
      << /* importDirectoryMode */ ctkDICOMBrowser::ImportDirectoryAddLink
      << /* expectedImporedPatients */ 1
      << /* expectedImporedStudies */ 1
      << /* expectedImporedSeries */ 1
      << /* expectedImporedInstances */ 100
      << /* expectedTotalPatients */ 1
      << /* expectedTotalStudies */ 1
      << /* expectedTotalSeries */ 1
      << /* expectedTotalInstances */ 100;
}

// ----------------------------------------------------------------------------
CTK_TEST_MAIN(ctkDICOMBrowserTest)
#include "moc_ctkDICOMBrowserTest.cpp"
