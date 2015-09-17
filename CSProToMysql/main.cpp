#include <tclap/CmdLine.h>
#include <QFile>
#include <QtXml>

//Field Definition structure
struct fieldDef
{
  QString name;
  QString desc;
  QString type;
  int size;
  int decSize;
  QString rTable;
  QString rField;
  bool key;
  bool join;
};
typedef fieldDef TfieldDef;

//Lookup value structure
struct lkpValue
{
  QString code;
  QString desc;
};
typedef lkpValue TlkpValue;

//Table structure
struct tableDef
{
  QString name;
  QString desc;
  QList<TfieldDef> fields;
  QList<TlkpValue> lkpValues;
  QString link;
  bool isLookUp;
};
typedef tableDef TtableDef;

QList<TtableDef> tables; //List of tables
QList<fieldDef> mainID; //Main CSPro ID

QString prefix;

bool tableExist(QString table)
{
    for (int pos = 0; pos <= tables.count()-1; pos++)
    {
        if (tables[pos].name == table)
            return true;
    }
    return false;
}

//This function sort values in lookuptable by code
bool lkpComp(TlkpValue left, TlkpValue right)
{
  return left.code < right.code;
}

QString isLkpDuplicated(TtableDef table)
{
    QString res;
    int pos2;
    bool same;

    // We use this two lists to compare values after sorting them by code
    // because of ordering some values might not match.
    QList<TlkpValue> thisValues;
    QList<TlkpValue> currentValues;

    //Move the new list of values to a new list and sort it by code
    thisValues.clear();
    thisValues.append(table.lkpValues);
    qSort(thisValues.begin(),thisValues.end(),lkpComp);

    for (int pos = 0; pos <= tables.count()-1; pos++)
    {
        if (tables[pos].isLookUp)
        {
            //Move the current list of values to a new list and sort it by code
            currentValues.clear();
            currentValues.append(tables[pos].lkpValues);
            qSort(currentValues.begin(),currentValues.end(),lkpComp);

            if (currentValues.count() == thisValues.count()) //Same number of values
            {
                res = tables[pos].name;
                same = true;
                for (pos2 = 0; pos2 <= tables[pos].lkpValues.count()-1;pos2++)
                {
                    //Compares if an item in the list dont have same code or same description
                    if ((currentValues[pos2].code.simplified().toLower() != thisValues[pos2].code.simplified().toLower()) ||
                            (currentValues[pos2].desc.simplified().toLower() != thisValues[pos2].desc.simplified().toLower()))
                    {
                        res = "";
                        same = false;
                        break;
                    }
                }
                if (same)
                {
                    return res;
                }
            }
        }
    }
    return res;
}

QString getKeyField(QString table)
{
    int pos2;
    QString res;
    for (int pos = 0; pos <= tables.count()-1; pos++)
    {
        if (tables[pos].name == table)
        {
            for (pos2 = 0; pos2 <= tables[pos].fields.count()-1;pos2++)
            {
                if (tables[pos].fields[pos2].key == true)
                {
                    res = tables[pos].fields[pos2].name;
                    return res;
                }
            }
        }
    }
    return res;
}

QList<TlkpValue> getLkpValuesFromLink(QString link)
{
    QList<TlkpValue> res;
    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        if (tables[pos].link == link)
        {
            res.append(tables[pos].lkpValues);
            break;
        }
    }
    return res;
}

