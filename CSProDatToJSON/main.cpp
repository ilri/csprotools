#include <tclap/CmdLine.h>
#include <QtXml>
#include <QFile>
#include <QDir>
#include <QtSql>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>

struct fieldDef
{
  QString name; //Field name
  QString desc; //Field description
  QString type; //Field type
  int size; //Field size
  int decSize; //Field decimal spaces
};
typedef fieldDef TfieldDef; //Structure to store fields

struct tableDef
{
  QString name; //Table name
  QString desc; //Table description
  QString code; //Table code. In CSPro each record is indentified by one char. For example 'A'
  bool mainTable;
  QList<TfieldDef> fields; //List of fields in a table
};
typedef tableDef TtableDef;

struct recordDef
{
    QString tableName;
    QStringList data;
    bool mainTable;
    QList<TfieldDef> fields;
};
typedef recordDef TrecordDef;

struct caseDef
{
  QString caseID; //Case ID
  QString caseUUID; //MD5 of the case ID. This will be the file name  
  QList<TrecordDef> records;
};
typedef caseDef TcaseDef;

QList<TtableDef> tables; //List of tables
QList<fieldDef> mainID; //Main field ID
QList<TcaseDef> cases; //List of cases in the DAT
int mainIDSize; //Main ID Size
int nerrors;
QString version;
bool overWriteJSON;

void logout(QString message)
{
    QString temp;
    temp = message + "\n";
    printf(temp.toUtf8().data());
}

int getTablePos(QString code)
{
    /*
        Returns the position of a table in the list of tables using the table code
    */
    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        if (tables[pos].code == code)
        {
            return pos;
        }
    }
    return -1;
}

float getFactor(int decPlaces)
{
    /*
        This fuction gets a divide factor to convert a numeric value in the DAT file
        to its value with decimals.

        In CSPro if the user indicate decimals the data entry separate the entry box between values
        and decimals but the data could be stored without the decimal character if
        decimal char = no. However if the data entry happened without the character
        it is not possible to include it later on. If we divide the value to a factor then we can
        get the decimals without the separator character.

    */
    QString res;
    res = "1";
    for (int pos = 1; pos <= decPlaces; pos++)
    {
        res = res + "0";
    }
    return res.toFloat();
}

int getCaseIndex(QString caseID)
{
    for (int pos = 0; pos < cases.count(); pos++)
    {
        if (cases[pos].caseID == caseID)
            return pos;
    }
    return -1;
}

int getRecordPos(TcaseDef caseID, QString tableName)
{
    for (int pos = 0; pos < caseID.records.count();pos++)
    {
        if (caseID.records[pos].tableName == tableName)
        {
            return pos;
        }
    }
    return -1;
}

int checkValue(QString value,QString caseID, QString table, QString file, int RowID, QTextStream &out)
{
    bool warning;
    warning = false;
    if (value == "*")
        warning = true;
    else
    {
        if (value.indexOf("**") >= 0)
            warning = true;
    }
    if (warning)
    {
        out << file + "\t";
        out << caseID + "\t";
        out << table + "\t";
        out << QString::number(RowID) + "\t";
        out << "Value may be corrupted and is stored with ** \n";
        nerrors++;
        return 1;
    }
    return 0;
}

