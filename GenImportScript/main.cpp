#include <QCoreApplication>
#include <tclap/CmdLine.h>
#include <QFile>
#include <QDomDocument>
#include <QDomNodeList>
#include <QDomNode>
#include <QDomElement>
#include <QTextStream>
#include <QDateTime>
#include <QStringList>

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
  QList<TfieldDef> fields; //List of fields in a table
};
typedef tableDef TtableDef;

QList<TtableDef> tables; //List of tables
QStringList mainSurveyID;

QString mainTable;

void printLog(QString message)
{
    QString temp;
    temp = message + "\n";
    printf(temp.toLocal8Bit().data());
}

QString prefix;

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

int readXML(QString xmlSource)
{
    /*
        This process reads the XML file and move each record into a list calles tables.
        Each table structure has the fields and their characteristics.
    */

    //Try to open the XML as an XML document
    QDomDocument doc("mydocument");
    QFile xmlfile(xmlSource);
    if (!xmlfile.open(QIODevice::ReadOnly))
        return -1;
    if (!doc.setContent(&xmlfile))
    {
        xmlfile.close();
        return -1;
    }
    xmlfile.close();

    QDomNodeList list;
    QDomNodeList list2;//List of XML tags
    QDomElement item; //The item xml tag
    QDomElement record; //The record xml tag

    QList<TfieldDef> mainID;

    int pos;

    //This section of code extract the main item of the survey
    list = doc.elementsByTagName("IdItems");
    if (list.count() > 0)
    {
        item = list.item(0).toElement();
        list = item.elementsByTagName("Item");
        if (list.count() > 0)
        {
            for (pos = 0; pos <= list.count()-1;pos++)
            {
                TfieldDef varmainID;

                item = list.item(pos).toElement();

                list2 = item.elementsByTagName("Label");
                varmainID.desc = list2.item(0).firstChild().nodeValue().toLower();
                list2 = item.elementsByTagName("Name");
                varmainID.name = list2.item(0).firstChild().nodeValue();

                list2 = item.elementsByTagName("Len");
                varmainID.size = list2.item(0).firstChild().nodeValue().toInt();
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
    }
    else
    {
        printLog("Ups no ID items!");
        return -1;
    }

    //We create for each table a fields to store main item of the survey
    QList<TfieldDef> mainRelField;
    for (pos = 0; pos <= mainID.count()-1;pos++)
    {
        TfieldDef varmainRelField;

        varmainRelField.name = mainID[pos].name;

        mainSurveyID.append(varmainRelField.name);

        varmainRelField.desc = mainID[pos].desc;
        varmainRelField.size = mainID[pos].size;
        varmainRelField.type = mainID[pos].type;
        varmainRelField.decSize = 0;

        mainRelField.append(varmainRelField);
    }



    fieldDef childKey;
    childKey.name = "record_id";
    childKey.desc = "Record ID";
    childKey.size = 3;
    childKey.type = "INT";
    childKey.decSize = 0;

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

            table.fields.append(mainRelField); //Appens the main item field

            if (table.name.simplified() != mainTable.simplified().toLower())
                table.fields.append(childKey); //Appens the child key field

            list = record.elementsByTagName("Items"); //Get the tag for Items

            table.fields.append(getFields(list.item(0).firstChild())); //Appends each item to the table

            tables.append(table); //Append the table to the list

            record = record.nextSibling().toElement(); //Move to the next record
        }
    }
    else
    {
        printLog("Ups no Records!");
        return -1;
    }
    return 0;
}