QList<TfieldDef> getFields(QDomNode node, bool supress) //Return a list of Fields
{
    QList<TfieldDef> res;
    QDomNodeList list;
    QDomElement item;
    QDomNode VSVNode;
    //int pos;
    QString VLName;
    QString VLName2;
    QStringList VSData;

    bool hasYes;
    bool hasNo;
    QString sameTable;

    int lkp;
    QString tableLink;

    while (!node.isNull())
    {
        TfieldDef field;
        field.decSize = 0;
        field.key = false;
        field.join = false;
        item = node.toElement();
        list = item.elementsByTagName("Label");
        field.desc = list.item(0).firstChild().nodeValue();
        list = item.elementsByTagName("Name");
        field.name = list.item(0).firstChild().nodeValue().toLower();
        list = item.elementsByTagName("Len");
        field.size = list.item(0).firstChild().nodeValue().toInt();
        list = item.elementsByTagName("DataType");


        if (list.count() > 0)
        {
            if (list.item(0).firstChild().nodeValue() == "Alpha")
                field.type = "Varchar";
            else
            {
                list = item.elementsByTagName("Decimal");
                if (list.count() > 0)
                {
                    field.type = "Decimal";
                    field.decSize = list.item(0).firstChild().nodeValue().toInt();
                }
                else
                    field.type = "INT";
            }
        }
        else
        {
            list = item.elementsByTagName("Decimal");
            if (list.count() > 0)
            {
                field.type = "Decimal";
                field.decSize = list.item(0).firstChild().nodeValue().toInt();
            }
            else
                field.type = "INT";
        }

        list = item.elementsByTagName("ValueSet");
        if (list.count() > 0)
        {
            TtableDef table;
            table.isLookUp = true;
            item = list.item(0).toElement();
            list = item.elementsByTagName("Label");
            table.desc = list.item(0).firstChild().nodeValue();
            list = item.elementsByTagName("Name");
            VLName = list.item(0).firstChild().nodeValue();

            /*pos = VLName.indexOf("_");
            if (pos >= 0)
                VLName = VLName.left(pos);*/

            VLName = VLName.replace("_","");

            VLName2 = VLName.toLower();
            VLName = "LKP" + VLName;

            VLName = VLName.toLower();
            table.name = VLName;



            TfieldDef lkpcode;
            lkpcode.desc = "Lookup field code";
            lkpcode.name = VLName2 + "_cod";
            lkpcode.type = field.type;
            lkpcode.size = field.size;
            lkpcode.decSize = 0;
            lkpcode.rField = "";
            lkpcode.rTable = "";
            lkpcode.key = true;
            lkpcode.join = false;
            table.fields.append(lkpcode);

            TfieldDef lkpdesc;
            lkpdesc.desc = "Lookup field description";
            lkpdesc.name = VLName2 + "_des";
            lkpdesc.type = "varchar";
            lkpdesc.size = 120;
            lkpdesc.decSize = 0;
            lkpdesc.rField = "";
            lkpdesc.rTable = "";
            lkpdesc.key = false;
            lkpdesc.join = false;
            table.fields.append(lkpdesc);

            list = item.elementsByTagName("ValueSetValues");
            if (list.count() > 0)
            {
                VSVNode = list.item(0).firstChild();
                while (!VSVNode.isNull())
                {

                    VSData = VSVNode.firstChild().nodeValue().split("|",QString::SkipEmptyParts);
                    TlkpValue VSValue;
                    VSValue.code = VSData[0].replace("'","").simplified();
                    if (VSData.count() > 1)
                        VSValue.desc = VSData[1].replace("'","").simplified();
                    else
                        VSValue.desc = "Value without description";
                    table.lkpValues.append(VSValue);


                    VSVNode = VSVNode.nextSibling();
                }
                list = item.elementsByTagName("Link");
                if (list.count() > 0)
                {
                    table.link = list.item(0).firstChild().nodeValue();                    
                }

            }
            else
            {                
                list = item.elementsByTagName("Link");
                if (list.count() > 0)
                {
                    tableLink = list.item(0).firstChild().nodeValue();
                    table.lkpValues.append(getLkpValuesFromLink(tableLink));                    
                }
            }


            if (supress == false)
            {
                sameTable = isLkpDuplicated(table);
                if (sameTable.isEmpty())
                {
                    if (tableExist(table.name) == false)
                        tables.append(table);
                    field.rTable = table.name;
                    field.rField = lkpcode.name;
                }
                else
                {
                    field.rTable = sameTable;
                    field.rField = getKeyField(sameTable);
                    qDebug() << table.name + " = " + sameTable + ". " + sameTable + " used instead";
                }
            }
            else
            {                
                if (table.lkpValues.count() == 2)
                {
                    hasNo = false;
                    hasYes = true;                    
                    for (lkp = 0; lkp <= table.lkpValues.count()-1;lkp++)
                    {
                        if ((table.lkpValues[lkp].code.toInt() == 0) && (table.lkpValues[lkp].desc.trimmed().toLower() == "no"))
                            hasNo = true;
                        if ((table.lkpValues[lkp].code.toInt() == 1) && (table.lkpValues[lkp].desc.trimmed().toLower() == "yes"))
                            hasYes = true;
                    }                    
                    if (hasYes && hasNo)
                    {                        
                        field.rTable = "";
                        field.rField = "";
                    }
                    else
                    {
                        sameTable = isLkpDuplicated(table);
                        if (sameTable.isEmpty())
                        {
                            if (tableExist(table.name) == false)
                                tables.append(table);
                            field.rTable = table.name;
                            field.rField = lkpcode.name;
                        }
                        else
                        {
                            field.rTable = sameTable;
                            field.rField = getKeyField(sameTable);
                            qDebug() << table.name + " = " + sameTable + ". " + sameTable + " used instead";
                        }
                    }
                }
                else
                {
                    sameTable = isLkpDuplicated(table);
                    if (sameTable.isEmpty())
                    {
                        if (tableExist(table.name) == false)
                            tables.append(table);
                        field.rTable = table.name;
                        field.rField = lkpcode.name;
                    }
                    else
                    {
                        field.rTable = sameTable;
                        field.rField = getKeyField(sameTable);
                        qDebug() << table.name + " = " + sameTable + ". " + sameTable + " used instead";
                    }
                }
            }
        }
        else
        {
            field.rField = "";
            field.rTable = "";
        }

        res.append(field);
        node = node.nextSibling();
    }

    return res;
}