void storeCase(QJsonObject &jsonObj,QList<TfieldDef> fields,QStringList data,QString caseID,QString record, QString file, QTextStream &out)
{
    int currPos;
    QString value;
    float fvalue;
    int ndec;
    int row;
    row = 0;
    for (int idx = 0; idx < data.count(); idx++)
    {
        currPos = 0;
        row++;
        for (int pos = 0; pos <= fields.count()-1;pos++)
        {
            if (currPos + fields[pos].size <= data[idx].length()) //If the data of the field exist in the DAT line
            {
                value = data[idx].mid(currPos,fields[pos].size); //Gets the value from the data file
                currPos = currPos + fields[pos].size; //Moves to the next field of the dat line
                if (checkValue(value,caseID,record,file,row,out) == 0)
                {
                    if (fields[pos].type == "TEXT") //If the field is text
                    {
                        //We add the value is its not empty otherwise we add NULL
                        if (!value.simplified().simplified().isEmpty())
                        {
                            value = value.simplified().replace("'","`");
                            value = value.simplified().replace(",","");
                            jsonObj[fields[pos].name] = value;
                        }
                    }
                    else
                    {
                        //If the field is numeric then we extract the data if its not empty
                        if (!value.simplified().isEmpty())
                        {
                            ndec = fields[pos].decSize;
                            if (ndec > 0) //The data has decimal positions
                            {
                                if (value.simplified().indexOf(".") >= 0) //The data has the decimal char separation
                                {
                                    jsonObj[fields[pos].name] = value.simplified().toFloat();
                                }
                                else
                                {
                                    //If the data has decimals but not separation then we transform the data to a float value with decimals
                                    fvalue = value.simplified().toFloat(); //Converts the value from string to float
                                    fvalue = fvalue / getFactor(ndec); //Divides the float value to a factor to extract decimals. For example 100 if the value has two decimals but not char separation
                                    jsonObj[fields[pos].name] = QString::number(fvalue,'f',ndec).toFloat(); //Add the value as float
                                }
                            }
                            else
                                jsonObj[fields[pos].name] = value.simplified().toInt(); //No decimals then add the value as integer
                        }
                    }
                }
                else
                    jsonObj[fields[pos].name] = value;
            }
            else
            {
                int left;
                left = data[idx].length()-currPos;
                if (left > 0)
                {
                    QString leftStr;

                    leftStr = data[idx].right(left);
                    if (!leftStr.simplified().isEmpty())
                    {
                        value = leftStr; //Gets the value from what is left
                        if (checkValue(value,caseID,record,file,row,out) == 0)
                        {
                            if (fields[pos].type == "TEXT") //If the field is text
                            {
                                //We add the value is its not empty otherwise we add NULL
                                if (!value.simplified().isEmpty())
                                {
                                    value = value.simplified().replace("'","`");
                                    value = value.simplified().replace(",","");
                                    jsonObj[fields[pos].name] = value;
                                }
                            }
                            else
                            {
                                //If the field is numeric then we extract the data if its not empty
                                if (!value.simplified().isEmpty())
                                {
                                    ndec = fields[pos].decSize;
                                    if (ndec > 0) //The data has decimal positions
                                    {
                                        if (value.simplified().indexOf(".") >= 0) //The data has the decimal char separation
                                            jsonObj[fields[pos].name] = value.simplified().toFloat();
                                        else
                                        {
                                            //If the data has decimals but not separation then we transform the data to a float value with decimals
                                            fvalue = value.simplified().toFloat(); //Converts the value from string to float
                                            fvalue = fvalue / getFactor(ndec); //Divides the float value to a factor to extract decimals. For example 100 if the value has two decimals but not char separation
                                            jsonObj[fields[pos].name] = QString::number(fvalue,'f',ndec).toFloat();
                                        }
                                    }
                                    else
                                        jsonObj[fields[pos].name] = value.simplified().toInt(); //No decimals then add the value as integer
                                }
                            }
                        }
                        else
                            jsonObj[fields[pos].name] = value;
                    }
                }
                break;
            }
        }
    }
}

