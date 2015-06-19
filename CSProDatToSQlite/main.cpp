#include <tclap/CmdLine.h>
#include <QtXml>
#include <QFile>
#include <QtSql>

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
  QList<TfieldDef> fields; //List of fields in a table
};
typedef tableDef TtableDef;

QList<TtableDef> tables; //List of tables
QList<fieldDef> mainID; //Main field ID
int mainIDSize; //Main ID Size
QSqlDatabase db; //SQLite Database
int nerrors;

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

void readDAT(QString datSource, QString logFile)
{
    /*
        This process reads the CSPro DAT and using the XML data stores in tables, it creates SQL
        inserts for each record
    */
    QFile file(datSource);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) //Try to open the file
        return;

    QFile log(logFile);
    if (!QFile::exists(logFile))
    {
        if (!log.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    }
    else
    {
        if (!log.open(QIODevice::Append | QIODevice::Text))
        return;
    }

    QTextStream out(&log);

    out << "Error\tCaseID\tRecordName\tRowInCSPro\tError\tSQL\n";

    QTextStream in(&file); //Input stream of data

    in.setCodec("UTF-8");

    QString code; //Table code
    int currPos; //Current position in one line of data
    int tablePos; //Position of the table in the list of tables
    int pos;
    QString value; //The value of a field in a dat file
    QString sqlinsert; //Insert SQL head
    QString sqlValues; //Insert SQL values
    int ndec; //Number of decimlas
    float fvalue; //Float value of a field in the dat file
    QString caseID; //Case ID
    int CSProID; //CSPro Row ID
    QString pCode; //Previous table code
    QSqlQuery query(db); //Query to the SQLite db
    int LineNo; //Line number of the data file.
    LineNo = 0;
    int size; //Size of the data beign read
    size = 0;
    CSProID = 0;

    int perc;
    perc = 0;

    db.exec("BEGIN TRANSACTION;"); //Begin an SQLite transaction for all inserts
    while (!in.atEnd()) //Goes throgh the DAT lines
    {
        LineNo++;
        QString line = in.readLine(); //Read the DAT line
        size = size + line.length();

        perc = (int)round((size*100)/file.size());

        printf("\r%i %% inserted in database", perc);

        if (!line.trimmed().isEmpty())
        {
            code = line[0];
            if (pCode != code)
            {
                caseID = "";
                pCode = code;
            }

            line = line.right(line.length()-1); //Remove the record code

            line = line + " "; //Add an extra space at the end. Fix some errors in codes when line is almost empty

            if (line.length() > mainIDSize) //Not include empty lines
            {
                tablePos = getTablePos(code);
                if (tablePos >= 0)
                {
                    currPos = 0;
                    sqlinsert = "INSERT INTO " + tables[tablePos].name + " (";
                    sqlValues = " VALUES (";

                    //If we change from caseID then we reset the CSProID otherwise increase it.
                    if (caseID != line.left(mainIDSize).trimmed())
                    {
                        CSProID = 1;
                        caseID = line.left(mainIDSize).trimmed();
                    }
                    else
                        CSProID++;

                    //Goes through all the files of the table
                    for (pos = 0; pos <= tables[tablePos].fields.count()-1;pos++)
                    {
                        //qDebug() << tables[tablePos].fields[pos].size;
                        if (currPos + tables[tablePos].fields[pos].size <= line.length()) //If the data of the field exist in the DAT line
                        {
                            value = line.mid(currPos,tables[tablePos].fields[pos].size); //Gets the value from the data file
                            currPos = currPos + tables[tablePos].fields[pos].size; //Moves to the next field of the dat line

                            sqlinsert = sqlinsert + tables[tablePos].fields[pos].name + ","; //Add the filed to the SQL insert head

                            if (tables[tablePos].fields[pos].type == "TEXT") //If the field is text
                            {
                                //We add the value is its not empty otherwise we add NULL
                                if (!value.simplified().simplified().isEmpty())
                                {
                                    value = value.simplified().replace("'","`");
                                    value = value.simplified().replace(";","");
                                    sqlValues = sqlValues + "'" + value + "',";
                                }
                                else
                                    sqlValues = sqlValues + "NULL,";
                            }
                            else
                            {
                                //If the field is numeric then we extract the data if its not empty
                                if (!value.simplified().isEmpty())
                                {
                                    ndec = tables[tablePos].fields[pos].decSize;
                                    if (ndec > 0) //The data has decimal positions
                                    {
                                        if (value.simplified().indexOf(".") >= 0) //The data has the decimal char separation
                                            sqlValues = sqlValues + value.simplified() + ",";
                                        else
                                        {
                                            //If the data has decimals but not separation then we transform the data to a float value with decimals
                                            fvalue = value.simplified().toFloat(); //Converts the value from string to float
                                            fvalue = fvalue / getFactor(ndec); //Divides the float value to a factor to extract decimals. For example 100 if the value has two decimals but not char separation
                                            sqlValues = sqlValues + QString::number(fvalue,'f',ndec) + ","; //Add the value to SQL insert values
                                        }
                                    }
                                    else
                                        sqlValues = sqlValues + value.simplified() + ","; //No decimals then add the value to the SQL insert values
                                }
                                else
                                    sqlValues = sqlValues + "NULL,"; //Data is empty then add NULL to the SQL insert values
                            }
                        }
                        else
                        {
                            int left;
                            left = line.length()-currPos;
                            if (left > 0)
                            {
                                QString leftStr;

                                leftStr = line.right(left);
                                if (!leftStr.simplified().isEmpty())
                                {
                                    value = leftStr; //Gets the value from what is left

                                    sqlinsert = sqlinsert + tables[tablePos].fields[pos].name + ","; //Add the filed to the SQL insert head

                                    if (tables[tablePos].fields[pos].type == "TEXT") //If the field is text
                                    {
                                        //We add the value is its not empty otherwise we add NULL
                                        if (!value.simplified().isEmpty())
                                        {
                                            value = value.simplified().replace("'","`");
                                            value = value.simplified().replace(";","");
                                            sqlValues = sqlValues + "'" + value + "',";
                                        }
                                        else
                                            sqlValues = sqlValues + "NULL,";
                                    }
                                    else
                                    {
                                        //If the field is numeric then we extract the data if its not empty
                                        if (!value.simplified().isEmpty())
                                        {
                                            ndec = tables[tablePos].fields[pos].decSize;
                                            if (ndec > 0) //The data has decimal positions
                                            {
                                                if (value.simplified().indexOf(".") >= 0) //The data has the decimal char separation
                                                    sqlValues = sqlValues + value.simplified() + ",";
                                                else
                                                {
                                                    //If the data has decimals but not separation then we transform the data to a float value with decimals
                                                    fvalue = value.simplified().toFloat(); //Converts the value from string to float
                                                    fvalue = fvalue / getFactor(ndec); //Divides the float value to a factor to extract decimals. For example 100 if the value has two decimals but not char separation
                                                    sqlValues = sqlValues + QString::number(fvalue,'f',ndec) + ","; //Add the value to SQL insert values
                                                }
                                            }
                                            else
                                                sqlValues = sqlValues + value.simplified() + ","; //No decimals then add the value to the SQL insert values
                                        }
                                        else
                                            sqlValues = sqlValues + "NULL,"; //Data is empty then add NULL to the SQL insert values
                                    }
                                }
                            }
                            break;
                        }
                    }
                    //Appends the uploaded = 0 and CSProID
                    sqlinsert = sqlinsert + "uploaded,CSProID)";
                    sqlValues = sqlValues + "0," + QString::number(CSProID) + ")";
                    sqlinsert = sqlinsert + sqlValues; //Connects the insert heading with the insert values in one Insert SQL                 

                    //sqlinsert = sqlinsert.replace(".00.","0");
                    sqlinsert = sqlinsert.replace("******","*****");
                    sqlinsert = sqlinsert.replace("*****","****");
                    sqlinsert = sqlinsert.replace("****","***");
                    sqlinsert = sqlinsert.replace("***","**");
                    sqlinsert = sqlinsert.replace("**","*");
                    sqlinsert = sqlinsert.replace("'*'","NULL");
                    sqlinsert = sqlinsert.replace("*","NULL");



                    if (!query.exec(sqlinsert)) //Executes the SQL
                    {
                        //Write in the log if there is an error.
                        nerrors++;
                        out << "Insert error in DAT line " + QString::number(LineNo) + "\t" + caseID + "\t" + tables[tablePos].name + "\t" + QString::number(CSProID) + "\t"  + query.lastError().databaseText() + "\t" + sqlinsert + "\n";
                    }
                }
            }
            else
            {
                //The line is empty - We just Increase or reset CSProID based on the CaseID
                tablePos = getTablePos(code);
                if (tablePos >= 0)
                {
                    //If we change case ID then we need to restart the CSPro ID
                    if (caseID != line.left(line.length()).trimmed())
                    {
                        CSProID = 1;
                        caseID = line.left(line.length()).trimmed();
                    }
                    else
                        CSProID++;
                }
            }
        }
    }
    db.exec("END TRANSACTION;"); //Ends the transaction to apply all the insert statements.
    if (perc != 100)
        printf("\r%i %% inserted in database\n", 100);
    else
        printf("\n");
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

        //qDebug() << field.name;

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

void readXML(QString xmlSource)
{
    /*
        This process reads the XML file and move each record into a list calles tables.
        Each table structure has the fields and their characteristics.
    */

    //Try to open the XML as an XML document
    QDomDocument doc("mydocument");
    QFile xmlfile(xmlSource);
    if (!xmlfile.open(QIODevice::ReadOnly))
        return;
    if (!doc.setContent(&xmlfile))
    {
        xmlfile.close();
        return;
    }
    xmlfile.close();

    QDomNodeList list; //List of XML tags
    QDomNodeList list2; //List of XML tags
    QDomElement item; //The item xml tag
    QDomElement record; //The record xml tag

    int pos;

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
        qDebug() << "Ups no ID items!";
        return;
    }
    mainIDSize = 0;
    //We create for each table a fields to store main item of the survey
    QList<fieldDef> mainRelField;
    for (pos = 0; pos <= mainID.count()-1;pos++)
    {
        fieldDef varmainRelField;

        varmainRelField.name = mainID[pos].name;
        varmainRelField.desc = mainID[pos].desc;
        varmainRelField.size = mainID[pos].size;
        mainIDSize = mainIDSize + mainID[pos].size;
        varmainRelField.type = mainID[pos].type;
        varmainRelField.decSize = 0;

        mainRelField.append(varmainRelField);

    }

    //We create the uploaded field
    fieldDef uploaded;
    uploaded.name = "uploaded";
    uploaded.desc = "uploaded";
    uploaded.size = 3;
    uploaded.type = "INTEGER";
    uploaded.decSize = 0;


    //We create the CSProID field
    fieldDef CSProID;
    CSProID.name = "CSProID";
    CSProID.desc = "CSProID";
    CSProID.size = 3;
    CSProID.type = "INTEGER";
    CSProID.decSize = 0;


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

            list = record.elementsByTagName("RecordTypeValue"); //Each record has an unique code of one character
            table.code = list.item(0).firstChild().nodeValue().replace("'","");
            //qDebug() << "TableCode: " + table.code;

            table.fields.append(mainRelField); //Appens the main item field

            list = record.elementsByTagName("Items"); //Get the tag for Items

            table.fields.append(getFields(list.item(0).firstChild())); //Appends each item to the table
            table.fields.append(uploaded); //Append the uploaded field
            table.fields.append(CSProID); //Append the CSProID field

            tables.append(table); //Append the table to the list

            record = record.nextSibling().toElement(); //Move to the next record
        }
    }
    else
    {
        qDebug() << "Ups no Records!";
        return;
    }
}