void addTables(QTextStream &out)
{
    QString str;
    QString smainSurveyID;
    int pos2;
    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        out << "    if table.upper() == '" + tables[pos].name.toUpper() + "':" << "\n";
        out << "         for row in rows: #Pass through each row of " + tables[pos].name.toUpper() << "\n";
        out << "            rowID = row[\"rowId\"] #Gets the current row" << "\n";
        out << "            #Constructs an insert SQL" << "\n";
        str = "            sql = 'INSERT INTO " + prefix + tables[pos].name.toLower() + "(";
        for (pos2 = 0; pos2 <= tables[pos].fields.count()-1;pos2++)
        {
            str = str + tables[pos].fields[pos2].name.toLower() + ",";
        }
        str = str.left(str.length()-1) + "'";
        out << str << "\n";
        out << "            sql = sql + ') VALUES ('" << "\n";
        for (pos2 = 0; pos2 <= tables[pos].fields.count()-2;pos2++)
        {
            if (tables[pos].fields[pos2].name.toLower() != "record_id")
                out << "            sql = sql + \"'\" + correctString(row[\"" + tables[pos].fields[pos2].name.toLower() + "\"]) + \"',\"" << "\n";
            else
                out << "            sql = sql + \"'\" + correctString(row[\"csproID\"]) + \"',\"" << "\n";
        }
        if (tables[pos].fields.count() > 0)
        {
            if (tables[pos].fields[tables[pos].fields.count()-1].name.toLower() != "record_id")
                out << "            sql = sql + \"'\" + correctString(row[\"" + tables[pos].fields[tables[pos].fields.count()-1].name.toLower() + "\"]) + \"')\"" << "\n";
            else
                out << "            sql = sql + \"'\" + correctString(row[\"csproID\"]) + \"')\"" << "\n";
        }
        out << "            sql = sql.replace(\"'NULL'\", \"NULL\")" << "\n";
        out << "\n";
        out << "            try:" << "\n";
        out << "                myCur.execute(sql)  #Send the insert to MySQL" << "\n";
        out << "                liteCur.execute('UPDATE ' + table + ' SET uploaded = 1 WHERE rowId = ' + str(rowID))" << "\n";
        out << "            except MySQLdb.Error,  e:" << "\n";
        out << "                #If there is an error then write the information to the log file" << "\n";
        out << "                # csproID is the row in CSPro containing the problem for each CaseID" << "\n";

        smainSurveyID = "";
        for (pos2 = 0; pos2 <= mainSurveyID.count()-1;pos2++)
        {
            smainSurveyID = smainSurveyID + "correctString(row[\"" + mainSurveyID[pos2] + "\"]) + \"-\" + ";
        }
        smainSurveyID = smainSurveyID.left(smainSurveyID.length()-9);

        out << "                writeLog(cntMy, cntLite, logFile, table, rowID," + smainSurveyID + ",row[\"csproID\"] ,e.args[0],e.args[1], sql )" << "\n";
        out << "         cntMy.commit() #Commits the inserts in MySQL" << "\n";
        out << "         cntLite.commit() #Commits updates in SQLite" << "\n\n";
    }
}

int getPython(QString outFile)
{
    QFile file(":/base.py");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        printLog("Cannot read base script file");
        return -1;
    }

    QFile ofile(outFile);
    if (!ofile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        printLog("Cannot write to output file");
        return -1;
    }

    QTextStream out(&ofile);

    QTextStream in(&file);
    int lineNo;
    lineNo = 1;
    bool addTbls;
    addTbls = false;

    //Contructs the heading
    out << "#!/usr/bin/python" << "\n";
    out << "# -*- coding: utf-8 -*-" << "\n\n";

    QDateTime date;
    date = QDateTime::currentDateTime();

    out << "# Code generated by META" << "\n";
    out << "# Created: " << date.toString("ddd MMMM d yyyy h:m:s ap") << "\n";
    out << "# by: CSProToPythonScript 1.0" << "\n";
    out << "# WARNING! All changes made in this file might be lost when running CSProToPythonScript!" << "\n";
    out << "# IMPORTANT NOTE: This import script is made to fit the MySQL schema created by CSProToMysqlScript. " << "\n";
    out << "#                 If you modify the schema then you need to implement those changes in this script" << "\n";
    out << "#                 CSProToPythonScript DOES NOT SYNCHRONIZE any changes you made in the MySQL schema" << "\n\n";

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (lineNo >= 9)
        {
            if ((lineNo <= 32) || (lineNo >= 80))
                out << line << "\n";
            else
            {
                if (addTbls == false)
                {
                    addTables(out);
                    addTbls = true;
                }
            }
        }
        lineNo++;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    QString title;
    title = title + "****************************************************************** \n";
    title = title + " * CSProToPythonSCript 1.0                                        * \n";
    title = title + " * This tool reads a CSPRO XML and generates a Python script that * \n";
    title = title + " * moves the data from a SQLite database to a MySQL database      * \n";
    title = title + " * This tool is part of CSPro Tools (c) ILRI, 2012                * \n";
    title = title + " ****************************************************************** \n";

    TCLAP::CmdLine cmd(title.toAscii().data(), ' ', "1.0 (Beta 1)");
    //Required arguments
    TCLAP::ValueArg<std::string> inputArg("x","input","Input CSPro XML File",true,"","string");
    TCLAP::ValueArg<std::string> outputArg("o","output","Output Python script File. Default ./output.py",false,"./output.py","string");
    TCLAP::ValueArg<std::string> mainTableArg("t","mainTable","Main table of the survey.",true,"","string");
    TCLAP::ValueArg<std::string> prefixArg("p","prefix","Table prefix. _ will be added. Default empty",false,"","string");

    cmd.add(inputArg);
    cmd.add(outputArg);
    cmd.add(mainTableArg);
    cmd.add(prefixArg);
    //Parsing the command lines
    cmd.parse( argc, argv );

    //Getting the variables from the command

    QString input = QString::fromUtf8(inputArg.getValue().c_str());
    QString output = QString::fromUtf8(outputArg.getValue().c_str());
    prefix = QString::fromUtf8(prefixArg.getValue().c_str());

    if (prefix != "")
        prefix = prefix.trimmed() + "_";

    mainTable = QString::fromUtf8(mainTableArg.getValue().c_str());

    if (QFile::exists(input))
    {
        if (readXML(input) == 0)
        {
            getPython(output);
        }
        else
        {
            printLog("Error reading XML file");
            return -1;
        }
    }
    else
    {
        printLog("Input XML file does not exists");
        return -1;
    }

    return 0;
}