void storeRecord(QJsonArray &jsonArray,QList<TfieldDef> fields,QStringList data,QString caseID,QString record, QString file, QTextStream &out)
{
    int currPos;
    QString value;
    float fvalue;
    int ndec;
    int row;
    row = 0;
    for (int idx = 0; idx < data.count(); idx++)
    {
        currPos = 0;
        row++;
        QJsonObject jsonObj;
        for (int pos = 0; pos <= fields.count()-1;pos++)
        {
            if (currPos + fields[pos].size <= data[idx].length()) //If the data of the field exist in the DAT line
            {
                value = data[idx].mid(currPos,fields[pos].size); //Gets the value from the data file
                currPos = currPos + fields[pos].size; //Moves to the next field of the dat line
                if (checkValue(value,caseID,record,file,row,out) == 0)
                {
                    if (fields[pos].type == "TEXT") //If the field is text
                    {
                        //We add the value is its not empty otherwise we add NULL
                        if (!value.simplified().simplified().isEmpty())
                        {
                            value = value.simplified().replace("'","`");
                            value = value.simplified().replace(",","");
                            jsonObj[fields[pos].name] = value;
                        }
                    }
                    else
                    {
                        //If the field is numeric then we extract the data if its not empty
                        if (!value.simplified().isEmpty())
                        {
                            ndec = fields[pos].decSize;
                            if (ndec > 0) //The data has decimal positions
                            {
                                if (value.simplified().indexOf(".") >= 0) //The data has the decimal char separation
                                {
                                    jsonObj[fields[pos].name] = value.simplified().toFloat();
                                }
                                else
                                {
                                    //If the data has decimals but not separation then we transform the data to a float value with decimals
                                    fvalue = value.simplified().toFloat(); //Converts the value from string to float
                                    fvalue = fvalue / getFactor(ndec); //Divides the float value to a factor to extract decimals. For example 100 if the value has two decimals but not char separation
                                    jsonObj[fields[pos].name] = QString::number(fvalue,'f',ndec).toFloat(); //Add the value as float
                                }
                            }
                            else
                                jsonObj[fields[pos].name] = value.simplified().toInt(); //No decimals then add the value as integer
                        }
                    }
                }
                else
                    jsonObj[fields[pos].name] = value;
            }
            else
            {
                int left;
                left = data[idx].length()-currPos;
                if (left > 0)
                {
                    QString leftStr;

                    leftStr = data[idx].right(left);
                    if (!leftStr.simplified().isEmpty())
                    {
                        value = leftStr; //Gets the value from what is left
                        if (checkValue(value,caseID,record,file,row,out) == 0)
                        {
                            if (fields[pos].type == "TEXT") //If the field is text
                            {
                                //We add the value is its not empty otherwise we add NULL
                                if (!value.simplified().isEmpty())
                                {
                                    value = value.simplified().replace("'","`");
                                    value = value.simplified().replace(",","");
                                    jsonObj[fields[pos].name] = value;
                                }
                            }
                            else
                            {
                                //If the field is numeric then we extract the data if its not empty
                                if (!value.simplified().isEmpty())
                                {
                                    ndec = fields[pos].decSize;
                                    if (ndec > 0) //The data has decimal positions
                                    {
                                        if (value.simplified().indexOf(".") >= 0) //The data has the decimal char separation
                                            jsonObj[fields[pos].name] = value.simplified().toFloat();
                                        else
                                        {
                                            //If the data has decimals but not separation then we transform the data to a float value with decimals
                                            fvalue = value.simplified().toFloat(); //Converts the value from string to float
                                            fvalue = fvalue / getFactor(ndec); //Divides the float value to a factor to extract decimals. For example 100 if the value has two decimals but not char separation
                                            jsonObj[fields[pos].name] = QString::number(fvalue,'f',ndec).toFloat();
                                        }
                                    }
                                    else
                                        jsonObj[fields[pos].name] = value.simplified().toInt(); //No decimals then add the value as integer
                                }
                            }
                        }
                        else
                            jsonObj[fields[pos].name] = value;
                    }
                }
                break;
            }
        }
        jsonArray.append(jsonObj);
    }
}