bool relTableJoin(QString rtable, QString rfield)
{

    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        if (tables[pos].name == rtable)
        {
            for (int pos2 = 0; pos2 <= tables[pos].fields.count()-1;pos2++)
            {
                if (tables[pos].fields[pos2].name == rfield)
                    return tables[pos].fields[pos2].join;
            }
        }
    }

    return false;
}

void genSQL(QString ddlFile,QString insFile, QString metaFile, QString maintable, QString UUIDFile)
{
    QStringList fields;
    QStringList indexes;
    QStringList keys;
    QStringList rels;
    int clm;
    QString sql;
    QString keysql;
    QString insertSQL;
    QString field;
    int idx;
    idx = 0;

    QString index;
    QString constraint;

    QFile sqlCreateFile(ddlFile);
    if (!sqlCreateFile.open(QIODevice::WriteOnly | QIODevice::Text))
             return;
    QTextStream sqlCreateStrm(&sqlCreateFile);

    QFile sqlInsertFile(insFile);
    if (!sqlInsertFile.open(QIODevice::WriteOnly | QIODevice::Text))
             return;
    QTextStream sqlInsertStrm(&sqlInsertFile);

    QFile sqlUpdateFile(metaFile);
    if (!sqlUpdateFile.open(QIODevice::WriteOnly | QIODevice::Text))
             return;
    QTextStream sqlUpdateStrm(&sqlUpdateFile);

    QFile UUIDTriggerFile(UUIDFile);
    if (!UUIDTriggerFile.open(QIODevice::WriteOnly | QIODevice::Text))
             return;
    QTextStream UUIDStrm(&UUIDTriggerFile);

    QList<TfieldDef> joinrels;

    QDateTime date;
    date = QDateTime::currentDateTime();

    sqlCreateStrm << "-- Code generated by META" << "\n";
    sqlCreateStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
    sqlCreateStrm << "-- by: xmlToMysql Version 1.0" << "\n";
    sqlCreateStrm << "-- WARNING! All changes made in this file might be lost when running xmlToMysql again" << "\n\n";

    sqlInsertStrm << "-- Code generated by META" << "\n";
    sqlInsertStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
    sqlInsertStrm << "-- by: xmlToMysql Version 1.0" << "\n";
    sqlInsertStrm << "-- WARNING! All changes made in this file might be lost when running xmlToMysql again" << "\n\n";

    UUIDStrm << "-- Code generated by META" << "\n";
    UUIDStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
    UUIDStrm << "-- by: xmlToMysql Version 1.0" << "\n";
    UUIDStrm << "-- WARNING! All changes made in this file might be lost when running xmlToMysql again" << "\n\n";

    for (int pos = 0; pos <= tables.count()-1;pos++)
    {

        sqlUpdateStrm << "UPDATE dict_tblinfo SET tbl_des = '" + tables[pos].desc + "' WHERE tbl_cod = '" + prefix + tables[pos].name.toLower() + "';\n";

        fields.clear();
        indexes.clear();
        keys.clear();
        rels.clear();
        sql = "";
        keysql = "";

        joinrels.clear();

        fields << "CREATE TABLE IF NOT EXISTS " + prefix + tables[pos].name.toLower() + "(" << "\n";
        keys << "PRIMARY KEY (";
        for (clm = 0; clm <= tables[pos].fields.count()-1; clm++)
        {

            sqlUpdateStrm << "UPDATE dict_clminfo SET clm_des = '" + tables[pos].fields[clm].desc + "' WHERE tbl_cod = '" + prefix + tables[pos].name.toLower() + "' AND clm_cod = '" + tables[pos].fields[clm].name + "';\n";

            if (tables[pos].fields[clm].type != "Decimal")
                field = tables[pos].fields[clm].name.toLower() + " " + tables[pos].fields[clm].type + "(" + QString::number(tables[pos].fields[clm].size) + ")";
            else
                field = tables[pos].fields[clm].name.toLower() + " " + tables[pos].fields[clm].type + "(" + QString::number(tables[pos].fields[clm].size) + "," + QString::number(tables[pos].fields[clm].decSize) + ")";
            if (tables[pos].fields[clm].key == true)
                field = field + " NOT NULL , ";
            else
                field = field + " , ";
            fields << field << "\n";

            if (tables[pos].fields[clm].key == true)
                keys << tables[pos].fields[clm].name + " , ";
            if (!tables[pos].fields[clm].rTable.isEmpty())
            {

                if (relTableJoin(tables[pos].name,tables[pos].fields[clm].name) == false)
                {

                    idx++;

                    index = "INDEX fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower() + "_" + tables[pos].fields[clm].rTable.toLower() ;
                    indexes << index.left(64) + " (" + tables[pos].fields[clm].name.toLower() + ") , " << "\n";

                    constraint = "CONSTRAINT fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower() + "_" + tables[pos].fields[clm].rTable.toLower();
                    rels << constraint.left(64) << "\n";
                    rels << "FOREIGN KEY (" + tables[pos].fields[clm].name.toLower() + ")" << "\n";
                    rels << "REFERENCES " + prefix + tables[pos].fields[clm].rTable.toLower() + " (" + tables[pos].fields[clm].rField.toLower() + ")" << "\n";
                    if (tables[pos].fields[clm].rTable.toLower() == maintable.toLower())
                        rels << "ON DELETE CASCADE " << "\n";
                    else
                        rels << "ON DELETE RESTRICT " << "\n";
                    rels << "ON UPDATE NO ACTION," << "\n";
                }
                else
                {
                    joinrels.append(tables[pos].fields[clm]); //If its a join relationship we need to process it later
                }

            }

        }
        //Process join relationships if there is any
        if (joinrels.count() > 0)
        {
            idx++;
            index = "INDEX fkjoin" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower();
            index = index.left(64) + " (";
            constraint = "CONSTRAINT fkjoin" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower();
            rels << constraint.left(64) << "\n";

            QString foreignKey;
            QString references;

            foreignKey = "FOREIGN KEY (";

            // TODO: We asume only the multiple references only goes to one unique table
            // This might be OK since in CSPro we are linking all child tables to the main table

            references = "REFERENCES " + prefix + joinrels[0].rTable.toLower() + " (";

            for (clm = 0; clm <= joinrels.count()-1; clm++)
            {
                index = index + joinrels[clm].name + ",";
                foreignKey = foreignKey + joinrels[clm].name + ",";
                references = references + joinrels[clm].name + ",";
            }

            //Append the inddex
            index = index.left(index.length()-1) + ") , ";
            indexes << index << "\n";

            //Append the foreign Key
            foreignKey = foreignKey.left(foreignKey.length()-1) + ")";
            rels << foreignKey << "\n";

            //Append the references
            references = references.left(references.length()-1) + ")";
            rels << references << "\n";
            rels << "ON DELETE CASCADE " << "\n";
            rels << "ON UPDATE NO ACTION," << "\n";

        }


        for (clm = 0; clm <= fields.count() -1;clm++)
        {
            sql = sql + fields[clm];
        }
        for (clm = 0; clm <= keys.count() -1;clm++)
        {
            keysql = keysql + keys[clm];
        }
        clm = keysql.lastIndexOf(",");
        keysql = keysql.left(clm) + ") , \n";

        sql = sql + keysql;

        for (clm = 0; clm <= indexes.count() -1;clm++)
        {
            sql = sql + indexes[clm];
        }
        for (clm = 0; clm <= rels.count() -1;clm++)
        {
            sql = sql + rels[clm];
        }
        clm = sql.lastIndexOf(",");
        sql = sql.left(clm);
        sql = sql + ")" + "\n ENGINE = InnoDB; \n\n";

        //Append UUIDs triggers to the the
        UUIDStrm << "CREATE TRIGGER uudi_" + prefix + tables[pos].name.toLower() + " BEFORE INSERT ON " + prefix + tables[pos].name.toLower() + " FOR EACH ROW SET new.rowuuid = uuid();\n\n";

        sqlCreateStrm << sql;

        if (tables[pos].lkpValues.count() > 0)
        {
            for (clm = 0; clm <= tables[pos].lkpValues.count()-1;clm++)
            {
                insertSQL = "INSERT INTO " + prefix + tables[pos].name.toLower() + " (";
                for (int pos2 = 0; pos2 <= tables[pos].fields.count()-2;pos2++)
                {
                    insertSQL = insertSQL + tables[pos].fields[pos2].name + ",";
                }
                insertSQL = insertSQL.left(insertSQL.length()-1) + ")  VALUES ('";

                insertSQL = insertSQL + tables[pos].lkpValues[clm].code + "','";
                insertSQL = insertSQL + tables[pos].lkpValues[clm].desc + "');";
                sqlInsertStrm << insertSQL << "\n";
            }
        }
    }
}