void genSQL(QString logFile)
{
    /*
        This process create the SQL create statements
    */

    QFile log(logFile);
    if (!QFile::exists(logFile))
    {
        if (!log.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    }
    else
    {
        if (!log.open(QIODevice::Append | QIODevice::Text))
        return;
    }

    QTextStream out(&log);

    QString sql;
    QSqlQuery query(db);
    db.exec("BEGIN TRANSACTION;");
    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        sql = "CREATE TABLE " + tables[pos].name + "(";
        for (int clm = 0; clm <= tables[pos].fields.count()-1; clm++)
        {
            sql = sql + tables[pos].fields[clm].name + " " + tables[pos].fields[clm].type + ",";
        }
        sql = sql.left(sql.length()-1) + ")";
        if (!query.exec(sql)) //Execute the sql statement
        {
            //If there is an error then report it to the log
            out << "Create Error: " + query.lastError().databaseText() + ". Executing: " + sql + "\n";
        }
    }
    db.exec("END TRANSACTION;");
}

int main(int argc, char *argv[])
{
    QString title;
    title = title + "****************************************************************** \n";
    title = title + " * DATtoSQLite 1.0                                                * \n";
    title = title + " * This tool generates a SQLite database using a CSPro DAT file.  * \n";
    title = title + " * The purpose is to use the SQLite database to control the       * \n";
    title = title + " * upload of data into MySQL. The SQLite database is used         * \n";
    title = title + " * by the Python script to upload the data into MySQL             * \n";
    title = title + " * This tool is part of CSPro Tools (c) ILRI, 2012                * \n";
    title = title + " ****************************************************************** \n";

    TCLAP::CmdLine cmd(title.toAscii().constData(), ' ', "1.0 (Beta 1)");
    //Required arguments
    TCLAP::ValueArg<std::string> xmlArg("x","inputXML","Input XML File",true,"","string");
    TCLAP::ValueArg<std::string> datArg("d","inputDAT","Input DAT file",true,"","string");
    TCLAP::ValueArg<std::string> liteArg("s","sqliteDB","Output sqlite database. Default ./output.sqlite",false,"./output.sqlite","string");
    TCLAP::ValueArg<std::string> logArg("l","logFile","Output log file. Default ./output.csv",false,"./output.csv","string");

    TCLAP::SwitchArg overwrite("w","overwrite","Overwrite log file if exists", cmd, true);


    cmd.add(xmlArg);
    cmd.add(datArg);
    cmd.add(liteArg);
    cmd.add(logArg);
    //Parsing the command lines
    cmd.parse( argc, argv );

    //Getting the variables from the command

    bool over = overwrite.getValue();
    QString xml = QString::fromUtf8(xmlArg.getValue().c_str());
    QString dat = QString::fromUtf8(datArg.getValue().c_str());
    QString lite = QString::fromUtf8(liteArg.getValue().c_str());
    QString log = QString::fromUtf8(logArg.getValue().c_str());


    if (over) //If overwrite
    {
        if (QFile::exists(log)) //If the file exists
            QFile::remove(log); //Remove it
    }


    db = QSqlDatabase::addDatabase("QSQLITE","dblite");

    db.setDatabaseName(lite); //Set the databasename according to the textbox in the UI
    if (!db.open()) //Try to opens the database
    {
        qDebug() << "Unable to establish a SQLite connection: " + db.lastError().databaseText();
        return 1;
    }

    nerrors = 0;

    readXML(xml); //Read the XML
    genSQL(log); //Generate the SQL create scripts
    readDAT(dat,log); //Read the dat file

    if (nerrors > 0)
        qDebug() << "Done. But error were encountered. Check the log file";
    else
        qDebug() << "Done. No errors were encountered";

    return 0;

}