int readDAT(QString datSource, QString logFile, QDir outdir)
{
    /*
        This process reads the CSPro DAT and using the XML data stores in tables, it creates JSON
        files for each case
    */
    QFile file(datSource);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) //Try to open the file
        return 1;

    QFile log(logFile);
    if (!QFile::exists(logFile))
    {
        if (!log.open(QIODevice::WriteOnly | QIODevice::Text))
        return 1;
    }
    else
    {        
        if (!log.open(QIODevice::Append | QIODevice::Text))
        return 1;
    }
    QTextStream out(&log);


    out << "File\tCaseID\tRecordName\tRowInCSPro\tError\n";

    QTextStream in(&file); //Input stream of data

    in.setCodec("UTF-8");

    QString code; //Table code

    int tablePos; //Position of the table in the list of tables           
    int size; //Size of the data beign read
    size = 0;

    int recordIndex;
    int caseIndex;
    // We pass throgh the file and organize the data into two lists:
    // cases and records
    while (!in.atEnd()) //Goes throgh the DAT lines
    {
        QString line = in.readLine(); //Read the DAT line
        size = size + line.length();

        if (!line.trimmed().isEmpty())
        {
            if (version == "5")
                code = line[0];
            else
                code = line.left(3);


            if (version == "5")
                line = line.right(line.length()-1); //Remove the record code CSPro 5
            else
                line = line.right(line.length()-3); //Remove the record code CSPro 6

            line = line + " "; //Add an extra space at the end. Fix some errors in codes when line is almost empty

            if (line.length() > mainIDSize) //Not include empty lines
            {
                tablePos = getTablePos(code);
                if (tablePos >= 0)
                {
                    caseIndex = getCaseIndex(line.left(mainIDSize).trimmed());
                    if (caseIndex >= 0)
                    {
                        recordIndex = getRecordPos(cases[caseIndex],tables[tablePos].name);
                        if (recordIndex >= 0)
                        {
                            //Add just the data to the record
                            cases[caseIndex].records[recordIndex].data.append(line);
                        }
                        else
                        {
                            //Add the records to the case
                            TrecordDef aRecord;
                            aRecord.tableName = tables[tablePos].name;
                            aRecord.mainTable = tables[tablePos].mainTable;
                            aRecord.fields.append(tables[tablePos].fields);
                            aRecord.data.append(line);
                            cases[caseIndex].records.append(aRecord);
                        }
                    }
                    else
                    {
                        //Creates the case                        
                        TcaseDef aCase;
                        aCase.caseID = line.left(mainIDSize).trimmed();                        
                        aCase.caseUUID = QString(QCryptographicHash::hash(aCase.caseID.toUtf8(),QCryptographicHash::Md5).toHex());
                        //Add the records to the case
                        TrecordDef aRecord;
                        aRecord.tableName = tables[tablePos].name;
                        aRecord.mainTable = tables[tablePos].mainTable;
                        aRecord.fields.append(tables[tablePos].fields);
                        aRecord.data.append(line);
                        aCase.records.append(aRecord);
                        cases.append(aCase);
                    }
                }
            }
        }
    }

    QString fileName;

    //Creates the JSON files
    for (int cn = 0; cn < cases.count(); cn++)
    {
        fileName = outdir.absolutePath() + "/" + cases[cn].caseUUID + ".json";
        QFile saveFile(fileName);
        if (saveFile.exists() and !overWriteJSON)
        {
            logout(fileName + " already exists. Ignoring it");
            continue;
        }

        if (!saveFile.open(QIODevice::WriteOnly))
        {
            logout("Couldn't store file " + cases[cn].caseUUID + ".json");
            return 1;
        }
        QTextStream out(&saveFile);
        out.setCodec("UTF-8");
        QJsonObject mCaseObj;
        for (int rn = 0; rn < cases[cn].records.count(); rn++)
        {
            if (cases[cn].records[rn].mainTable)
            {
                storeCase(mCaseObj,cases[cn].records[rn].fields,cases[cn].records[rn].data,cases[cn].caseID,cases[cn].records[rn].tableName,cases[cn].caseUUID,out);
            }
            else
            {
                QJsonArray jsonArray;
                storeRecord(jsonArray,cases[cn].records[rn].fields,cases[cn].records[rn].data,cases[cn].caseID,cases[cn].records[rn].tableName,cases[cn].caseUUID,out);
                if (jsonArray.count() > 0)
                    mCaseObj[cases[cn].records[rn].tableName] = jsonArray;
            }
        }
        QJsonDocument saveDoc(mCaseObj);
        //saveFile.write(saveDoc.toJson());
        out << saveDoc.toJson();
        saveFile.close();
    }
    return 0;
}