void appendUUIDs()
{
    int pos;
    for (pos = 0; pos <= tables.count()-1;pos++)
    {
        TfieldDef UUIDField;
        UUIDField.name = "rowuuid";
        UUIDField.desc = "Unique Row Identifier (UUID)";
        UUIDField.key = false;
        UUIDField.type = "varchar";
        UUIDField.size = 80;
        UUIDField.decSize = 0;
        UUIDField.rTable = "";
        UUIDField.rField = "";
        tables[pos].fields.append(UUIDField);
    }
}

int main(int argc, char *argv[])
{
    QString title;
    title = title + "****************************************************************** \n";
    title = title + " * XMLToMySQL 1.0                                                 * \n";
    title = title + " * This tool generates MySQL scripts using the CSPro XML file.    * \n";
    title = title + " * The tool generate three scripts:                               * \n";
    title = title + " * create.sql (DDL SQL code for all records in the XML)           * \n";
    title = title + " * insert.sql (Inserts of lookup table values)                    * \n";
    title = title + " * dict.sql (Inserts of metadata into the dictionary files)       * \n";
    title = title + " * This tool is part of CSPro Tools (c) ILRI, 2012                * \n";
    title = title + " ****************************************************************** \n";

    TCLAP::CmdLine cmd(title.toLatin1().constData(), ' ', "1.0 (Beta 1)");
    //Required arguments
    TCLAP::ValueArg<std::string> inputArg("x","inputXML","Input XML File",true,"","string");
    TCLAP::ValueArg<std::string> tableArg("t","mainTable","Main record in the XML that will become the main table",true,"","string");
    TCLAP::ValueArg<std::string> ddlArg("c","outputdml","Output DDL file. Default ./create.sql",false,"./create.sql","string");
    TCLAP::ValueArg<std::string> insertArg("i","outputinsert","Output insert file. Default ./insert.sql",false,"./insert.sql","string");
    TCLAP::ValueArg<std::string> metadataArg("m","outputmetadata","Output metadata file. Default ./metadata.sql",false,"./metadata.sql","string");
    TCLAP::ValueArg<std::string> prefixArg("p","prefix","Prefix for each table. _ is added to the prefix. Default no prefix",false,"","string");
    TCLAP::ValueArg<std::string> uuidFileArg("u","uuidfile","Output UUID trigger file",false,"./uuid-triggers.sql","string");

    TCLAP::SwitchArg includeYesNoLkpSwitch("l","inclkps","Add this argument to include Yes/No lookup tables", cmd, false);

    cmd.add(inputArg);
    cmd.add(tableArg);
    cmd.add(ddlArg);
    cmd.add(insertArg);
    cmd.add(metadataArg);
    cmd.add(prefixArg);
    cmd.add(uuidFileArg);
    //Parsing the command lines
    cmd.parse( argc, argv );

    //Getting the variables from the command

    QString input = QString::fromUtf8(inputArg.getValue().c_str());
    QString ddl = QString::fromUtf8(ddlArg.getValue().c_str());
    QString insert = QString::fromUtf8(insertArg.getValue().c_str());
    QString metadata = QString::fromUtf8(metadataArg.getValue().c_str());
    QString mainTable = QString::fromUtf8(tableArg.getValue().c_str());
    QString UUIDFile = QString::fromUtf8(uuidFileArg.getValue().c_str());
    prefix = QString::fromUtf8(prefixArg.getValue().c_str());

    if (prefix != "")
        prefix = prefix + "_";
    else
        prefix = "";

    bool ignoreYesNo = !includeYesNoLkpSwitch.getValue();


    QDomDocument doc("mydocument");
    QFile xmlfile(input);
    if (!xmlfile.open(QIODevice::ReadOnly))
        return 1;
    if (!doc.setContent(&xmlfile))
    {
        xmlfile.close();
        return 1;
    }
    xmlfile.close();

    QDomNodeList list;
    QDomNodeList list2;
    QDomElement item;
    QDomElement record;
    int pos;
    list = doc.elementsByTagName("IdItems");
    if (list.count() > 0)
    {
        item = list.item(0).toElement();
        list = item.elementsByTagName("Item");

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
                    varmainID.type = "Varchar";
                else
                    varmainID.type = "INT";
            }
            else
            {
                varmainID.type = "INT";
            }
            varmainID.key = true;
            varmainID.join = true;
            varmainID.rField = "";
            varmainID.rTable = "";
            varmainID.decSize = 0;

            mainID.append(varmainID);
        }

        if (list.count() > 0)
        {

        }
    }
    else
    {
        qDebug() << "Ups no ID items!";
        return 1;
    }
    /*TtableDef mainTable;
    mainTable.name = "mainsurveyinfo";
    mainTable.desc = "Main Survey Information";
    mainTable.fields.append(mainID);
    tables.append(mainTable);*/

    QList<fieldDef> mainRelField;

    for (pos = 0; pos <= mainID.count()-1;pos++)
    {
        fieldDef varmainRelField;

        varmainRelField.name = mainID[pos].name;
        varmainRelField.desc = mainID[pos].desc;
        varmainRelField.size = mainID[pos].size;
        varmainRelField.type = mainID[pos].type;
        varmainRelField.key = true;
        varmainRelField.decSize = 0;
        varmainRelField.rTable = mainTable;
        varmainRelField.rField = mainID[pos].name;

        mainRelField.append(varmainRelField);

    }




    fieldDef childKey;
    childKey.name = "record_id";
    childKey.desc = "Record ID";
    childKey.size = 3;
    childKey.type = "INT";
    childKey.key = true;
    childKey.decSize = 0;
    childKey.rTable = "";
    childKey.rField = "";


    list = doc.elementsByTagName("Records");
    if (list.count() > 0)
    {
        record = list.item(0).firstChild().toElement(); //First record
        while (!record.isNull())
        {
            TtableDef table;
            table.isLookUp = false;
            list = record.elementsByTagName("Label");
            table.desc = list.item(0).firstChild().nodeValue();

            list = record.elementsByTagName("Name");
            table.name = list.item(0).firstChild().nodeValue().toLower();


            if (table.name.toLower() != mainTable.toLower())
            {
                table.fields.append(mainRelField);
                table.fields.append(childKey);
            }
            else
            {
                table.fields.append(mainID);
            }

            list = record.elementsByTagName("Items");

            table.fields.append(getFields(list.item(0).firstChild(),ignoreYesNo));

            if (tableExist(table.name) == false)
                tables.append(table);

            record = record.nextSibling().toElement();
        }
    }
    else
    {
        qDebug() << "Ups no Records!";
        return 1;
    }

    appendUUIDs();

    genSQL(ddl,insert,metadata,mainTable.toLower(),UUIDFile);


    return 0;
}