QList<TfieldDef> getFields(QDomNode node)
{
    /*
        This process reads each Record of the CSPro XML and extract the information for each field.
        The process converts the CSPro data types to SQLite data types. For example Alpha to Text
    */
    QList<TfieldDef> res;
    QDomNodeList list;
    QDomElement item;
    QString type; //If the item is a Subitem or an item
    while (!node.isNull()) //Goes through all the items of the record extracting information
    {
        TfieldDef field;
        field.decSize = 0;
        item = node.toElement();
        list = item.elementsByTagName("Label");
        field.desc = list.item(0).firstChild().nodeValue();
        list = item.elementsByTagName("Name");
        field.name = list.item(0).firstChild().nodeValue().toLower();

        list = item.elementsByTagName("Len");
        field.size = list.item(0).firstChild().nodeValue().toInt();

        list = item.elementsByTagName("ItemType");
        if (list.count() > 0)
            type = list.item(0).firstChild().nodeValue();
        else
            type = "";

        list = item.elementsByTagName("DataType");
        if (list.count() > 0)
        {
            if (list.item(0).firstChild().nodeValue() == "Alpha")
                field.type = "TEXT";
            else
            {
                list = item.elementsByTagName("Decimal");
                if (list.count() > 0)
                {
                    field.type = "FLOAT";
                }
                else
                    field.type = "INTEGER";
            }
        }
        else
        {
            list = item.elementsByTagName("Decimal");
            if (list.count() > 0)
            {
                field.type = "FLOAT";
                field.decSize = list.item(0).firstChild().nodeValue().toInt();
            }
            else
                field.type = "INTEGER";
        }
        if (type != "SubItem") //We dont append subitems
            res.append(field);

        node = node.nextSibling(); //Move to the next item
    }

    return res;
}

int readXML(QString xmlSource, QString mainTable)
{
    /*
        This process reads the XML file and move each record into a list calles tables.
        Each table structure has the fields and their characteristics.
    */

    //Try to open the XML as an XML document
    QDomDocument doc("mydocument");
    QFile xmlfile(xmlSource);
    if (!xmlfile.open(QIODevice::ReadOnly))
        return 1;
    if (!doc.setContent(&xmlfile))
    {
        xmlfile.close();
        return 1;
    }
    xmlfile.close();

    QDomNodeList list; //List of XML tags
    QDomNodeList list2; //List of XML tags
    QDomElement item; //The item xml tag
    QDomElement record; //The record xml tag

    int pos;
    mainIDSize = 0;
    //This section of code extract the main item of the survey
    list = doc.elementsByTagName("IdItems");
    if (list.count() > 0)
    {
        item = list.item(0).toElement();
        list = item.elementsByTagName("Item");

        for (pos = 0; pos <= list.count()-1;pos++)
        {
            fieldDef varmainID;

            item = list.item(pos).toElement();
            list2 = item.elementsByTagName("Label");
            varmainID.desc = list2.item(0).firstChild().nodeValue().toLower();
            list2 = item.elementsByTagName("Name");
            varmainID.name = list2.item(0).firstChild().nodeValue();
            list2 = item.elementsByTagName("Len");
            varmainID.size = list2.item(0).firstChild().nodeValue().toInt();

            mainIDSize = mainIDSize + varmainID.size;

            list2 = item.elementsByTagName("DataType");
            if (list2.count() > 0)
            {
                if (list2.item(0).firstChild().nodeValue() == "Alpha")
                    varmainID.type = "TEXT";
                else
                    varmainID.type = "INTEGER";
            }
            else
            {
                varmainID.type = "INTEGER";
            }
            varmainID.decSize = 0;

            mainID.append(varmainID);

        }
    }
    else
    {
        logout("Ups no ID items!");
        return 1;
    }

    QList<fieldDef> mainRelField;
    for (pos = 0; pos <= mainID.count()-1;pos++)
    {
        fieldDef varmainRelField;

        varmainRelField.name = mainID[pos].name;
        varmainRelField.desc = mainID[pos].desc;
        varmainRelField.size = mainID[pos].size;
        varmainRelField.type = mainID[pos].type;
        varmainRelField.decSize = 0;

        mainRelField.append(varmainRelField);

    }

    list = doc.elementsByTagName("Records"); //Get the record tags
    if (list.count() > 0)
    {
        record = list.item(0).firstChild().toElement(); //First record
        while (!record.isNull()) //Goes through each record and extract the information
        {
            TtableDef table;
            list = record.elementsByTagName("Label");
            table.desc = list.item(0).firstChild().nodeValue();
            list = record.elementsByTagName("Name");
            table.name = list.item(0).firstChild().nodeValue().toLower();
            if (table.name.trimmed() == mainTable.toLower().trimmed())
                table.mainTable = true;
            else
                table.mainTable = false;

            list = record.elementsByTagName("RecordTypeValue"); //Each record has an unique code of one character in CSPro <= 5 and 3 characters in CSPro >= 6
            table.code = list.item(0).firstChild().nodeValue().replace("'","");

            table.fields.append(mainRelField); //Appens the main item field

            list = record.elementsByTagName("Items"); //Get the tag for Items

            table.fields.append(getFields(list.item(0).firstChild())); //Appends each item to the table


            tables.append(table); //Append the table to the list

            record = record.nextSibling().toElement(); //Move to the next record
        }
    }
    else
    {
        logout("Ups no Records!");
        return 1;
    }
    return 0;
}





int main(int argc, char *argv[])
{
    QString title;
    title = title + "****************************************************************** \n";
    title = title + " * DATtoJSON 1.0                                                  * \n";
    title = title + " * This tool generates JSON files using a CSPro DAT file.         * \n";
    title = title + " * The purpose is to use JSON files to control the                * \n";
    title = title + " * upload of data into MySQL. The JSON files are processed by     * \n";
    title = title + " * JSONToMySQL.                                                   * \n";
    title = title + " * This tool is part of CSPro Tools (c) ILRI, 2016                * \n";
    title = title + " * DATtoJSON is maintained by Carlos Quiros (cquiros@qlands.com)  * \n";
    title = title + " ****************************************************************** \n";

    TCLAP::CmdLine cmd(title.toUtf8().constData(), ' ', "1.0");
    //Required arguments
    TCLAP::ValueArg<std::string> xmlArg("x","inputXML","Input XML File",true,"","string");
    TCLAP::ValueArg<std::string> datArg("d","inputDAT","Input DAT file",true,"","string");
    TCLAP::ValueArg<std::string> outArg("o","outputDir","Output directory for the JSON files",true,"","string");
    TCLAP::ValueArg<std::string> logArg("l","logFile","Output error log file. Default ./error.csv",false,"./output.csv","string");
    TCLAP::ValueArg<std::string> tableArg("t","mainTable","Main record of the CSPro file",true,"","string");
    TCLAP::ValueArg<std::string> verArg("v","Version","CSPro version (5 for CSPro <= 5 or 6 for CSPro >=6).",true,"","string");
    TCLAP::SwitchArg overwritelog("w","overwritelog","Overwrite log file if exists", cmd, true);
    TCLAP::SwitchArg overwritejson("W","overwritejson","Overwrite JSON file if exists", cmd, false);


    cmd.add(xmlArg);
    cmd.add(datArg);
    cmd.add(outArg);
    cmd.add(logArg);
    cmd.add(verArg);
    cmd.add(tableArg);
    //Parsing the command lines
    cmd.parse( argc, argv );

    //Getting the variables from the command

    bool overlog = overwritelog.getValue();
    overWriteJSON = overwritejson.getValue();
    QString xml = QString::fromUtf8(xmlArg.getValue().c_str());
    QString dat = QString::fromUtf8(datArg.getValue().c_str());
    QString outdir = QString::fromUtf8(outArg.getValue().c_str());
    QString mainTable = QString::fromUtf8(tableArg.getValue().c_str());
    QString log = QString::fromUtf8(logArg.getValue().c_str());
    version = QString::fromUtf8(verArg.getValue().c_str());

    QDir dir(outdir);
    if (!dir.exists())
    {
        logout("Target directory does not exists");
        return 1;
    }

    if (overlog) //If overwrite
    {
        if (QFile::exists(log)) //If the file exists
            QFile::remove(log); //Remove it
    }

    nerrors = 0;

    if (readXML(xml,mainTable) == 1)
    {
        return 1;
    }
    readDAT(dat,log,dir); //Read the dat file

    if (nerrors > 0)
    {
        logout("Done. But error were encountered. Check the log file");
        return 1;
    }

    else
    {
        logout("Done. No errors were encountered");
        return 1;
    }

    return 0;

}
